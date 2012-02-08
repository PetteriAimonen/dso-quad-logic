#include "breaklines.hh"
#include <stdio.h>

BreakLines::BreakLines(const XPosHandler *xpos, signaltime_t tickfreq):
    linecolor(0xFFFF), textcolor(0xFFFF), y0(20), y1(220),
    tickfreq(tickfreq), breaks(), xpos(xpos)
{
}

void format_time(char *buf, size_t size, signaltime_t time, signaltime_t freq)
{
    signaltime_t nanoseconds = 1000000000 * time / freq;
    
    if (nanoseconds < 1000)
    {
        snprintf(buf, size, "%ld ns", (int32_t)nanoseconds);
        return;
    }
    
    signaltime_t divider = 1000;
    while (nanoseconds >= 1000 * divider && divider < 1000000000)
    {
        divider *= 1000;
    } 
    
    // Format the numeric part using 2 significant digits
    size_t numlen;
    int32_t integer = nanoseconds / divider;
    if (integer < 10)
    {
        uint32_t fraction = (nanoseconds - integer * divider) * 10 / divider;
        numlen = snprintf(buf, size, "%ld.%01lu", integer, fraction);
    }
    else
    {
        numlen = snprintf(buf, size, "%ld", integer);
    }
    
    // Format the unit
    buf += numlen;
    size -= numlen;
    if (divider == 1000)
        snprintf(buf, size, " us");
    else if (divider == 1000000)
        snprintf(buf, size, " ms");
    else
        snprintf(buf, size, " s");
}

void BreakLines::Prepare(int xstart, int xend)
{
    xpos->get_breaks(breaks);
    texts.clear();
    texts.reserve(breaks.size());
    
    for (size_t i = 0; i < breaks.size(); i++)
    {
        auto &brk = breaks[i];
        if (brk.x == 0)
        {
            // Due to the left_edge_time adjustment in XPosHandler,
            // this would show wrong time.
            continue; 
        }
        
        char buffer[10];
        format_time(buffer, sizeof(buffer), brk.right - brk.left, tickfreq);
        
        texts.emplace_back(brk.x, y1, buffer);
        texts.back().halign = TextDrawable::CENTER;
        texts.back().color = textcolor;
        
        texts.back().Prepare(xstart, xend);
    }
}

void BreakLines::Draw(uint16_t buffer[], int screenheight, int x)
{
    const int halfwidth = separation / 2;
    
    for (auto &brk: breaks)
    {
        if (x >= brk.x - halfwidth && x <= brk.x + halfwidth)
        {
            DoDraw(buffer, screenheight, x, brk);
            break;
        }
    }
    
    for (auto &txt: texts)
    {
        txt.Draw(buffer, screenheight, x);
    }
}

void BreakLines::DoDraw(uint16_t buffer[], int screenheight, int x,
                        const XPosHandler::Break &brk)
{
    int end_y = y1 - 16;
    
    if (end_y > screenheight)
        end_y = screenheight;
    
    int left_x = brk.x - separation / 2 + sawtooth;
    int right_x = brk.x + separation / 2 - sawtooth;
    
//     printf("left %d right %d x %d\n", left_x, right_x, x);
    
    if (x > left_x && x < right_x)
    {
        memset(buffer, 0, 2 * screenheight);
        return;
    }
    
    if (x <= left_x)
    {
        int offset = left_x - x;
        int d1 = 2 * (sawtooth - offset);
        int d2 = 2 * offset;
        int y = y0 + offset;
        while (y < end_y)
        {
            buffer[y] = linecolor;
            
            y++;
            int end = y + d1;
            while (y < end && y < end_y)
            {
                buffer[y] = 0;
                y++;
            }
            
            if (y < end_y)
                buffer[y] = linecolor;
            
            y += d2;
        }
    }
    else if (x >= right_x)
    {
        int offset = sawtooth - (x - right_x);
        int d1 = 2 * (sawtooth - offset);
        int d2 = 2 * offset;
        int y = y0 + offset;
        while (y < end_y)
        {
            buffer[y] = linecolor;
            
            y += d1;
            
            if (y < end_y)
                buffer[y] = linecolor;
            
            y++;
            int end = y + d2;
            while (y < end && y < end_y)
            {
                buffer[y] = 0;
                y++;
            }
        }
    }
}
