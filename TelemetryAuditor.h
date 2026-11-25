// ============================================================================
//  TelemetryAuditor.h  -  UNIT I : FRIEND CLASS
//  Declared as 'friend' inside SensorStation and TelemetryBuffer, so it can read
//  their private raw metrics even though no getters exist for them.
// ============================================================================
#ifndef ALF_TELEMETRY_AUDITOR_H
#define ALF_TELEMETRY_AUDITOR_H

#include <ostream>
#include "SensorStation.h"
#include "TelemetryBuffer.h"

namespace DisasterCore {
namespace Sensors {

class TelemetryAuditor {
public:
    static void audit(const TelemetryBuffer& buffer, std::ostream& os);
    static void audit(const SensorStation& station, std::ostream& os);
};

} // namespace Sensors
} // namespace DisasterCore

#endif
