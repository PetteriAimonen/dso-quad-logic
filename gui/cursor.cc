#include "cursor.hh"

Cursor::Cursor(const XPosHandler *xpos):
y0(20), y1(220), linecolor(0xFFFF), xpos(xpos)
{}

void Cursor::Prepare(int xstart, int xend)
{
    middle_x = xpos->get_x(xpos->get_xpos());
}

void Cursor::Draw(uint16_t buffer[], int screenheight, int x)
{
    if (x != middle_x)
        return;
    
    for (int y = y0; y < y1; y += 2)
    {
        buffer[y] = linecolor;
    }
}
