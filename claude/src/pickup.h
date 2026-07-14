#pragma once
#include "raylib.h"

struct World;

enum class PickupKind : int {
    Appelflap = 0,   // +25 health
    Rookworst,       // +50 health
    Labels,          // +20 label ammo
    Bonuskaart,      // +50 armour
    Keycard,         // the bedrijfsleider's pass. There is exactly one.
};

struct Pickup {
    PickupKind kind = PickupKind::Appelflap;
    Vector2 pos{};
    bool taken = false;
    float phase = 0.0f;   // so they do not all bob in lockstep
};

void PickupsUpdate(World& w, float dt);
