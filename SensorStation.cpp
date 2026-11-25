#include "SensorStation.h"
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace DisasterCore {
namespace Sensors {

int SensorStation::activeStations_ = 0;
int SensorStation::totalCreated_ = 0;

SensorStation::SensorStation()
    : id_("UNSET"), name_("UNSET"), zone_("UNASSIGNED"), kind_("none"), location_(),
      buffer_(8), hazardIndex_(0.0), peakHazard_(0.0), level_(Alerts::AlertLevel::Normal),
      totalReadings_(0), offlineEvents_(0), online_(true) {
    ++activeStations_;
    ++totalCreated_;
}

SensorStation::SensorStation(const std::string& id, const std::string& name, const std::string& zone,
                             const std::string& kind, const GeoCoordinate& location,
                             std::size_t bufferCapacity)
    : id_(id), name_(name), zone_(zone), kind_(kind), location_(location),
      buffer_(bufferCapacity), hazardIndex_(0.0), peakHazard_(0.0),
      level_(Alerts::AlertLevel::Normal), totalReadings_(0), offlineEvents_(0), online_(true) {
    ++activeStations_;
    ++totalCreated_;
}

SensorStation::SensorStation(const SensorStation& o)
    : id_(o.id_), name_(o.name_), zone_(o.zone_), kind_(o.kind_), location_(o.location_),
      buffer_(o.buffer_),                       // invokes TelemetryBuffer deep copy
      hazardIndex_(o.hazardIndex_), peakHazard_(o.peakHazard_), level_(o.level_),
      totalReadings_(o.totalReadings_), offlineEvents_(o.offlineEvents_), online_(o.online_) {
    ++activeStations_;
    ++totalCreated_;
}

SensorStation::~SensorStation() { --activeStations_; }

void SensorStation::recordReading(const TelemetryReading& reading, double hazard,
                                  Alerts::AlertLevel level) {
    buffer_.push(reading);
    hazardIndex_ = hazard;
    peakHazard_ = std::max(peakHazard_, hazard);
    level_ = level;
    ++totalReadings_;
    online_ = true;
}

void SensorStation::markOffline() {
    if (online_) ++offlineEvents_;
    online_ = false;
}

bool SensorStation::operator>(const SensorStation& other) const {
    return hazardIndex_ > other.hazardIndex_;
}

// ---- stream insertion: console layout or config-file layout (see manipulators) ----
std::ostream& operator<<(std::ostream& os, const SensorStation& s) {
    std::ostringstream num;
    if (os.iword(stationFormatIndex()) == 1) {
        // Same field order that operator>> expects -> perfect file round-trip.
        num << std::fixed << std::setprecision(5) << s.location_.latitude << ' '
            << s.location_.longitude << ' ' << std::setprecision(0) << s.location_.elevation;
        os << s.id_ << ' ' << s.name_ << ' ' << s.zone_ << ' ' << s.kind_ << ' ' << num.str();
    } else {
        num << std::fixed << std::setprecision(1) << s.hazardIndex_;
        os << '[' << s.id_ << "] " << s.name_ << " (" << s.zone_ << ", " << s.kind_
           << ") hazard=" << num.str() << " level=" << Alerts::toString(s.level_)
           << (s.online_ ? "" : " [OFFLINE]");
    }
    return os;
}

// ---- stream extraction: "id name zone kind latitude longitude elevation" ----
std::istream& operator>>(std::istream& is, SensorStation& s) {
    std::string id, name, zone, kind;
    GeoCoordinate g;
    if (is >> id >> name >> zone >> kind >> g.latitude >> g.longitude >> g.elevation) {
        s.id_ = id;  s.name_ = name;  s.zone_ = zone;  s.kind_ = kind;  s.location_ = g;
    }
    return is;
}

// ----------------------------------------------------------------------------
//  RegionalZone
// ----------------------------------------------------------------------------
void RegionalZone::add(const SensorStation& s) {
    Member m = { s.id(), s.hazardIndex(), s.alertLevel() };
    members_.push_back(m);
}

RegionalZone& RegionalZone::merge(const RegionalZone& other) {
    members_.insert(members_.end(), other.members_.begin(), other.members_.end());
    return *this;
}

double RegionalZone::peakHazard() const {
    double peak = 0.0;
    for (std::size_t i = 0; i < members_.size(); ++i) peak = std::max(peak, members_[i].hazard);
    return peak;
}

double RegionalZone::meanHazard() const {
    if (members_.empty()) return 0.0;
    double sum = 0.0;
    for (std::size_t i = 0; i < members_.size(); ++i) sum += members_[i].hazard;
    return sum / static_cast<double>(members_.size());
}

Alerts::AlertLevel RegionalZone::worstLevel() const {
    Alerts::AlertLevel worst = Alerts::AlertLevel::Normal;
    for (std::size_t i = 0; i < members_.size(); ++i)
        worst = Alerts::maxLevel(worst, members_[i].level);
    return worst;
}

RegionalZone operator+(const SensorStation& a, const SensorStation& b) {
    RegionalZone z(a.zone() == b.zone() ? a.zone() : a.zone() + "+" + b.zone());
    z.add(a);
    z.add(b);
    return z;
}

RegionalZone operator+(const RegionalZone& zone, const SensorStation& s) {
    RegionalZone result(zone);
    result.add(s);
    return result;
}

RegionalZone operator+(const RegionalZone& a, const RegionalZone& b) {
    RegionalZone result(a.name() == b.name() ? a.name() : a.name() + "+" + b.name());
    result.merge(a);
    result.merge(b);
    return result;
}

std::ostream& operator<<(std::ostream& os, const RegionalZone& z) {
    std::ostringstream num;
    num << std::fixed << std::setprecision(1) << "peak=" << z.peakHazard() << " mean=" << z.meanHazard();
    os << "Zone " << z.name() << " : " << z.size() << " station(s), " << num.str()
       << ", worst=" << Alerts::toString(z.worstLevel());
    return os;
}

} // namespace Sensors
} // namespace DisasterCore
