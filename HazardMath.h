// ============================================================================
//  HazardMath.h  -  UNIT I : inline functions, function overloading,
//                   call by reference and return by reference
// ============================================================================
#ifndef ALF_HAZARD_MATH_H
#define ALF_HAZARD_MATH_H

#include <cmath>
#include "TelemetryTypes.h"

namespace DisasterCore {
namespace Sensors {

constexpr double kPi = 3.14159265358979323846;

// ---- small inline helpers used on every sensor sweep ----------------------
inline double clamp01(double v) { return v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v); }
inline double normalise(double v, double lo, double hi) { return clamp01((v - lo) / (hi - lo)); }

// ---- parameter bundles that make the overloads unambiguous ----------------
struct RainfallSlope  { double rainfallRate;  double slopeAngle; };
struct SoilVibration  { double soilSaturation; double vibrationHz; };
struct RiverRain      { double riverLevel;    double rainfallRate; };

// ---- OVERLOADED computeHazardIndex(): returns a 0..100 risk score ----------
// 1) rainfall rate + slope angle        (tiltmeter / rain-gauge nodes)
inline double computeHazardIndex(const RainfallSlope& rs) {
    return 100.0 * (0.65 * normalise(rs.rainfallRate, 0.0, 120.0) +
                    0.35 * normalise(rs.slopeAngle,   0.0, 55.0));
}
// 2) soil saturation + vibration frequency (soil-moisture / geophone nodes)
inline double computeHazardIndex(const SoilVibration& sv) {
    return 100.0 * (0.60 * normalise(sv.soilSaturation, 0.0, 100.0) +
                    0.40 * normalise(sv.vibrationHz,    0.0, 25.0));
}
// 3) river level + rainfall rate (river sonar gauges -> flash-flood risk)
inline double computeHazardIndex(const RiverRain& rr) {
    return 100.0 * (0.70 * normalise(rr.riverLevel,    0.0, 8.0) +
                    0.30 * normalise(rr.rainfallRate,  0.0, 120.0));
}
// 4) full composite of both families plus river level (hybrid probes)
inline double computeHazardIndex(const RainfallSlope& rs, const SoilVibration& sv, double riverLevel) {
    return 0.40 * computeHazardIndex(rs) +
           0.40 * computeHazardIndex(sv) +
           0.20 * 100.0 * normalise(riverLevel, 0.0, 8.0);
}
// 5) convenience overload working on a complete telemetry packet
inline double computeHazardIndex(const TelemetryReading& r) {
    const RainfallSlope rs = { r.rainfallRate, r.slopeAngle };
    const SoilVibration sv = { r.soilSaturation, r.vibrationHz };
    return computeHazardIndex(rs, sv, r.riverLevel);
}

// ---- CALL BY REFERENCE + RETURN BY REFERENCE -------------------------------
// The coordinate is modified in place (no copy) and the same object is returned
// so that transformations can be chained:  scale(toLocalGridKm(p, origin), 1000)
inline GeoCoordinate& toLocalGridKm(GeoCoordinate& point, const GeoCoordinate& origin) {
    const double kmPerDegree = 111.32;
    point.latitude  = (point.latitude  - origin.latitude) * kmPerDegree;
    point.longitude = (point.longitude - origin.longitude) * kmPerDegree *
                      std::cos(origin.latitude * kPi / 180.0);
    point.elevation -= origin.elevation;
    return point;
}

inline GeoCoordinate& scaleCoordinate(GeoCoordinate& point, double factor) {
    point.latitude *= factor;
    point.longitude *= factor;
    point.elevation *= factor;
    return point;
}

// Telemetry state accumulation: adds 'sample' into 'total' and returns 'total'.
inline TelemetryReading& accumulate(TelemetryReading& total, const TelemetryReading& sample) {
    total.rainfallRate   += sample.rainfallRate;
    total.slopeAngle     += sample.slopeAngle;
    total.soilSaturation += sample.soilSaturation;
    total.vibrationHz    += sample.vibrationHz;
    total.riverLevel     += sample.riverLevel;
    return total;
}

} // namespace Sensors
} // namespace DisasterCore

#endif
