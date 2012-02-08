/* A Drawable that draws a string on the screen. */

#pragma once

#include "drawable.hh"
#include <memory>

class TextDrawable: public Drawable {
public:
    enum VerticalAlign
    {
        TOP = 0,
        MIDDLE = 1,
        BOTTOM = 2
    };
    
    enum HorizontalAlign
    {
        LEFT = 0,
        CENTER = 1,
        RIGHT = 2
    };
    
    // The class will make a private copy of the string.
    TextDrawable(int x0, int y0, const char *str);
    virtual ~TextDrawable() {};
    
    void set_text(const char *str);
    
    virtual void Draw(uint16_t buffer[], int screenheight, int x);
    
    int x0;
    int y0;
    VerticalAlign valign; // Default: TOP
    HorizontalAlign halign; // Default: LEFT
    uint16_t color; // Default: White
    bool invert; // Default: false
    
private:
    std::unique_ptr<const char[]> str;
    size_t len;
};
