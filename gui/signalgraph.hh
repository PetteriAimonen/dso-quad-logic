/* A graph of a single binary signal. */

#pragma once

#include "xposhandler.hh"
#include "drawable.hh"
#include "signalstream.hh"
#include <memory>

class SignalGraph: public Drawable
{
public:
    SignalGraph(const SignalStream &stream, const XPosHandler* xpos, int channel);
    
    virtual void Prepare(int xstart, int xend);
    virtual void Draw(uint16_t buffer[], int screenheight, int x);

    int y0; // Default: 0
    int height; // Default: 16
    uint16_t color; // Default: White
    
    int offset; // FIXME: horrible ugly ugly hack, remove once signal capture is properly synced between channels
    
private:
    std::unique_ptr<SignalStream> stream;
    signals_t channel_mask;
    
    SignalEvent current_event;
    const XPosHandler *xpos;
};
