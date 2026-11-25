#include "TelemetryBuffer.h"
#include "HazardMath.h"
#include <algorithm>
#include <stdexcept>

namespace DisasterCore {
namespace Sensors {

TelemetryBuffer::TelemetryBuffer() : data_(nullptr), capacity_(8), count_(0), head_(0) {
    data_ = new TelemetryReading[capacity_];
}

TelemetryBuffer::TelemetryBuffer(std::size_t capacity)
    : data_(nullptr), capacity_(capacity ? capacity : 1), count_(0), head_(0) {
    data_ = new TelemetryReading[capacity_];
}

TelemetryBuffer::TelemetryBuffer(const TelemetryReading* src, std::size_t n)
    : data_(nullptr), capacity_(n ? n : 1), count_(n), head_(0) {
    data_ = new TelemetryReading[capacity_];
    for (std::size_t i = 0; i < n; ++i) data_[i] = src[i];
    head_ = n % capacity_;
}

// Deep copy: the new object owns its OWN array, so mutating one never affects the other.
TelemetryBuffer::TelemetryBuffer(const TelemetryBuffer& other)
    : data_(new TelemetryReading[other.capacity_]),
      capacity_(other.capacity_), count_(other.count_), head_(other.head_) {
    std::copy(other.data_, other.data_ + other.capacity_, data_);
}

TelemetryBuffer& TelemetryBuffer::operator=(const TelemetryBuffer& other) {
    if (this != &other) {                                   // self-assignment guard
        TelemetryReading* fresh = new TelemetryReading[other.capacity_];  // allocate first:
        std::copy(other.data_, other.data_ + other.capacity_, fresh);     // if new throws, *this is intact
        delete[] data_;
        data_ = fresh;
        capacity_ = other.capacity_;
        count_ = other.count_;
        head_ = other.head_;
    }
    return *this;
}

TelemetryBuffer::~TelemetryBuffer() { delete[] data_; }

void TelemetryBuffer::push(const TelemetryReading& reading) {
    data_[head_] = reading;
    head_ = (head_ + 1) % capacity_;
    if (count_ < capacity_) ++count_;
}

const TelemetryReading& TelemetryBuffer::at(std::size_t index) const {
    if (index >= count_) throw std::out_of_range("TelemetryBuffer::at - index out of range");
    return data_[(head_ + capacity_ - count_ + index) % capacity_];
}

TelemetryReading& TelemetryBuffer::at(std::size_t index) {
    return const_cast<TelemetryReading&>(static_cast<const TelemetryBuffer&>(*this).at(index));
}

const TelemetryReading& TelemetryBuffer::latest() const {
    if (count_ == 0) throw std::out_of_range("TelemetryBuffer::latest - buffer is empty");
    return at(count_ - 1);
}

TelemetryReading TelemetryBuffer::average() const {
    TelemetryReading total;
    if (count_ == 0) return total;
    for (std::size_t i = 0; i < count_; ++i) accumulate(total, at(i));   // reference accumulation
    const double n = static_cast<double>(count_);
    total.rainfallRate /= n;  total.slopeAngle /= n;  total.soilSaturation /= n;
    total.vibrationHz /= n;   total.riverLevel /= n;
    return total;
}

} // namespace Sensors
} // namespace DisasterCore
