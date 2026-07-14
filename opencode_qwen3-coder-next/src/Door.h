#pragma once

#include <raylib.h>

struct Door {
    int x, z;
    bool closed;
    bool hasKeycard;
    
    void init(int wx, int wz);
    void open();
};
