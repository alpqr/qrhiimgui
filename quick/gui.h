#ifndef GUI_H
#define GUI_H

#include "imgui.h"

struct Gui
{
    void init();
    void cleanup();
    void frame();
};

#endif
