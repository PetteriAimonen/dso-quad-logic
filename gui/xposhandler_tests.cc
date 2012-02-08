#include "xposhandler.hh"
#include "testsignalstream.hh"
#include "unittests.h"

int main()
{
    int status = 0;
    
    {
        COMMENT("Testing basic functionality (no collapsing)");
        TestSignalStream stream("_-_-_-_-_-_-_","","","");
        XPosHandler xpos(400, stream);
        
        xpos.set_xpos(0);
        xpos.set_zoom(0);
        TEST(xpos.get_x(4) == 4);
        TEST(xpos.get_time(4) == 4);
        
        xpos.set_xpos(4);
        xpos.set_zoom(7);
        TEST(xpos.get_time(200) == 4);
        TEST(xpos.get_time(100) == 3);
    }
    
    {
        COMMENT("Testing basic collapsing");
        TestSignalStream stream("-___________--","","","");
        XPosHandler xpos(400, stream);
        
        xpos.set_xpos(0);
        xpos.set_zoom(7); // 128 pixels per tick
        
        std::vector<XPosHandler::Break> breaks;
        xpos.get_breaks(breaks);
        TEST(breaks.size() == 1);
        
        TEST(xpos.get_x(13) < 300);
        
        TEST(xpos.get_time(xpos.get_x(13)) == 13);
    }
    
    return status;
}
