#include "dsosignalstream.hh"

DSOSignalStream::DSOSignalStream(const signal_buffer_t *buffer):
    read_pos(0), previous_event(), previous_was_last(false), buffer(buffer)
{
}

void DSOSignalStream::seek(signaltime_t time)
{
    SignalEvent dummy;
    
    if (time == 0)
    {
        read_pos = 0;
        previous_event = SignalEvent();
        previous_was_last = false;
        return;
    }
    
    if (time < previous_event.start)
    {
        // Estimate how long we would have to seek, so whether it is better
        // to seek from start.
        size_t bytes_to_seek = time * read_pos / previous_event.end;
        if (bytes_to_seek < read_pos - bytes_to_seek)
        {
            read_pos = 0;
            previous_event = SignalEvent();
            previous_was_last = false;
        }
    }
    
    if (previous_was_last)
        read_backwards(dummy);
    
    while (previous_event.end < time && read_forwards(dummy));
    
    while (previous_event.end > time && read_backwards(dummy));
    
    if (previous_event.end > time || previous_was_last)
    {
        read_pos = 0;
        previous_event = SignalEvent();
        previous_was_last = false;
    }
}

bool DSOSignalStream::read_forwards(SignalEvent &result)
{
    uint64_t value = 0;
    uint8_t bitpos = 0;
    uint8_t byte;
    
    if (previous_was_last)
    {
        // We need a seek() to get out of this state.
        return false;
    }
    
    if (read_pos >= buffer->bytes)
    {
        // Read from last_duration and last_value. Detect conflicting writes
        // by re-checking buffer->bytes later.
        result.start = previous_event.end;
        result.end = result.start + buffer->last_duration;
        result.levels = buffer->last_value;
        result.old_levels = previous_event.levels;
        
        if (result.start < result.end)
            previous_was_last = true;
    }
    
    if (read_pos < buffer->bytes)
    {
        // Read from encoded storage
        do {
            byte = buffer->storage[read_pos];
            value |= (uint64_t)(byte & 0x7F) << bitpos;
            read_pos++;
            bitpos += 7;
        } while (byte & 0x80);
        
        result.start = previous_event.end;
        result.end = result.start + (value >> 4);
        result.old_levels = previous_event.levels;
        result.levels = value & 0x0F;
        
        previous_was_last = false;
    }
    
    previous_event = result;
    
    // If the last event is 0-length, return false
    return result.start < result.end;
}

bool DSOSignalStream::read_backwards(SignalEvent &result)
{
    uint8_t byte;
    
    if (read_pos <= 0)
        return false;
    
    if (!previous_was_last)
    {
        // Seek to the previous event
        do {
            read_pos--;
        } while (buffer->storage[read_pos - 1] & 0x80);
    }
    
    // And read the event before that
    uint64_t value = 0;
    size_t pos = read_pos;
    do {
        pos--;
        byte = buffer->storage[pos];
        value <<= 7;
        value |= (uint64_t)(byte & 0x7F);
    } while (pos != 0 && buffer->storage[pos - 1] & 0x80);
    
    result = previous_event;
    previous_event.end = result.start;
    previous_event.start = result.start - (value >> 4);
    previous_event.levels = result.old_levels = value & 0x0F;
    
    // Note: the old_levels will not be valid, but it is not used anywhere.
    previous_event.old_levels = -1;
    
    previous_was_last = false;
    
    return true;
}

DSOSignalStream* DSOSignalStream::clone() const
{
    return new DSOSignalStream(*this);
}

