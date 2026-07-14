#pragma once

#include <raylib.h>

struct Player;

enum class PickupType {
    APPELFLAP,
    ROOKWORST,
    LABELS,
    BONUSKAART,
    KEYCARD
};

struct Pickup {
    Vector3 pos;
    PickupType type;
    bool collected;
    
    void init(PickupType t, float x, float z);
    void update(const Player& player);
};
