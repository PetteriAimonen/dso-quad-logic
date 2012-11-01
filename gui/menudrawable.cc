#include "menudrawable.hh"

#include <cstring>

extern "C" {
#include "BIOS.h"
}

const int borderVMargin = 5;
const int borderHMargin = 7;
const int spacing = 5;

MenuDrawable::MenuDrawable(int x0, int y0, int count):
    x0(x0), y0(y0), width(width), index(0), count(count), borderColor(0xFFFF), backgroundColor(0x0000), visible(false)
{    
    int centerX = x0 + width / 2;
    textHeight = FONT_HEIGHT + spacing;
    height = count*textHeight + 2*borderVMargin;
    topY = y0+height;
    
    for (int i = 0; i < count; i++)
    {
        text[i] = new TextDrawable(centerX, topY-(textHeight*i)-textHeight/2-borderVMargin, "TEST");
        text[i]->halign = TextDrawable::CENTER;
        text[i]->valign = TextDrawable::MIDDLE;
    }
    
    for (int i = 0; i < (count -1); i++)
        separators[i] = false;
}

void MenuDrawable::setText(int index, const char* str)
{
    text[index]->set_text(str);
}

void MenuDrawable::setColor(int index, uint16_t color)
{
    text[index]->color = color;
}

void MenuDrawable::setSeparator(int index, bool enabled)
{
    separators[index] = enabled;
}

void MenuDrawable::next()
{
    if (index < count-1)
        index++;
    else
        index = 0;
}

void MenuDrawable::previous()
{
    if (index > 0)
        index--;
    else
        index = count-1;
}


void MenuDrawable::Prepare(int xstart, int xend)
{
    if (!visible)
        return;
    
    width = 0;
    
    for (int i = 0; i < count; i++)
    {
        text[i]->invert = false;
        if (text[i]->text_width() > width )
        {
            width = text[i]->text_width();
        }
    }
    
    width += borderHMargin*2;
    int centerX = x0 + width / 2;
    
    for (int i = 0; i < count; i++)
    {
        text[i]->x0 = centerX;
    }
    
    text[index]->invert = true;
    
    Drawable::Prepare(xstart, xend);
}

void MenuDrawable::Draw(uint16_t buffer[], int screenheight, int x)
{   
    if (!visible)
        return;
    
    if ((x < x0) || (x >= (x0 + width)))
        return;
    
    //draw the rect
    if ((x > x0) && (x < x0 + width - 1))   //nice round corners
    {
        buffer[y0+height-1] = borderColor;  //head line
        buffer[y0] = borderColor;           //bottom line
    }
        
    for (int y = (y0 + 1); y < (y0 + height - 1); y++)
    {
        if ((x == x0) || (x == (x0 + width-1)))
            buffer[y] = borderColor;
        else
            buffer[y] = backgroundColor;
    }
    
    //draw the text
    for (int i = 0; i < count; i++)
    {
        text[i]->Draw(buffer, screenheight, x);
    }
    
    //draw separators
    if ((x > (x0 + borderHMargin)) && (x < x0 + width - 1 - borderHMargin))   //add some niceness
    {
        for (int i = 0; i < (count-1); i++)
        {
            if (separators[i])
                buffer[topY-(i+1)*textHeight-borderVMargin] = borderColor;
        }
    }
}
