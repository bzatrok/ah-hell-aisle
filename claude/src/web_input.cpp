#include "web_input.h"

WebInput gWebInput;

float WebConsumeYawDelta() {
    const float v = gWebInput.yawDelta;
    gWebInput.yawDelta = 0.0f;
    return v;
}

bool WebConsumeFirePressed() {
    const bool v = gWebInput.firePressed;
    gWebInput.firePressed = false;
    return v;
}

bool WebConsumeUsePressed() {
    const bool v = gWebInput.usePressed;
    gWebInput.usePressed = false;
    return v;
}

int WebConsumeWeaponStep() {
    const int v = gWebInput.weaponStep;
    gWebInput.weaponStep = 0;
    return v;
}

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>

// The JS touch layer in web/index.html calls these as Module._web_* — plain int/float
// args, so no ccall marshalling. The build is single-threaded (ASYNCIFY yields to the
// event loop between frames), so plain writes are safe.
extern "C" {

EMSCRIPTEN_KEEPALIVE void web_set_touch_mode(int on) { gWebInput.touchMode = on != 0; }

EMSCRIPTEN_KEEPALIVE void web_set_paused(int on) { gWebInput.paused = on != 0; }

EMSCRIPTEN_KEEPALIVE void web_add_yaw(float rad) { gWebInput.yawDelta += rad; }

EMSCRIPTEN_KEEPALIVE void web_set_move(float x, float y) {
    gWebInput.moveX = x;
    gWebInput.moveY = y;
}

EMSCRIPTEN_KEEPALIVE void web_set_fire(int down) {
    gWebInput.fireDown = down != 0;
    if (down) gWebInput.firePressed = true;
}

EMSCRIPTEN_KEEPALIVE void web_press_use(void) { gWebInput.usePressed = true; }

EMSCRIPTEN_KEEPALIVE void web_cycle_weapon(int step) { gWebInput.weaponStep += step; }

}  // extern "C"
#endif
