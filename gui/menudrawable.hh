#pragma once

#include "drawable.hh"
#include "textdrawable.hh"
#include <memory>

class MenuDrawable: public Drawable {
public:

    // The class will make a private copy of the string.
    MenuDrawable(int x0, int y0, int count);
    virtual ~MenuDrawable() {};
    
    virtual void Prepare(int xstart, int xend);
    virtual void Draw(uint16_t buffer[], int screenheight, int x);
    
    void setText(int index, const char *str);
    void setColor(int index, uint16_t color);
    void setSeparator(int index, bool enabled);
    void next();
    void previous();
    
    int x0;
    int y0;
    int width;
    int height;
    int index;
    int count;
    
    uint16_t borderColor; // Default: Yellow
    uint16_t backgroundColor; // Default: Blue
    
    bool visible; // Default: false
    
private:
    TextDrawable *text[10];
    bool         separators[9];
    int          textHeight;
    int topY;
    
};
