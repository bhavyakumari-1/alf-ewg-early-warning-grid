// ============================================================================
//  Exceptions.h  -  UNIT IV : custom exception hierarchy
//  Every class derives from std::exception and supplies its own what().
// ============================================================================
#ifndef ALF_EXCEPTIONS_H
#define ALF_EXCEPTIONS_H

#include <cstdint>
#include <exception>
#include <iomanip>
#include <sstream>
#include <string>

namespace DisasterCore {

// A node cannot deliver data (dead transducer, depleted battery, radio loss).
class SensorFailureException : public std::exception {
    std::string nodeId_;
    std::string reason_;
    std::string message_;
public:
    SensorFailureException(const std::string& nodeId, const std::string& reason)
        : nodeId_(nodeId), reason_(reason),
          message_("[SensorFailureException] node " + nodeId + ": " + reason) {}
    virtual ~SensorFailureException() noexcept {}
    const char* what() const noexcept override { return message_.c_str(); }
    const std::string& nodeId() const { return nodeId_; }
    const std::string& reason() const { return reason_; }
};

// The packet checksum does not match its payload (corruption in transit).
class TelemetryChecksumMismatch : public std::exception {
    std::string nodeId_;
    std::uint32_t expected_;
    std::uint32_t actual_;
    std::string message_;
    static std::string build(const std::string& id, std::uint32_t e, std::uint32_t a) {
        std::ostringstream os;
        os << "[TelemetryChecksumMismatch] node " << id << ": expected 0x"
           << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << e
           << " but packet carries 0x" << std::setw(8) << std::setfill('0') << a;
        return os.str();
    }
public:
    TelemetryChecksumMismatch(const std::string& nodeId, std::uint32_t expected, std::uint32_t actual)
        : nodeId_(nodeId), expected_(expected), actual_(actual),
          message_(build(nodeId, expected, actual)) {}
    virtual ~TelemetryChecksumMismatch() noexcept {}
    const char* what() const noexcept override { return message_.c_str(); }
    const std::string& nodeId() const { return nodeId_; }
    std::uint32_t expected() const { return expected_; }
    std::uint32_t actual() const { return actual_; }
};

// A measured value lies outside the physically possible range of the transducer.
class ThresholdOverflowException : public std::exception {
    std::string nodeId_;
    std::string parameter_;
    double value_;
    double limit_;
    std::string message_;
    static std::string build(const std::string& id, const std::string& p, double v, double l) {
        std::ostringstream os;
        os << "[ThresholdOverflowException] node " << id << ": " << p << " = "
           << std::fixed << std::setprecision(1) << v << " exceeds physical limit " << l;
        return os.str();
    }
public:
    ThresholdOverflowException(const std::string& nodeId, const std::string& parameter,
                               double value, double limit)
        : nodeId_(nodeId), parameter_(parameter), value_(value), limit_(limit),
          message_(build(nodeId, parameter, value, limit)) {}
    virtual ~ThresholdOverflowException() noexcept {}
    const char* what() const noexcept override { return message_.c_str(); }
    const std::string& nodeId() const { return nodeId_; }
    const std::string& parameter() const { return parameter_; }
    double value() const { return value_; }
    double limit() const { return limit_; }
};

} // namespace DisasterCore

#endif
