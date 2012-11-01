#include "textdrawable.hh"

#include <cstring>

extern "C" {
#include "BIOS.h"
}

TextDrawable::TextDrawable(int x0, int y0, const char *str):
    x0(x0), y0(y0), valign(TOP), halign(LEFT), color(0xFFFF), invert(false)
{
    set_text(str);
}

void TextDrawable::set_text(const char *str)
{
    len = strlen(str);
    char *copy = new char[len + 1];
    memcpy(copy, str, len + 1);
    this->str.reset((const char*)copy);
}

int TextDrawable::text_width()
{
    return len*FONT_WIDTH;
}

void TextDrawable::Draw(uint16_t buffer[], int screenheight, int x)
{
    int width = len * FONT_WIDTH;
    int left_edge_x = x0;
    if (halign == CENTER)
        left_edge_x -= width / 2;
    else if (halign == RIGHT)
        left_edge_x -= width;
    
    x -= left_edge_x;
    if (x < 0 || x >= width)
        return;
    
    int bottom_edge_y = y0;
    if (valign == MIDDLE)
        bottom_edge_y -= FONT_HEIGHT / 2;
    else if (valign == TOP)
        bottom_edge_y -= FONT_HEIGHT;
    
    if (bottom_edge_y < 0 || bottom_edge_y + FONT_HEIGHT > screenheight)
        return;
    
    char character = str[x / FONT_WIDTH];
    
    uint16_t column = __Get_TAB_8x14(character, x % FONT_WIDTH);
    
    if (character == ' ')
        column = 0;
    
    for (int i = 0; i < FONT_HEIGHT; i++)
    {
        if (!!(column & 4) != invert)
            buffer[bottom_edge_y + i] = color;
        
        column >>= 1;
    }
}

