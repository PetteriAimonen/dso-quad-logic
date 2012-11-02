/* Any object that can be rendered to screen.
 * The drawing model is that one vertical line (240 pixels * uint16_t) is
 * rendered at a time. All the Drawables in the region are called in turn
 * and allowed to render whatever they have to draw.
 */

#pragma once

#include <stdint.h>

class Drawable
{
public:  
    virtual ~Drawable() {};
    
    // Called immediately before rendering lines xstart <= x < xend.
    // You can do seeking/precalculation here if you want.
    virtual void Prepare(int xstart, int xend) {};
    
    // Render the vertical line at x to buffer
    virtual void Draw(uint16_t buffer[], int screenheight, int x) = 0;
    
    Drawable() {}
    Drawable(Drawable &&) = default;
};
