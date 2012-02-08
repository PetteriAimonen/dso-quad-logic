#include "signalgraph.hh"

SignalGraph::SignalGraph(const SignalStream &stream, const XPosHandler *xpos, int channel):
y0(0), height(16), color(0xFFFF), offset(0),
stream(stream.clone()), channel_mask(1 << channel),
current_event(), xpos(xpos)
{
}

void SignalGraph::Prepare(int xstart, int xend)
{
    signaltime_t start = xpos->get_time(xstart) + offset;
    stream->seek(start);
    stream->read_forwards(current_event);
}

void SignalGraph::Draw(uint16_t buffer[], int screenheight, int x)
{
    signaltime_t time = xpos->get_time(x) + offset;
    
    signals_t positive = 0, negative = 0;
    
    do {
        positive |= current_event.levels;
        negative |= ~current_event.levels;
    } while (current_event.end <= time && stream->read_forwards(current_event));
    
    if (current_event.end <= time)
        return; // End of stream
    
    positive &= channel_mask;
    negative &= channel_mask;
    
    if (positive && negative)
    {
        // Vertical edge
        for (int y = y0; y < y0 + height; y++)
        {
            buffer[y] = color;
        }
    }
    else if (positive)
    {
        buffer[y0 + height - 1] = color;
    }
    else if (negative)
    {
        buffer[y0] = color;
    }
}