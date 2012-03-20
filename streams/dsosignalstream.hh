/* A specialization of SignalStream for the DSO Quad. Stores 4 channels
 * using an efficient binary encoding.
 * 
 * Each event is encoded as a single variable length integer, using the
 * Google Protocol Buffers base-128 varint format:
 * http://code.google.com/apis/protocolbuffers/docs/encoding.html#varints
 * 
 * The lowest 4 bits of the integer are the signal levels. The upper bits,
 * up to 60 bits, are the number of ticks how long the levels remained.
 * 
 * When updating in the real time, the latest levels are kept separately
 * for faster updating. The meaning of last_duration and last_value are
 * same as in varint-encoded values. If last_duration is 0, there is no
 * real time event to take into account.
 */

#pragma once
#include "signalstream.hh"

// This is the structure for the low-level buffer used to store the data.
// The structure is updated from an interrupt, and can be read
// simultaneously by several SignalStreams.
struct signal_buffer_t
{
    // Number of bytes in the buffer
    volatile size_t bytes;
    
    // Length and value of the current level of the signals
    // Updated from interrupts, be slightly careful when reading.
    volatile signaltime_t last_duration;
    volatile signals_t last_value;
    
    // Storage for the time-deltas and levels.
    // Not marked volatile because the valid bytes never change after initial
    // write.
    uint8_t storage[25000];
};

class DSOSignalStream: public SignalStream {
public:
    DSOSignalStream(const signal_buffer_t *buffer);
    virtual ~DSOSignalStream() {};
    
    virtual void seek(signaltime_t time);
    
    // Reads next signal event from the buffer. Returns false if there are
    // no more events.
    virtual bool read_forwards(SignalEvent &result);
    
    // Reads the previous event, ie. the one before next read position.
    // E.g. if called right after read_forwards(), this returns the same
    // event.
    virtual bool read_backwards(SignalEvent &result);
    
    virtual DSOSignalStream* clone() const;
    
    static const int frequency = 500000;
    
private:
    size_t read_pos; // Next position to be read
    SignalEvent previous_event; // Event immediately before read_pos (old_levels is not valid)
    bool previous_was_last; // Previous event was read from last_duration
    const signal_buffer_t *buffer;
};

