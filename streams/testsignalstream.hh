/* SignalStream implementation for testing purposes.
 * Takes a constant string and outputs that as a signal.
 */

#pragma once

#include "signalstream.hh"

class TestSignalStream: public SignalStream
{
public:   
    
    // The signals should be strings of '_' and any other characters, e.g.
    // "_-_--__" = 0101100. The stream has the length of the longest string.
    TestSignalStream(
        const char *signal1,
        const char *signal2,
        const char *signal3,
        const char *signal4
    )
    {
        this->signal1 = signal1;
        this->signal2 = signal2;
        this->signal3 = signal3;
        this->signal4 = signal4;
        this->time = 0;
        
        length1 = strlen(signal1);
        length2 = strlen(signal2);
        length3 = strlen(signal3);
        length4 = strlen(signal4);
        
        max_time = length1;
        if (length2 > max_time) max_time = length2;
        if (length3 > max_time) max_time = length3;
        if (length4 > max_time) max_time = length4;
    }
    
    virtual void seek(signaltime_t time)
    {
        this->time = time;
    }
    
    virtual bool read_forwards(SignalEvent &result)
    {
        if (time >= max_time)
            return false;
        
        result.start = time;
        result.old_levels = get_signals(time - 1);
        result.levels = get_signals(time);
        
        do {
            time++;
        } while (time <= max_time && get_signals(time) == result.levels);
        
        result.end = time;
        
        return true;
    }
    
    virtual bool read_backwards(SignalEvent &result)
    {
        if (time <= 0)
            return false;
        
        time--;
        
        result.end = time + 1;
        result.levels = get_signals(time);
        
        while (time > 0 && get_signals(time - 1) == result.levels)
            time--;
        
        result.old_levels = get_signals(time - 1);
        result.start = time;
        
        return true;
    }
    
    virtual TestSignalStream* clone() const
    {
        return new TestSignalStream(*this);
    }
    
private:
    const char *signal1;
    size_t length1;
    const char *signal2;
    size_t length2;
    const char *signal3;
    size_t length3;
    const char *signal4;
    size_t length4;
    
    size_t max_time;
    signaltime_t time;
    
    signals_t get_signals(signaltime_t time)
    {
        signals_t result = 0;
        
        if (time < 0)
            return 0;
        
        if (time < length1 && signal1[time] != '_')
            result |= 1;
        
        if (time < length2 && signal2[time] != '_')
            result |= 2;
        
        if (time < length3 && signal3[time] != '_')
            result |= 4;
        
        if (time < length4 && signal4[time] != '_')
            result |= 8;
        
        return result;
    }
};
