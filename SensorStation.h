// ============================================================================
//  SensorStation.h  -  UNIT II : data abstraction, const correctness,
//                      static members, operator overloading
//  Also defines RegionalZone (result of the overloaded binary operator+).
// ============================================================================
#ifndef ALF_SENSOR_STATION_H
#define ALF_SENSOR_STATION_H

#include <iosfwd>
#include <ostream>
#include <istream>
#include <string>
#include <vector>
#include "AlertLevel.h"
#include "TelemetryBuffer.h"
#include "TelemetryTypes.h"

namespace DisasterCore {
namespace Sensors {

// ---- custom stream manipulators that switch operator<< between two layouts ----
inline int stationFormatIndex() { static const int idx = std::ios_base::xalloc(); return idx; }
inline std::ostream& configFormat(std::ostream& os)  { os.iword(stationFormatIndex()) = 1; return os; }
inline std::ostream& displayFormat(std::ostream& os) { os.iword(stationFormatIndex()) = 0; return os; }

class SensorStation {
    friend class TelemetryAuditor;                                   // UNIT I friend class
    friend std::ostream& operator<<(std::ostream&, const SensorStation&);
    friend std::istream& operator>>(std::istream&, SensorStation&);

    // ---- private data (strict abstraction: no getters for the raw metrics) ----
    std::string id_;
    std::string name_;
    std::string zone_;
    std::string kind_;            // soil | tilt | sonar | hybrid
    GeoCoordinate location_;
    TelemetryBuffer buffer_;      // containment of a dynamic buffer
    double hazardIndex_;
    double peakHazard_;
    Alerts::AlertLevel level_;
    unsigned totalReadings_;
    unsigned offlineEvents_;
    bool online_;

    // ---- network-wide statics shared by every station ----
    static int activeStations_;
    static int totalCreated_;

public:
    SensorStation();
    SensorStation(const std::string& id, const std::string& name, const std::string& zone,
                  const std::string& kind, const GeoCoordinate& location,
                  std::size_t bufferCapacity = 8);
    SensorStation(const SensorStation& other);                  // deep copy (via TelemetryBuffer)
    SensorStation& operator=(const SensorStation& other) = default;
    ~SensorStation();

    static int activeStations() { return activeStations_; }
    static int totalCreated()   { return totalCreated_; }

    const std::string& id() const   { return id_; }
    const std::string& name() const { return name_; }
    const std::string& zone() const { return zone_; }
    const std::string& kind() const { return kind_; }
    const GeoCoordinate& location() const { return location_; }
    double hazardIndex() const { return hazardIndex_; }
    Alerts::AlertLevel alertLevel() const { return level_; }
    bool isOnline() const { return online_; }
    std::size_t bufferedReadings() const { return buffer_.size(); }

    void recordReading(const TelemetryReading& reading, double hazard, Alerts::AlertLevel level);
    void markOffline();

    // Relational operator: a station is "greater" when its hazard is higher.
    bool operator>(const SensorStation& other) const;
};

std::ostream& operator<<(std::ostream& os, const SensorStation& s);
std::istream& operator>>(std::istream& is, SensorStation& s);

// ----------------------------------------------------------------------------
//  RegionalZone : aggregate of several stations (the "regional zone vector")
// ----------------------------------------------------------------------------
class RegionalZone {
public:
    struct Member {
        std::string stationId;
        double hazard;
        Alerts::AlertLevel level;
    };
private:
    std::string name_;
    std::vector<Member> members_;
public:
    explicit RegionalZone(const std::string& name = "UNASSIGNED") : name_(name) {}

    void add(const SensorStation& s);
    RegionalZone& merge(const RegionalZone& other);

    const std::string& name() const { return name_; }
    std::size_t size() const { return members_.size(); }
    const std::vector<Member>& members() const { return members_; }
    double peakHazard() const;
    double meanHazard() const;
    Alerts::AlertLevel worstLevel() const;
};

// Overloaded binary operator+ : station + station, zone + station, zone + zone
RegionalZone operator+(const SensorStation& a, const SensorStation& b);
RegionalZone operator+(const RegionalZone& zone, const SensorStation& s);
RegionalZone operator+(const RegionalZone& a, const RegionalZone& b);
std::ostream& operator<<(std::ostream& os, const RegionalZone& z);

} // namespace Sensors
} // namespace DisasterCore

#endif
