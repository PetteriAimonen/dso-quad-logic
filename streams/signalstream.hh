/* A stream class for the lowest level of the events; the signals just as
 * they are read in. Handles the efficient memory encoding of the transitions.
 */

#pragma once

#include <stdint.h>
#include <cstring> // for size_t
#include "eventstream.hh"

// The signal levels are stored as a bitfield
// Width of this field could be increased if necessary
typedef uint16_t signals_t;

typedef uint32_t frequency_t;

struct SignalEvent: public Event
{
    // Values before the transition. Same as the .levels in previous event,
    // included here for convenience of signal parsers.
    signals_t old_levels;
    
    // New values of the signals, bit 0 = ch A etc.
    // The signals have these levels for duration begin <= time < end
    signals_t levels;
    
    virtual void to_string(char *buf, size_t size) const
    {
        if (size >= 5)
        {
            buf[0] = (levels & 8) ? '1' : '0';
            buf[1] = (levels & 4) ? '1' : '0';
            buf[2] = (levels & 2) ? '1' : '0';
            buf[3] = (levels & 1) ? '1' : '0';
            buf[4] = 0;
        }
    }
};

class SignalStream: public EventStream {
public:
    virtual ~SignalStream() {};
    
    // Seek to event that begins at or before time
    virtual void seek(signaltime_t time) = 0;
    
    // This is the generic EventStream interface.
    // If you don't need the generality, you may use read_forwards to 
    // avoid a memory allocation.
    virtual SignalEvent* read()
    {
        SignalEvent *result = new SignalEvent;
        
        if (read_forwards(*result))
        {
            return result;
        }
        else
        {
            delete result;
            return NULL;
        }
    }
    
    // Reads next signal event from the buffer. Returns false if there are
    // no more events.
    virtual bool read_forwards(SignalEvent &result) = 0;
    
    // Reads the previous event, ie. the one before next read position.
    // E.g. if called right after read_forwards(), this returns the same
    // event.
    virtual bool read_backwards(SignalEvent &result) = 0;
    
    // Get the tick frequency (ticks per second) of the stream
//     virtual frequency_t get_frequency() const;
    
    virtual SignalStream* clone() const = 0;
};
