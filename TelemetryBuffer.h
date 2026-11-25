// ============================================================================
//  TelemetryBuffer.h  -  UNIT II : dynamic memory management
//  Fixed-capacity ring buffer allocated on the heap.
//    * parameterized / dynamic constructors   (allocate with new[])
//    * deep copy constructor + copy assignment (no shared pointers)
//    * explicit destructor                     (delete[] -> no leaks)
// ============================================================================
#ifndef ALF_TELEMETRY_BUFFER_H
#define ALF_TELEMETRY_BUFFER_H

#include <cstddef>
#include "TelemetryTypes.h"

namespace DisasterCore {
namespace Sensors {

class TelemetryBuffer {
    friend class TelemetryAuditor;   // UNIT I: friend class reads raw internals

    TelemetryReading* data_;   // heap array of 'capacity_' readings
    std::size_t capacity_;
    std::size_t count_;        // number of valid readings
    std::size_t head_;         // index of the next write position

public:
    TelemetryBuffer();                                           // default capacity 8
    explicit TelemetryBuffer(std::size_t capacity);              // parameterized (dynamic) ctor
    TelemetryBuffer(const TelemetryReading* src, std::size_t n); // dynamic ctor from an array
    TelemetryBuffer(const TelemetryBuffer& other);               // deep copy ctor
    TelemetryBuffer& operator=(const TelemetryBuffer& other);    // deep copy assignment
    ~TelemetryBuffer();                                          // explicit destructor

    void push(const TelemetryReading& reading);   // overwrites the oldest when full
    std::size_t size() const     { return count_; }
    std::size_t capacity() const { return capacity_; }
    bool empty() const           { return count_ == 0; }

    // Return-by-reference element access (index 0 = oldest). Throws std::out_of_range.
    TelemetryReading&       at(std::size_t index);
    const TelemetryReading& at(std::size_t index) const;
    const TelemetryReading& latest() const;

    TelemetryReading average() const;
};

} // namespace Sensors
} // namespace DisasterCore

#endif
