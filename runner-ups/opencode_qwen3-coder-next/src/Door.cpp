#include "Door.h"

void Door::init(int wx, int wz) {
    x = wx;
    z = wz;
    closed = true;
    hasKeycard = false;
}

void Door::open() {
    closed = false;
}
