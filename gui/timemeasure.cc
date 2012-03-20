#include <stdio.h>
#include "timemeasure.hh"
#include "dsosignalstream.hh"
#include "../mathutils.h"

TimeMeasure::TimeMeasure(const XPosHandler *xpos):
y0(20), y1(200), linecolor(0xFFFF), state(HIDDEN), xpos(xpos),
text(0, 0, "")
{
    text.valign = TextDrawable::BOTTOM;
    text.halign = TextDrawable::CENTER;
}

void TimeMeasure::Prepare(int xstart, int xend)
{
    if (state == HIDDEN)
        return;
    
    if (state == START)
        time2 = xpos->get_xpos();
    
    x0 = xpos->get_x(time1);
    x1 = xpos->get_x(time2);
    
    if (x1 < x0)
    {
        int tmp = x0;
        x0 = x1;
        x1 = tmp;
    }
    
    text.color = linecolor;
    text.x0 = (x0 + x1) / 2;
    text.y0 = y1;
    
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%d us",
             (unsigned)(abs(time2 - time1) * 1000000 / DSOSignalStream::frequency));
    text.set_text(buffer);
    
    text.Prepare(xstart, xend);
}

void TimeMeasure::Draw(uint16_t buffer[], int screenheight, int x)
{
    if (state == HIDDEN)
        return;
    
    if (x == x0 || x == x1)
    {
        for (int y = y0; y < y1; y += 2)
        {
            buffer[y] = linecolor;
        }
    }
    else if (x > x0 && x < x1)
    {
        buffer[y1 - 2] = linecolor;
        
        // Draw arrowhead
        int delta = MIN(x - x0, x1 - x);
        if (delta < 4)
        {
            for (int i = 1; i < delta; i++)
            {
                buffer[y1 - 2 + i] = linecolor;
                buffer[y1 - 2 - i] = linecolor;
            }
        }
    }
    
    text.Draw(buffer, screenheight, x);
}

void TimeMeasure::Click()
{
    signaltime_t time = xpos->get_xpos();
    if (state == HIDDEN)
    {
        state = START;
        time1 = time;
    }
    else if (state == START)
    {
        state = COMPLETE;
        time2 = time;
    }
    else if (state == COMPLETE)
    {
        if (time == time2)
            state = HIDDEN;
        else
        {
            time2 = time;
        }
    }
}
