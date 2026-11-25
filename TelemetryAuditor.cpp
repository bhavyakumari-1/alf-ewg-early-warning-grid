#include "TelemetryAuditor.h"
#include <iomanip>
#include <sstream>

namespace DisasterCore {
namespace Sensors {

void TelemetryAuditor::audit(const TelemetryBuffer& b, std::ostream& os) {
    // Direct access to private members: data_, capacity_, count_, head_
    os << "    buffer  : capacity=" << b.capacity_ << " used=" << b.count_
       << " nextWriteSlot=" << b.head_ << " heapBlock=" << sizeof(TelemetryReading) * b.capacity_
       << " bytes\n";
}

void TelemetryAuditor::audit(const SensorStation& s, std::ostream& os) {
    std::ostringstream num;
    num << std::fixed << std::setprecision(2) << "rawHazard=" << s.hazardIndex_
        << " peakHazard=" << s.peakHazard_;
    os << "  AUDIT [" << s.id_ << "] " << num.str() << " totalReadings=" << s.totalReadings_
       << " offlineEvents=" << s.offlineEvents_ << " online=" << (s.online_ ? "yes" : "no") << '\n';
    audit(s.buffer_, os);
}

} // namespace Sensors
} // namespace DisasterCore
