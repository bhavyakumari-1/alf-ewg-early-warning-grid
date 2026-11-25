// ============================================================================
//  TelemetryTypes.h  -  Namespace DisasterCore::Sensors
//  Plain data carriers: GeoCoordinate, TelemetryReading and the checksum.
// ============================================================================
#ifndef ALF_TELEMETRY_TYPES_H
#define ALF_TELEMETRY_TYPES_H

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace DisasterCore {
namespace Sensors {

struct GeoCoordinate {
    double latitude;    // degrees (or km north of origin after local-grid transform)
    double longitude;   // degrees (or km east of origin after local-grid transform)
    double elevation;   // metres
    GeoCoordinate(double lat = 0.0, double lon = 0.0, double elev = 0.0)
        : latitude(lat), longitude(lon), elevation(elev) {}
};

struct TelemetryReading {
    double rainfallRate;     // mm / hour
    double slopeAngle;       // degrees
    double soilSaturation;   // percent
    double vibrationHz;      // seismic dominant frequency
    double riverLevel;       // metres above gauge zero
    unsigned sweepNo;        // sweep in which the sample was taken
    std::uint32_t checksum;  // FNV-1a hash of the payload

    TelemetryReading()
        : rainfallRate(0), slopeAngle(0), soilSaturation(0),
          vibrationHz(0), riverLevel(0), sweepNo(0), checksum(0) {}
};

// FNV-1a over the five measurements and the sweep number.
inline std::uint32_t computeChecksum(const TelemetryReading& r) {
    const double fields[5] = { r.rainfallRate, r.slopeAngle, r.soilSaturation,
                               r.vibrationHz, r.riverLevel };
    std::uint32_t h = 2166136261u;
    for (int i = 0; i < 5; ++i) {
        unsigned char bytes[sizeof(double)];
        std::memcpy(bytes, &fields[i], sizeof(double));
        for (std::size_t b = 0; b < sizeof(double); ++b) {
            h ^= bytes[b];
            h *= 16777619u;
        }
    }
    h ^= r.sweepNo;
    h *= 16777619u;
    return h;
}

inline bool checksumValid(const TelemetryReading& r) { return r.checksum == computeChecksum(r); }

} // namespace Sensors
} // namespace DisasterCore

#endif
