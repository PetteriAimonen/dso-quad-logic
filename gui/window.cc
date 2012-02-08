#include "window.hh"

Window::Window(int x0, int y0, int x1, int y1):
    items(), x0(x0), y0(y0), x1(x1), y1(y1)
{
}

void Window::Prepare(int xstart, int xend)
{
    if (xend <= x0 || xstart >= x1)
        return;
    
    if (xend > x1)
        xend = x1;
        
    xstart -= x0;
    xend -= x0;
    
    if (xstart < 0)
        xstart = 0;
    
    for (auto item: items)
    {
        item->Prepare(xstart, xend);
    }
}

void Window::Draw(uint16_t buffer[], int screenheight, int x)
{
    if (x < x0 || x >= x1)
        return;
    
    for (auto item: items)
    {
        item->Draw(buffer + y0, y1 - y0, x - x0);
    }
}

