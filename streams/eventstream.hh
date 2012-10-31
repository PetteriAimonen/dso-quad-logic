#pragma once

#include <stdint.h>
#include <cstring>
#include <memory>

typedef int64_t signaltime_t;

struct Event
{
    virtual ~Event() {}

    signaltime_t start; // Absolute time in ticks for the beginning of the event
    signaltime_t end; // Absolute time for end of the event (same as begin if the event doesn't have a length)
    
    // Format a description of the event to a string.
    virtual void to_string(char *buf, size_t size) const
    {
        *buf = 0;
    }
};

class EventStream
{
public:
    virtual ~EventStream() {};
    
    // Seek to a position so that the next call to read() returns an event
    // that begins either before or at the given time.
    // I.e. post condition: stream.read()->begin <= time.
    virtual void seek(signaltime_t time) = 0;
    
    // Read the next event from the stream.
    // Returns NULL when there are no more events.
    // Caller is responsible for freeing the memory; use unique_ptr.
    virtual Event* read() = 0;
    
    // Make a deep clone of this object, ie. it should make new copies
    // of all non-const objects.
    virtual EventStream* clone() const = 0;
};

