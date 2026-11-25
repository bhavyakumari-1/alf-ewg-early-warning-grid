// ============================================================================
//  Functors.h  -  UNIT IV : custom function objects used with STL algorithms
// ============================================================================
#ifndef ALF_FUNCTORS_H
#define ALF_FUNCTORS_H

#include <map>
#include <string>
#include "AlertLevel.h"
#include "EarlyWarningSiren.h"
#include "SensorStation.h"

namespace DisasterCore {
namespace Alerts {

// One entry of the evacuation plan for a zone.
struct EvacuationOrder {
    std::string zone;
    AlertLevel level;
    double hazard;
    unsigned population;
    std::string route;

    // Larger = more urgent: higher hazard acting on more people.
    double priorityScore() const { return hazard * static_cast<double>(population) / 1000.0; }
};

// Priority-based evacuation routing: most severe level first, then by priority score.
// Used as the comparator of std::sort.
struct EvacuationPriority {
    bool operator()(const EvacuationOrder& a, const EvacuationOrder& b) const {
        if (a.level != b.level) return a.level > b.level;
        return a.priorityScore() > b.priorityScore();
    }
};

// Sorts stations by descending hazard using the overloaded SensorStation::operator>.
struct HazardDescending {
    bool operator()(const Sensors::SensorStation& a, const Sensors::SensorStation& b) const {
        return a > b;
    }
};

// Stateful functor passed to std::for_each: triggers each siren according to the
// alert level computed for its zone, and counts how many sirens changed state.
class SirenDispatcher {
    std::map<std::string, AlertLevel> zoneLevels_;
    unsigned changed_;
public:
    explicit SirenDispatcher(const std::map<std::string, AlertLevel>& zoneLevels)
        : zoneLevels_(zoneLevels), changed_(0) {}

    void operator()(EarlyWarningSiren& siren) {
        std::map<std::string, AlertLevel>::const_iterator it = zoneLevels_.find(siren.zone());
        const AlertLevel target = (it == zoneLevels_.end()) ? AlertLevel::Normal : it->second;
        if (siren.state() != target) {
            siren.activate(target);
            ++changed_;
        }
    }
    unsigned changedCount() const { return changed_; }
};

} // namespace Alerts
} // namespace DisasterCore

#endif
