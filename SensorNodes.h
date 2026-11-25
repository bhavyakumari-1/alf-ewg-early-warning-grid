// ============================================================================
//  SensorNodes.h  -  UNIT III : inheritance, containership, polymorphism
//
//  Class hierarchy (virtual inheritance resolves the diamond):
//
//                 DisasterTelemetryDevice            <- root (single shared copy)
//                           ^  (virtual)
//                     BaseSensorNode  (abstract, HAS-A PowerSubsystem)
//                 ^  (virtual)           ^  (virtual)
//       SolarPoweredNode        WirelessLoRaTransceiver
//                 ^                       ^
//                 +---- AutonomousHybridProbe ----+      <- derived leaf
//
//  Besides the hybrid probe, three simple concrete nodes derive directly from
//  BaseSensorNode so that the sweep loop shows real polymorphic variety.
// ============================================================================
#ifndef ALF_SENSOR_NODES_H
#define ALF_SENSOR_NODES_H

#include <cstddef>
#include <cstdint>
#include <string>
#include "AlertLevel.h"
#include "PowerSubsystem.h"
#include "TelemetryTypes.h"

namespace DisasterCore {
namespace Sensors {

enum class FaultMode { None, DeadSensor, CorruptPacket, RangeOverflow };

struct Thresholds {
    double watch;
    double warning;
    double evacuate;
};

// ---------------------------------------------------------------------------
//  Root of the diamond
// ---------------------------------------------------------------------------
class DisasterTelemetryDevice {
    std::string deviceId_;
    GeoCoordinate location_;
    std::string firmware_;
protected:
    DisasterTelemetryDevice();   // used only when a virtual base is not the most-derived initialiser
public:
    DisasterTelemetryDevice(const std::string& id, const GeoCoordinate& location,
                            const std::string& firmware = "v1.0");
    virtual ~DisasterTelemetryDevice();

    const std::string& deviceId() const { return deviceId_; }
    const std::string& firmware() const { return firmware_; }
    const GeoCoordinate& location() const { return location_; }
    GeoCoordinate& location() { return location_; }        // return by reference
    virtual std::string describe() const;
};

// ---------------------------------------------------------------------------
//  Abstract base : pure virtual readTelemetry() and evaluateThreshold()
// ---------------------------------------------------------------------------
class BaseSensorNode : public virtual DisasterTelemetryDevice {
    PowerSubsystem power_;            // CONTAINERSHIP (composition) - not inheritance
    TelemetryReading lastReading_;
    double stress_;                   // simulated environmental stress 0..1
    unsigned sweepNo_;
    FaultMode fault_;
    Thresholds thresholds_;
    std::uint32_t rngState_;          // portable linear-congruential generator

    double nextUniform();             // 0..1

protected:
    BaseSensorNode();                 // for intermediate classes (virtual base rule)
    BaseSensorNode(std::uint32_t seed, const PowerSubsystem& power, const Thresholds& thresholds);

    TelemetryReading generate();                      // builds a full simulated sample
    TelemetryReading finalize(TelemetryReading r);    // applies faults, stamps checksum
    Alerts::AlertLevel classify(double hazard) const;
    void validateRanges(const TelemetryReading& r) const;   // may throw ThresholdOverflowException
    PowerSubsystem& power() { return power_; }

public:
    virtual ~BaseSensorNode();

    // ---- the two pure virtual functions required by the specification ----
    virtual TelemetryReading readTelemetry() = 0;
    virtual Alerts::AlertLevel evaluateThreshold() const = 0;

    virtual double hazardIndex() const = 0;
    virtual std::string nodeType() const = 0;
    std::string describe() const override;

    void setEnvironment(double stress, unsigned sweepNo);
    void injectFault(FaultMode mode) { fault_ = mode; }
    FaultMode fault() const { return fault_; }
    void drainBattery(double percent) { power_.consume(percent); }
    const PowerSubsystem& powerSubsystem() const { return power_; }
    const TelemetryReading& lastReading() const { return lastReading_; }
};

// ---------------------------------------------------------------------------
//  Intermediate derivations (both inherit BaseSensorNode VIRTUALLY)
// ---------------------------------------------------------------------------
class SolarPoweredNode : public virtual BaseSensorNode {
    double panelWatts_;
protected:
    explicit SolarPoweredNode(double panelWatts = 10.0) : panelWatts_(panelWatts) {}
    void harvestSolar(double sunlightFraction);
public:
    virtual ~SolarPoweredNode() {}
    double panelWatts() const { return panelWatts_; }
};

class WirelessLoRaTransceiver : public virtual BaseSensorNode {
    int spreadingFactor_;
    unsigned packetsSent_;
    std::size_t bytesSent_;
protected:
    explicit WirelessLoRaTransceiver(int spreadingFactor = 9)
        : spreadingFactor_(spreadingFactor), packetsSent_(0), bytesSent_(0) {}
    std::size_t transmit(const TelemetryReading& reading);
public:
    virtual ~WirelessLoRaTransceiver() {}
    int spreadingFactor() const { return spreadingFactor_; }
    unsigned packetsSent() const { return packetsSent_; }
    std::size_t bytesSent() const { return bytesSent_; }
};

// ---------------------------------------------------------------------------
//  Derived leaf: solar + LoRa + every transducer
// ---------------------------------------------------------------------------
class AutonomousHybridProbe : public SolarPoweredNode, public WirelessLoRaTransceiver {
public:
    AutonomousHybridProbe(const std::string& id, const GeoCoordinate& location,
                          std::uint32_t seed, double batteryPercent = 100.0);

    TelemetryReading readTelemetry() override;
    Alerts::AlertLevel evaluateThreshold() const override;
    double hazardIndex() const override;
    std::string nodeType() const override { return "HybridProbe"; }
    std::string describe() const override;

    // True when both parents reach the SAME DisasterTelemetryDevice sub-object.
    bool singleRootInstance() const;
};

// ---------------------------------------------------------------------------
//  Simple concrete nodes (direct children of BaseSensorNode)
// ---------------------------------------------------------------------------
class SoilGeophoneNode : public BaseSensorNode {
public:
    SoilGeophoneNode(const std::string& id, const GeoCoordinate& location,
                     std::uint32_t seed, double batteryPercent = 100.0);
    TelemetryReading readTelemetry() override;
    Alerts::AlertLevel evaluateThreshold() const override;
    double hazardIndex() const override;
    std::string nodeType() const override { return "SoilGeophone"; }
};

class AcousticTiltmeterNode : public BaseSensorNode {
public:
    AcousticTiltmeterNode(const std::string& id, const GeoCoordinate& location,
                          std::uint32_t seed, double batteryPercent = 100.0);
    TelemetryReading readTelemetry() override;
    Alerts::AlertLevel evaluateThreshold() const override;
    double hazardIndex() const override;
    std::string nodeType() const override { return "Tiltmeter"; }
};

class RiverSonarNode : public BaseSensorNode {
public:
    RiverSonarNode(const std::string& id, const GeoCoordinate& location,
                   std::uint32_t seed, double batteryPercent = 100.0);
    TelemetryReading readTelemetry() override;
    Alerts::AlertLevel evaluateThreshold() const override;
    double hazardIndex() const override;
    std::string nodeType() const override { return "RiverSonar"; }
};

} // namespace Sensors
} // namespace DisasterCore

#endif
