/* A Drawable for drawing a jagged break lines where an idle portion of the
 * signal has been cut off the screen.
 */

#include "xposhandler.hh"
#include "drawable.hh"
#include "textdrawable.hh"
#include <vector>

class BreakLines: public Drawable
{
public:
    BreakLines(const XPosHandler *xpos, signaltime_t tickfreq);
    
    virtual void Prepare(int xstart, int xend);
    virtual void Draw(uint16_t buffer[], int screenheight, int x);
    
    uint16_t linecolor; // Default: White
    uint16_t textcolor; // Default: White
    
    int y0; // Default: 20
    int y1; // Default: 220
    
    signaltime_t tickfreq;
    
private:
    std::vector<XPosHandler::Break> breaks;
    std::vector<TextDrawable> texts;
    const XPosHandler *xpos;
    
    void DoDraw(uint16_t buffer[], int screenheight, int x, const XPosHandler::Break &brk);
    
    // Separation between the jagged lines
    static const int separation = 20;
    
    // Amplitude of the sawtooth
    static const int sawtooth = 5;
};
