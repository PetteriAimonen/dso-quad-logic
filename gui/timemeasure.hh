/* A marker for measuring time between two events. */

#pragma once

#include "drawable.hh"
#include "xposhandler.hh"
#include "textdrawable.hh"

class TimeMeasure: public Drawable
{
public:
    TimeMeasure(const XPosHandler *xpos);
    
    virtual void Prepare(int xstart, int xend);
    virtual void Draw(uint16_t buffer[], int screenheight, int x);
    
    // Times to measure between.
    signaltime_t time1;
    signaltime_t time2;
    
    int y0; // Default: 20
    int y1; // Default: 200
    int linecolor; // Default: White
    
    // When being set using the buttons, the time measure
    // has three states: hidden, from start pos to cursor
    // and from start to end.
    enum State
    {
        HIDDEN = 0,
        START = 1,
        COMPLETE = 2
    };
    
    State state;
    
    void Click();
    
private:
    const XPosHandler *xpos;
    int x0;
    int x1;
    
    TextDrawable text;
};