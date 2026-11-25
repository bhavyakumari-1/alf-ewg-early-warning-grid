// ============================================================================
//  PowerSubsystem.h  -  UNIT III : independent class used for CONTAINERSHIP
//  A BaseSensorNode HAS-A PowerSubsystem (composition), it IS-NOT one.
// ============================================================================
#ifndef ALF_POWER_SUBSYSTEM_H
#define ALF_POWER_SUBSYSTEM_H

namespace DisasterCore {
namespace Sensors {

class PowerSubsystem {
    double batteryPercent_;
    double panelWatts_;
    static double clampPercent(double v) { return v < 0.0 ? 0.0 : (v > 100.0 ? 100.0 : v); }
public:
    // Below this level the node browns out (value used inline: C++11 safe).
    static double cutoffPercent() { return 5.0; }

    explicit PowerSubsystem(double batteryPercent = 100.0, double panelWatts = 0.0)
        : batteryPercent_(clampPercent(batteryPercent)), panelWatts_(panelWatts) {}

    void consume(double percent) { batteryPercent_ = clampPercent(batteryPercent_ - percent); }
    void charge(double percent)  { batteryPercent_ = clampPercent(batteryPercent_ + percent); }
    bool isDepleted() const      { return batteryPercent_ <= cutoffPercent(); }
    double batteryPercent() const { return batteryPercent_; }
    double panelWatts() const     { return panelWatts_; }
};

} // namespace Sensors
} // namespace DisasterCore

#endif
