#include "Pickup.h"
#include "Player.h"
#include <cmath>

void Pickup::init(PickupType t, float x, float z) {
    pos = {x, 1.5f, z};
    type = t;
    collected = false;
}

void Pickup::update(const Player& player) {
    if (collected) return;
    
    const char* name;
    switch (type) {
        case PickupType::APPELFLAP: name = "Appelflap"; break;
        case PickupType::ROOKWORST: name = "Rookworst"; break;
        case PickupType::LABELS: name = "Bonuskaart Labels"; break;
        case PickupType::BONUSKAART: name = "Bonuskaart"; break;
        case PickupType::KEYCARD: name = "Magazijn Keycard"; break;
    }
    
    float dx = player.pos.x - pos.x;
    float dz = player.pos.z - pos.z;
    float dist = sqrt(dx * dx + dz * dz);
    
    if (dist < 1.0f && !collected) {
        collected = true;
        if (type == PickupType::KEYCARD) {
            ((Player*)&player)->hasKeycard = true;
        }
    }
}
