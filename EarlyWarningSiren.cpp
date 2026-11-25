#include "EarlyWarningSiren.h"

namespace DisasterCore {
namespace Alerts {

AlertLevel EarlyWarningSiren::globalLevel_ = AlertLevel::Normal;
int EarlyWarningSiren::sirenCount_ = 0;

EarlyWarningSiren::EarlyWarningSiren(const std::string& id, const std::string& zone)
    : sirenId_(id), zone_(zone), state_(AlertLevel::Normal), activations_(0) {
    ++sirenCount_;
}

EarlyWarningSiren::EarlyWarningSiren(const EarlyWarningSiren& o)
    : sirenId_(o.sirenId_), zone_(o.zone_), state_(o.state_), activations_(o.activations_) {
    ++sirenCount_;
}

EarlyWarningSiren::~EarlyWarningSiren() { --sirenCount_; }

void EarlyWarningSiren::activate(AlertLevel level) {
    if (level != state_) {
        if (level > AlertLevel::Normal) ++activations_;
        state_ = level;
    }
}

void EarlyWarningSiren::silence() { state_ = AlertLevel::Normal; }

std::ostream& operator<<(std::ostream& os, const EarlyWarningSiren& s) {
    return os << "Siren " << s.sirenId_ << " @" << s.zone_ << " state=" << toString(s.state_)
              << " activations=" << s.activations_;
}

} // namespace Alerts
} // namespace DisasterCore
