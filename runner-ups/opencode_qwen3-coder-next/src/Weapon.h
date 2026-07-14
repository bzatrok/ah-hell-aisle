#include <raylib.h>

#pragma once

struct Player;

#include "WeaponType.h"

struct Weapon {
    Vector3 offset;
    bool firing;
    float fireTime;
    float animTimer;
    
    void update(float deltaTime, float rotZ, float posX, float posZ);
    void fire(Player* player);
};

inline void Weapon::update(float deltaTime, float rotZ, float posX, float posZ) {
    animTimer += deltaTime;
}

inline void Weapon::fire(Player* player) {
    firing = true;
    fireTime = 0.0f;
    
    if (player->weaponType == WeaponType::STOKBROOD) {
        player->ammo--;
    }
}
