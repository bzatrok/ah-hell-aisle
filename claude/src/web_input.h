#pragma once
// Input injected from the browser shell (web/index.html) on touch devices.
// Native builds compile this too; nothing ever writes it there.
struct WebInput {
    bool  touchMode = false;   // set once on first touch: disables mouse-look
    bool  paused    = false;   // phone held portrait: rotate overlay is up, game freezes
    float yawDelta  = 0.0f;    // rad, right-thumb drag aim; consumed per frame
    float moveX = 0.0f;        // virtual stick, screen-right strafe, |v| <= 1
    float moveY = 0.0f;        // virtual stick, screen-up = forward,  |v| <= 1
    bool  fireDown    = false; // held state
    bool  firePressed = false; // edge — survives a sub-frame tap; consumed
    bool  usePressed  = false; // edge; consumed
    int   weaponStep  = 0;     // accumulated swipe steps (±1 each); consumed
};
extern WebInput gWebInput;
float WebConsumeYawDelta();
bool  WebConsumeFirePressed();
bool  WebConsumeUsePressed();
int   WebConsumeWeaponStep();
