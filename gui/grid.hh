/* This is an automatically adjusting horizontal grid. */

#pragma once

#include <memory>
#include "drawable.hh"
#include "signalstream.hh"
#include "xposhandler.hh"
#include "fix16.h"

class Grid: public Drawable
{
public:
    Grid(const SignalStream &stream, const XPosHandler* xpos);
    
    virtual void Prepare(int xstart, int xend);
    virtual void Draw(uint16_t buffer[], int screenheight, int x);
    
    uint16_t color; // Default: White
    int y0; // Default: 0;
    int y1; // Default: 240;
    
private:
    std::unique_ptr<SignalStream> stream;
    const XPosHandler *xpos;
    
    fix16_t step; // Step in pixels
    int offset; // X position of (any 1) line
};