/* A window that allows repositioning of Drawables on the screen. */

#pragma once

#include "drawable.hh"
#include <vector>

class Window: public Drawable
{
public:
    Window(int x0, int y0, int x1, int y1);
    
    std::vector<Drawable*> items;
    
    virtual void Prepare(int xstart, int xend);
    virtual void Draw(uint16_t buffer[], int screenheight, int x);

    int x0;
    int y0;
    int x1;
    int y1;
};