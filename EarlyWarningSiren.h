// ============================================================================
//  EarlyWarningSiren.h  -  Namespace DisasterCore::Alerts
//  UNIT II : static data member + static member functions hold the
//            network-wide EMERGENCY ALERT LEVEL shared by every siren.
// ============================================================================
#ifndef ALF_EARLY_WARNING_SIREN_H
#define ALF_EARLY_WARNING_SIREN_H

#include <ostream>
#include <string>
#include "AlertLevel.h"

namespace DisasterCore {
namespace Alerts {

class EarlyWarningSiren {
    std::string sirenId_;
    std::string zone_;
    AlertLevel state_;
    unsigned activations_;

    static AlertLevel globalLevel_;   // one value shared by ALL sirens
    static int sirenCount_;

public:
    EarlyWarningSiren(const std::string& id, const std::string& zone);
    EarlyWarningSiren(const EarlyWarningSiren& other);
    EarlyWarningSiren& operator=(const EarlyWarningSiren& other) = default;
    ~EarlyWarningSiren();

    void activate(AlertLevel level);
    void silence();

    const std::string& id() const { return sirenId_; }
    const std::string& zone() const { return zone_; }
    AlertLevel state() const { return state_; }
    unsigned activations() const { return activations_; }

    static AlertLevel globalAlertLevel() { return globalLevel_; }
    static void setGlobalAlertLevel(AlertLevel level) { globalLevel_ = level; }
    static int sirenCount() { return sirenCount_; }

    friend std::ostream& operator<<(std::ostream& os, const EarlyWarningSiren& s);
};

} // namespace Alerts
} // namespace DisasterCore

#endif
