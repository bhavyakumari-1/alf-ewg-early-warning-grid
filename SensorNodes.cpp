#include "SensorNodes.h"
#include "Exceptions.h"
#include "HazardMath.h"
#include <iomanip>
#include <sstream>

namespace DisasterCore {
namespace Sensors {

using Alerts::AlertLevel;

// ===========================================================================
//  DisasterTelemetryDevice
// ===========================================================================
DisasterTelemetryDevice::DisasterTelemetryDevice()
    : deviceId_("UNINITIALISED"), location_(), firmware_("v0") {}

DisasterTelemetryDevice::DisasterTelemetryDevice(const std::string& id, const GeoCoordinate& location,
                                                 const std::string& firmware)
    : deviceId_(id), location_(location), firmware_(firmware) {}

DisasterTelemetryDevice::~DisasterTelemetryDevice() {}

std::string DisasterTelemetryDevice::describe() const {
    return "Device " + deviceId_ + " fw " + firmware_;
}

// ===========================================================================
//  BaseSensorNode
// ===========================================================================
namespace {
    const double kReadCostPercent = 2.0;      // battery consumed per sample

    // Physical limits of the transducers (anything above is a hardware fault).
    const double kMaxRainfall = 400.0;   // mm/h
    const double kMaxSlope = 90.0;       // degrees
    const double kMaxSaturation = 100.0; // %
    const double kMaxVibration = 100.0;  // Hz
    const double kMaxRiverLevel = 30.0;  // m
}

BaseSensorNode::BaseSensorNode()
    : power_(), lastReading_(), stress_(0.0), sweepNo_(0), fault_(FaultMode::None),
      thresholds_(), rngState_(1u) {
    thresholds_.watch = 40.0; thresholds_.warning = 60.0; thresholds_.evacuate = 80.0;
}

BaseSensorNode::BaseSensorNode(std::uint32_t seed, const PowerSubsystem& power,
                               const Thresholds& thresholds)
    : power_(power), lastReading_(), stress_(0.0), sweepNo_(0), fault_(FaultMode::None),
      thresholds_(thresholds), rngState_(seed ? seed : 1u) {}

BaseSensorNode::~BaseSensorNode() {}

double BaseSensorNode::nextUniform() {
    rngState_ = rngState_ * 1664525u + 1013904223u;          // Numerical Recipes LCG
    return static_cast<double>(rngState_ >> 8) / 16777216.0; // 24 random bits -> [0,1)
}

void BaseSensorNode::setEnvironment(double stress, unsigned sweepNo) {
    stress_ = clamp01(stress);
    sweepNo_ = sweepNo;
}

TelemetryReading BaseSensorNode::generate() {
    if (fault_ == FaultMode::DeadSensor)
        throw SensorFailureException(deviceId(), "transducer not responding (watchdog timeout)");
    if (power_.isDepleted()) {
        std::ostringstream os;
        os << "battery depleted (" << std::fixed << std::setprecision(1)
           << power_.batteryPercent() << "%)";
        throw SensorFailureException(deviceId(), os.str());
    }
    power_.consume(kReadCostPercent);

    const double s = stress_;
    TelemetryReading r;
    r.rainfallRate   = 150.0 * s * (1.0 + 0.10 * (nextUniform() - 0.5));
    r.slopeAngle     = (18.0 + 37.0 * s) * (1.0 + 0.06 * (nextUniform() - 0.5));
    r.soilSaturation = (28.0 + 68.0 * s) * (1.0 + 0.06 * (nextUniform() - 0.5));
    r.vibrationHz    = (1.5 + 22.0 * s * s) * (1.0 + 0.10 * (nextUniform() - 0.5));
    r.riverLevel     = (0.8 + 6.8 * s) * (1.0 + 0.06 * (nextUniform() - 0.5));
    if (r.soilSaturation > 100.0) r.soilSaturation = 100.0;
    return r;
}

TelemetryReading BaseSensorNode::finalize(TelemetryReading r) {
    r.sweepNo = sweepNo_;
    if (fault_ == FaultMode::RangeOverflow) {      // sensor saturates: absurd values
        r.rainfallRate *= 50.0;  r.slopeAngle *= 50.0;  r.soilSaturation *= 50.0;
        r.vibrationHz *= 50.0;   r.riverLevel *= 50.0;
    }
    r.checksum = computeChecksum(r);
    if (fault_ == FaultMode::CorruptPacket)        // bit-flip AFTER the checksum was computed
        r.rainfallRate += 3.3;
    lastReading_ = r;
    return r;
}

AlertLevel BaseSensorNode::classify(double hazard) const {
    if (hazard >= thresholds_.evacuate) return AlertLevel::Evacuate;
    if (hazard >= thresholds_.warning)  return AlertLevel::Warning;
    if (hazard >= thresholds_.watch)    return AlertLevel::Watch;
    return AlertLevel::Normal;
}

void BaseSensorNode::validateRanges(const TelemetryReading& r) const {
    if (r.rainfallRate > kMaxRainfall)
        throw ThresholdOverflowException(deviceId(), "rainfallRate", r.rainfallRate, kMaxRainfall);
    if (r.slopeAngle > kMaxSlope)
        throw ThresholdOverflowException(deviceId(), "slopeAngle", r.slopeAngle, kMaxSlope);
    if (r.soilSaturation > kMaxSaturation)
        throw ThresholdOverflowException(deviceId(), "soilSaturation", r.soilSaturation, kMaxSaturation);
    if (r.vibrationHz > kMaxVibration)
        throw ThresholdOverflowException(deviceId(), "vibrationHz", r.vibrationHz, kMaxVibration);
    if (r.riverLevel > kMaxRiverLevel)
        throw ThresholdOverflowException(deviceId(), "riverLevel", r.riverLevel, kMaxRiverLevel);
}

std::string BaseSensorNode::describe() const {
    std::ostringstream os;
    os << nodeType() << ' ' << deviceId() << " battery=" << std::fixed << std::setprecision(0)
       << power_.batteryPercent() << '%';
    return os.str();
}

// ===========================================================================
//  Intermediate classes
// ===========================================================================
void SolarPoweredNode::harvestSolar(double sunlightFraction) {
    power().charge(panelWatts_ * clamp01(sunlightFraction) * 0.25);   // % gained per sweep
}

std::size_t WirelessLoRaTransceiver::transmit(const TelemetryReading& reading) {
    (void)reading;
    const std::size_t payload = sizeof(TelemetryReading);
    ++packetsSent_;
    bytesSent_ += payload;
    return payload;
}

// ===========================================================================
//  AutonomousHybridProbe (diamond leaf)
// ===========================================================================
AutonomousHybridProbe::AutonomousHybridProbe(const std::string& id, const GeoCoordinate& location,
                                             std::uint32_t seed, double batteryPercent)
    : DisasterTelemetryDevice(id, location, "v2.1-hybrid"),      // virtual bases are initialised
      BaseSensorNode(seed, PowerSubsystem(batteryPercent, 12.0),  // by the MOST-DERIVED class
                     Thresholds{ 38.0, 58.0, 76.0 }),
      SolarPoweredNode(12.0),
      WirelessLoRaTransceiver(9) {}

TelemetryReading AutonomousHybridProbe::readTelemetry() {
    harvestSolar(0.6);                       // from SolarPoweredNode
    TelemetryReading r = finalize(generate());
    transmit(r);                             // from WirelessLoRaTransceiver
    return r;
}

double AutonomousHybridProbe::hazardIndex() const { return computeHazardIndex(lastReading()); }

AlertLevel AutonomousHybridProbe::evaluateThreshold() const {
    validateRanges(lastReading());
    return classify(hazardIndex());
}

std::string AutonomousHybridProbe::describe() const {
    std::ostringstream os;
    os << BaseSensorNode::describe() << " | solar " << panelWatts() << "W | LoRa SF"
       << spreadingFactor() << " | packets=" << packetsSent();
    return os.str();
}

bool AutonomousHybridProbe::singleRootInstance() const {
    const DisasterTelemetryDevice* viaSolar = static_cast<const SolarPoweredNode*>(this);
    const DisasterTelemetryDevice* viaLora  = static_cast<const WirelessLoRaTransceiver*>(this);
    return viaSolar == viaLora;
}

// ===========================================================================
//  Simple concrete nodes
// ===========================================================================
SoilGeophoneNode::SoilGeophoneNode(const std::string& id, const GeoCoordinate& location,
                                   std::uint32_t seed, double batteryPercent)
    : DisasterTelemetryDevice(id, location, "v1.4-soil"),
      BaseSensorNode(seed, PowerSubsystem(batteryPercent, 0.0), Thresholds{ 40.0, 60.0, 80.0 }) {}

TelemetryReading SoilGeophoneNode::readTelemetry() {
    TelemetryReading r = generate();
    r.rainfallRate = 0.0;  r.slopeAngle = 0.0;  r.riverLevel = 0.0;   // not measured by this probe
    return finalize(r);
}
double SoilGeophoneNode::hazardIndex() const {
    const SoilVibration sv = { lastReading().soilSaturation, lastReading().vibrationHz };
    return computeHazardIndex(sv);                                    // overload #2
}
AlertLevel SoilGeophoneNode::evaluateThreshold() const {
    validateRanges(lastReading());
    return classify(hazardIndex());
}

AcousticTiltmeterNode::AcousticTiltmeterNode(const std::string& id, const GeoCoordinate& location,
                                             std::uint32_t seed, double batteryPercent)
    : DisasterTelemetryDevice(id, location, "v1.2-tilt"),
      BaseSensorNode(seed, PowerSubsystem(batteryPercent, 0.0), Thresholds{ 45.0, 65.0, 85.0 }) {}

TelemetryReading AcousticTiltmeterNode::readTelemetry() {
    TelemetryReading r = generate();
    r.soilSaturation = 0.0;  r.vibrationHz = 0.0;  r.riverLevel = 0.0;
    return finalize(r);
}
double AcousticTiltmeterNode::hazardIndex() const {
    const RainfallSlope rs = { lastReading().rainfallRate, lastReading().slopeAngle };
    return computeHazardIndex(rs);                                    // overload #1
}
AlertLevel AcousticTiltmeterNode::evaluateThreshold() const {
    validateRanges(lastReading());
    return classify(hazardIndex());
}

RiverSonarNode::RiverSonarNode(const std::string& id, const GeoCoordinate& location,
                               std::uint32_t seed, double batteryPercent)
    : DisasterTelemetryDevice(id, location, "v1.0-sonar"),
      BaseSensorNode(seed, PowerSubsystem(batteryPercent, 0.0), Thresholds{ 40.0, 60.0, 80.0 }) {}

TelemetryReading RiverSonarNode::readTelemetry() {
    TelemetryReading r = generate();
    r.slopeAngle = 0.0;  r.soilSaturation = 0.0;  r.vibrationHz = 0.0;
    return finalize(r);
}
double RiverSonarNode::hazardIndex() const {
    const RiverRain rr = { lastReading().riverLevel, lastReading().rainfallRate };
    return computeHazardIndex(rr);                                    // overload #3
}
AlertLevel RiverSonarNode::evaluateThreshold() const {
    validateRanges(lastReading());
    return classify(hazardIndex());
}

} // namespace Sensors
} // namespace DisasterCore
