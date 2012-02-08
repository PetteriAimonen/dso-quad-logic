#include "xposhandler.hh"

XPosHandler::XPosHandler(int screenwidth, const SignalStream &stream):
    screenwidth(screenwidth),
    zoom(0),
    collapse_threshold(screenwidth),
    collapse_length(screenwidth / 4)
{
    this->stream.reset(stream.clone());
    this->left_edge_time = 0;
    
    set_xpos(0);
}

int XPosHandler::ticks_to_pixels(signaltime_t ticks) const
{
    if (zoom >= 0)
    {
        return ticks << zoom;
    }
    else
    {
        return ticks >> (-zoom);
    }
}

signaltime_t XPosHandler::pixels_to_ticks(int pixels) const
{
    if (zoom >= 0)
    {
        return pixels >> zoom;
    }
    else
    {
        return pixels << (-zoom);
    }
}

extern "C" {
#include "stdio.h"
}

void XPosHandler::set_zoom(int zoom)
{
    this->zoom = zoom;
    
    stream->seek(x_pos);
    SignalEvent event;
    stream->read_forwards(event);
    
    // Go left counting events until we hit the left edge of the screen
    signaltime_t time_on_screen = 0, time_in_signal = 0;
    signaltime_t threshold = pixels_to_ticks(collapse_threshold) - 1;
    signaltime_t width = pixels_to_ticks(screenwidth / 2);
    
    while (time_on_screen < width &&
           stream->read_backwards(event))
    {
        signaltime_t real_len = event.end - event.start;
        
        if (event.end > x_pos)
            event.end = x_pos;
        
        signaltime_t partial_len = event.end - event.start;
        
        if (real_len > threshold)
        {
            // There is an idle period that will be collapsed on display.
            // We have to figure how much space will it take on the left
            // side of x_pos.
            signaltime_t new_len = pixels_to_ticks(collapse_length);

            if (partial_len < new_len / 2)
            {
                // Cursor is on the left side of the break
                time_in_signal += partial_len;
                time_on_screen += partial_len;
            }
            else if (time_on_screen + new_len / 2 < width)
            {
                // Cursor is on the right side of the break
                signaltime_t delta = real_len - partial_len;
                if (delta > new_len / 2)
                    delta = new_len / 2;
                
                time_in_signal += partial_len;
                time_on_screen += new_len - delta;
            }
            else
            {
                // Break is at the left edge of the screen
                time_in_signal += new_len / 2;
                time_on_screen += new_len / 2;
            }
        }
        else
        {
            time_in_signal += partial_len;
            time_on_screen += partial_len;
        }
    }
    
    printf("time %ld\n", (uint32_t)time_in_signal);
    
    time_in_signal -= time_on_screen - width;
    left_edge_time = x_pos - time_in_signal;
    
    if (left_edge_time < 0)
        left_edge_time = 0;
    
    printf("left_edge_time %ld\n", (uint32_t)left_edge_time);
    
    // Now find all the breaks
    stream->seek(left_edge_time);
    breaks.clear();
    
    while (stream->read_forwards(event) &&
           get_x(event.start) < screenwidth)
    {
        int event_len = ticks_to_pixels(event.end - event.start);
        
        if (event_len > collapse_threshold)
        {
            printf("Breaking at %10ld %10ld\n", (uint32_t)event.start, (uint32_t)(event.end - event.start));
            Break brk = {};
            signaltime_t new_len = pixels_to_ticks(collapse_length);
            brk.left = event.start + new_len / 2;
            brk.right = event.end - new_len / 2;
            
            if (brk.left < left_edge_time)
                brk.left = left_edge_time;
            
            if (brk.right <= brk.left)
                continue;
            
            brk.x = get_x(brk.left);
            
            breaks.push_back(brk);
        }
    }
    
    // Remove the last break if nothing happens after it
    if (breaks.size() > 0 &&
        event.start < breaks.back().left &&
        !stream->read_forwards(event))
    {
        breaks.pop_back();
    }
}

void XPosHandler::set_xpos(signaltime_t time)
{
    x_pos = time;
    set_zoom(zoom);
}

void XPosHandler::move_xpos(int pixels)
{
    x_pos = get_time(get_x(x_pos) + pixels);
    
    if (x_pos < 0)
        x_pos = 0;
    
    set_zoom(zoom);
}

signaltime_t XPosHandler::get_time(int x) const
{
    signaltime_t time = left_edge_time + pixels_to_ticks(x);
    for (const Break &brk : breaks)
    {
        if (brk.x < x)
        {
            time += brk.right - brk.left;
        }
        else
        {
            break;
        }
    }
    return time;
}

int XPosHandler::get_x(signaltime_t time) const
{
    signaltime_t gaps = 0;
    for (const Break &brk : breaks)
    {
        if (brk.right < time)
        {
            gaps += brk.right - brk.left;
        }
        else if (brk.left <= time && brk.right >= time)
        {
            return brk.x;
        }
        else
        {
            break;
        }
    }
    
    int x = ticks_to_pixels(time - gaps - left_edge_time);
    
    return x;
}

void XPosHandler::get_breaks(std::vector<Break> &results) const
{
    results.clear();
    results.reserve(breaks.size());
    
    for (const Break &brk : breaks)
    {
        results.push_back(brk);
    }
}
