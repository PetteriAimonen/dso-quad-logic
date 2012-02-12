#include "grid.hh"
#include <stdio.h>

Grid::Grid(const SignalStream &stream, const XPosHandler* xpos):
    color(0xFFFF), y0(0), y1(240),
    stream(stream.clone()), xpos(xpos), step(0), offset(0)
{}

void Grid::Prepare(int xstart, int xend)
{
    // Note: should have some nice constant somewhere for the graph screen
    // area. Incidentally, xstart and xend would have the right values but
    // that is not how I intended them to be used. 
    signaltime_t start = xpos->get_time(0);
    signaltime_t end = xpos->get_time(400);
    
    stream->seek(start);
    
    SignalEvent event;
    signaltime_t prev_transitions[4] = {-1, -1, -1, -1};
    signaltime_t shortest = -1;
    signaltime_t sum = 0;
    int count = 0;
    
    // Don't bother finding grid smaller than this, it won't be shown.
    int zoom = xpos->get_zoom();
    signaltime_t threshold = (zoom >= 0) ? (5 >> zoom) : (5 << (-zoom));
    
    // Find the shortest level on the screen, considering each signal
    // separately. Simultaneusly make an average of the shortest event.
    while(stream->read_forwards(event) && event.start < end &&
        (shortest > threshold || shortest == -1))
    {
        for (int i = 0; i < 4; i++)
        {
            signals_t mask = (1 << i);
            if ((event.levels & mask) != (event.old_levels & mask))
            {
                if (prev_transitions[i] != -1)
                {
                    signaltime_t delta = event.start - prev_transitions[i];
                    if (shortest == -1 || shortest > delta)
                    {
                        shortest = delta;
                        sum = delta;
                        count = 1;
                    }
                    else
                    {
                        int multiplier = delta / shortest;
                        if (multiplier < 5)
                        {
                            sum += delta;
                            count += multiplier;
                        }
                    }
                }
                prev_transitions[i] = event.start;
            }
        }
    }
    
    int zoom_adjust = zoom + 16; // fix16_t scaling
    
    if (zoom_adjust < 0)
    {
        step = (sum >> (-zoom_adjust)) / count;
    }
    else
    {
        step = (sum << zoom_adjust) / count;
    }
    
    if (step < fix16_from_int(5))
        step = 0;
    
    // Determine the offset simply by the closest edge to the cursor
    signaltime_t center_time = xpos->get_xpos();
    stream->seek(center_time);
    stream->read_forwards(event);
    if (event.end - center_time < center_time - event.start)
        offset = xpos->get_x(event.end);
    else
        offset = xpos->get_x(event.start);
}

void Grid::Draw(uint16_t buffer[], int screenheight, int x)
{
    if (step == 0)
        return;
    
    // Find the index of the closest line to current x value,
    // and then map that back to x value.
    int index = fix16_div(x - offset, step);
    int line_x = fix16_mul(index, step) + offset;
    
    if (x == line_x)
    {
        for (int y = y0; y < screenheight && y < y1; y++)
        {
            buffer[y] = color;
        }
    }
}
