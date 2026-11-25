#include "IncidentLogger.h"
#include <cstring>
#include <ctime>
#include <stdexcept>
#include <type_traits>

namespace DisasterCore {
namespace Alerts {

static_assert(std::is_trivially_copyable<IncidentRecord>::value,
              "IncidentRecord must be trivially copyable for binary I/O");

static void copyText(char* dest, std::size_t size, const std::string& src) {
    std::memset(dest, 0, size);
    std::strncpy(dest, src.c_str(), size - 1);   // always leaves a terminating '\0'
}

IncidentRecord IncidentRecord::make(std::uint32_t id, std::uint32_t sweep, AlertLevel level,
                                    double hazard, const std::string& station,
                                    const std::string& zone, const std::string& message) {
    IncidentRecord r;
    std::memset(&r, 0, sizeof(r));
    r.incidentId = id;
    r.sweepNo = sweep;
    r.timestamp = static_cast<std::int64_t>(std::time(nullptr));
    r.level = static_cast<std::int32_t>(level);
    r.hazard = hazard;
    copyText(r.stationId, sizeof(r.stationId), station);
    copyText(r.zone, sizeof(r.zone), zone);
    copyText(r.message, sizeof(r.message), message);
    return r;
}

static const char kMagic[8] = { 'A', 'L', 'F', 'E', 'W', 'G', '1', '\0' };

IncidentLogger::IncidentLogger(const std::string& path)
    : path_(path), committedCount_(0), pendingCount_(0), committedEnd_(0), inTransaction_(false) {
    file_.open(path_.c_str(), std::ios::in | std::ios::out | std::ios::binary);
    if (!file_.is_open()) {                                  // first run: create the file
        std::ofstream create(path_.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
        if (!create) throw std::runtime_error("IncidentLogger: cannot create " + path_);
        FileHeader h;
        std::memset(&h, 0, sizeof(h));
        std::memcpy(h.magic, kMagic, sizeof(kMagic));
        h.version = 1;
        h.count = 0;
        create.write(reinterpret_cast<const char*>(&h), sizeof(h));
        create.close();
        file_.open(path_.c_str(), std::ios::in | std::ios::out | std::ios::binary);
        if (!file_.is_open()) throw std::runtime_error("IncidentLogger: cannot open " + path_);
    }

    FileHeader h;
    file_.seekg(0, std::ios::beg);
    file_.read(reinterpret_cast<char*>(&h), sizeof(h));
    if (!file_ || std::memcmp(h.magic, kMagic, sizeof(kMagic)) != 0)
        throw std::runtime_error("IncidentLogger: " + path_ + " is not a valid incident log");
    committedCount_ = h.count;

    file_.seekg(0, std::ios::end);                           // sanity-check file length
    const std::streamoff size = file_.tellg();
    if (size < static_cast<std::streamoff>(recordOffset(committedCount_)))
        throw std::runtime_error("IncidentLogger: " + path_ + " is truncated");
    file_.clear();
}

IncidentLogger::~IncidentLogger() {
    if (inTransaction_) rollback();
    if (file_.is_open()) file_.close();
}

std::streampos IncidentLogger::recordOffset(std::uint32_t index) const {
    return static_cast<std::streamoff>(sizeof(FileHeader)) +
           static_cast<std::streamoff>(index) * static_cast<std::streamoff>(sizeof(IncidentRecord));
}

void IncidentLogger::writeHeader(std::uint32_t count) {
    FileHeader h;
    std::memset(&h, 0, sizeof(h));
    std::memcpy(h.magic, kMagic, sizeof(kMagic));
    h.version = 1;
    h.count = count;
    file_.seekp(0, std::ios::beg);
    file_.write(reinterpret_cast<const char*>(&h), sizeof(h));
    file_.flush();
    if (!file_) throw std::runtime_error("IncidentLogger: header write failed");
}

void IncidentLogger::beginTransaction() {
    if (inTransaction_) throw std::logic_error("IncidentLogger: transaction already open");
    file_.clear();
    committedEnd_ = recordOffset(committedCount_);
    file_.seekp(committedEnd_);
    file_.seekg(committedEnd_);
    pendingCount_ = 0;
    inTransaction_ = true;
}

void IncidentLogger::append(const IncidentRecord& record) {
    if (!inTransaction_) throw std::logic_error("IncidentLogger: append outside a transaction");
    file_.write(reinterpret_cast<const char*>(&record), sizeof(record));
    if (!file_) throw std::runtime_error("IncidentLogger: write failed");
    ++pendingCount_;
}

void IncidentLogger::commit() {
    if (!inTransaction_) throw std::logic_error("IncidentLogger: commit without transaction");
    file_.flush();
    writeHeader(committedCount_ + pendingCount_);       // header last => atomic visibility
    committedCount_ += pendingCount_;
    pendingCount_ = 0;
    inTransaction_ = false;
}

void IncidentLogger::rollback() {
    if (!inTransaction_) return;
    file_.clear();
    file_.seekp(committedEnd_);                          // rewind the put pointer
    file_.seekg(committedEnd_);                          // and the get pointer
    pendingCount_ = 0;
    inTransaction_ = false;
}

std::vector<IncidentRecord> IncidentLogger::readAll() {
    std::vector<IncidentRecord> records;
    file_.clear();
    file_.flush();
    file_.seekg(recordOffset(0));
    for (std::uint32_t i = 0; i < committedCount_; ++i) {
        IncidentRecord r;
        file_.read(reinterpret_cast<char*>(&r), sizeof(r));
        if (!file_) throw std::runtime_error("IncidentLogger: read failed");
        records.push_back(r);
    }
    file_.clear();
    return records;
}

} // namespace Alerts
} // namespace DisasterCore
