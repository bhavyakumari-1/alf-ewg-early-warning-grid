// ============================================================================
//  AlertLevel.h  -  Namespace DisasterCore::Alerts
//  Four-step alert scale shared by sensors, sirens and evacuation planning.
// ============================================================================
#ifndef ALF_ALERT_LEVEL_H
#define ALF_ALERT_LEVEL_H

namespace DisasterCore {
namespace Alerts {

enum class AlertLevel : int { Normal = 0, Watch = 1, Warning = 2, Evacuate = 3 };

inline const char* toString(AlertLevel level) {
    switch (level) {
        case AlertLevel::Normal:   return "NORMAL";
        case AlertLevel::Watch:    return "WATCH";
        case AlertLevel::Warning:  return "WARNING";
        case AlertLevel::Evacuate: return "EVACUATE";
    }
    return "UNKNOWN";
}

// Returns the more severe of two levels (scoped enums support built-in <, >).
inline AlertLevel maxLevel(AlertLevel a, AlertLevel b) { return (a < b) ? b : a; }

} // namespace Alerts
} // namespace DisasterCore

#endif
