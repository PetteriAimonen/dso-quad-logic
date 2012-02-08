/* This class handles the mapping of time values to X position and vice-versa.
 * It automatically collapses long idle periods to a shorter representation.
 */

#pragma once

#include <vector>
#include <memory>
#include "signalstream.hh"

class XPosHandler
{
public:
    // The constructor clones the SignalStream.
    XPosHandler(int screenwidth, const SignalStream &stream);
    
    // Zoom value
    // Scale is 2**zoom pixels/tick, so positive value is zoom in and
    // negative is zoom out. Zero is 1 pixel = 1 tick.
    int get_zoom() const { return zoom; }
    void set_zoom(int zoom);
    
    // X position: time instant at the center of the screen
    signaltime_t get_xpos() const { return x_pos; }
    void set_xpos(signaltime_t time);
    void move_xpos(int pixels);
    
    // Interface for drawing: convert time <=> pixel pos
    signaltime_t get_time(int x) const;
    int get_x(signaltime_t time) const;
    
    // Discontinuous points where idle periods have been collapsed
    struct Break {
        int x; // X position where the break occurs
        signaltime_t left; // Time of the break, measured from left side
        signaltime_t right; // Time of the break, measured from right side
    };
    
    void get_breaks(std::vector<Break> &results) const;
    
private:
    std::unique_ptr<SignalStream> stream;
    std::vector<Break> breaks;
    const int screenwidth;
    
    int zoom;
    signaltime_t x_pos;
    signaltime_t left_edge_time;
    
    // Collapsing of idle periods
    // Values in pixels
    const int collapse_threshold;
    const int collapse_length;
    
    int ticks_to_pixels(signaltime_t ticks) const;
    signaltime_t pixels_to_ticks(int pixels) const;
};
