/* Cursor for showing the current xpos. A vertical dotted line. */

#pragma once

#include "drawable.hh"
#include "xposhandler.hh"

class Cursor: public Drawable
{
public:
    Cursor(const XPosHandler *xpos);
    
    virtual void Prepare(int xstart, int xend);
    virtual void Draw(uint16_t buffer[], int screenheight, int x);
    
    int y0; // Default: 20
    int y1; // Default: 220
    int linecolor; // Default: White
    
private:
    const XPosHandler *xpos;
    int middle_x;
};