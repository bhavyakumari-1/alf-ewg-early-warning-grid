// ============================================================================
//  IncidentLogger.h  -  UNIT IV : binary persistence with transactional rollback
//
//  File layout (little endian, written with std::fstream in binary mode):
//      [FileHeader : magic, version, committedCount]
//      [IncidentRecord #0][IncidentRecord #1] ...
//
//  Transaction protocol
//      beginTransaction : remember the committed end-of-data offset (seekp)
//      append           : write a record after that offset
//      commit           : seekp(0) and rewrite the header count LAST -> the new
//                         records become visible atomically
//      rollback         : seekp/seekg back to the remembered offset; the header
//                         count is untouched, so the half-written tail is
//                         invisible to readers and overwritten by the next txn
// ============================================================================
#ifndef ALF_INCIDENT_LOGGER_H
#define ALF_INCIDENT_LOGGER_H

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>
#include "AlertLevel.h"

namespace DisasterCore {
namespace Alerts {

// Fixed-size trivially-copyable record -> safe to write with ostream::write().
struct IncidentRecord {
    std::uint32_t incidentId;
    std::uint32_t sweepNo;
    std::int64_t  timestamp;
    std::int32_t  level;
    double        hazard;
    char          stationId[16];
    char          zone[24];
    char          message[64];

    static IncidentRecord make(std::uint32_t id, std::uint32_t sweep, AlertLevel level, double hazard,
                               const std::string& station, const std::string& zone,
                               const std::string& message);
};

class IncidentLogger {
    struct FileHeader {
        char magic[8];
        std::uint32_t version;
        std::uint32_t count;
    };

    std::string path_;
    std::fstream file_;
    std::uint32_t committedCount_;
    std::uint32_t pendingCount_;
    std::streampos committedEnd_;
    bool inTransaction_;

    std::streampos recordOffset(std::uint32_t index) const;
    void writeHeader(std::uint32_t count);

public:
    explicit IncidentLogger(const std::string& path);   // opens or creates the file
    ~IncidentLogger();                                  // rolls back an open txn
    IncidentLogger(const IncidentLogger&) = delete;
    IncidentLogger& operator=(const IncidentLogger&) = delete;

    void beginTransaction();
    void append(const IncidentRecord& record);
    void commit();
    void rollback();

    std::vector<IncidentRecord> readAll();              // uses seekg to scan records
    std::uint32_t committedCount() const { return committedCount_; }
    std::uint32_t pendingCount() const { return pendingCount_; }
    bool inTransaction() const { return inTransaction_; }
    const std::string& path() const { return path_; }
};

} // namespace Alerts
} // namespace DisasterCore

#endif
