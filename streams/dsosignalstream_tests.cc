#include "dsosignalstream.hh"
#include "unittests.h"

int main()
{
    int status = 0;
    
    {
        COMMENT("Test basic SignalStream parsing");
        signal_buffer_t buffer = {
            4, 0, 0, {0x12, 0x34, 0x56, 0x88, 0x01}
        };
        DSOSignalStream stream(&buffer);
        std::unique_ptr<SignalEvent> event;
        
        event.reset(stream.read());
        TEST(event && event->start == 0 && event->end == 1
             && event->levels == 2);
        
        event.reset(stream.read());
        TEST(event && event->start == 1 && event->end == 4
             && event->levels == 4 && event->old_levels == 2);
        
        event.reset(stream.read());
        TEST(event && event->start == 4 && event->end == 9
             && event->levels == 6 && event->old_levels == 4);
        
        event.reset(stream.read());
        TEST(event && event->start == 9 && event->end == 17
             && event->levels == 8 && event->old_levels == 6);
        
        event.reset(stream.read());
        TEST(!event);
        
        COMMENT("Test seek to start");
        stream.seek(0);
        event.reset(stream.read());
        TEST(event && event->start == 0 && event->end == 1
             && event->levels == 2);
        
        COMMENT("Test seek past the buffer");
        stream.seek(99);
        event.reset(stream.read());
        TEST(!event);
        
        COMMENT("Test seek 1 event backwards");
        stream.seek(9);
        event.reset(stream.read());
        TEST(event && event->start == 9 && event->end == 17
             && event->levels == 8 && event->old_levels == 6);
    }
    
    {
        COMMENT("Test reading the last event from otherwise empty buffer");
        signal_buffer_t buffer = {
            0, 9, 10, {}
        };
        DSOSignalStream stream(&buffer);
        std::unique_ptr<SignalEvent> event(stream.read());
        
        TEST(event && event->start == 0 && event->end == 9 && event->levels == 10);
    }
    
    {
        COMMENT("Test seeking after reading the last event");
        signal_buffer_t buffer = {
            4, 5, 5, {0x11, 0x22, 0x33, 0x44}
        };
        DSOSignalStream stream(&buffer);
        std::unique_ptr<SignalEvent> event;
        
        stream.seek(3);
        event.reset(stream.read());
        TEST(event && event->start == 3 && event->end == 6
            && event->old_levels == 2 && event->levels == 3);
        
        event.reset(stream.read());
        TEST(event && event->start == 6 && event->end == 10
            && event->old_levels == 3 && event->levels == 4);
        
        event.reset(stream.read());
        TEST(event && event->start == 10 && event->end == 15
            && event->old_levels == 4 && event->levels == 5);
        
        event.reset(stream.read());
        TEST(!event);
        
        stream.seek(12);
        event.reset(stream.read());
        TEST(event && event->start == 10 && event->end == 15
            && event->old_levels == 4 && event->levels == 5);
        
        stream.seek(9);
        event.reset(stream.read());
        TEST(event && event->start == 6 && event->end == 10
            && event->old_levels == 3 && event->levels == 4);
    }
    
    {
        COMMENT("Test incremental reads");
        signal_buffer_t buffer = {
            0, 5, 5, {0x55, 0x22, 0x33, 0x44}
        };
        DSOSignalStream stream(&buffer);
        SignalEvent event;
        
        TEST(stream.read_forwards(event) &&
            event.start == 0 && event.end == 5 && event.levels == 5);
        
        TEST(!stream.read_forwards(event));
        
        buffer.bytes = 2;
        stream.seek(0);
        
        TEST(stream.read_forwards(event) &&
            event.start == 0 && event.end == 5 && event.levels == 5);
        TEST(stream.read_forwards(event) &&
            event.start == 5 && event.end == 7 && event.levels == 2);
    }
    
    return status;
}
