#include <raylib.h>

#pragma once

enum class WeaponType;

struct Player {
    Vector3 pos;
    float rotZ;
    Vector3 dir;
    Vector3 rightDir;
    
    float moveSpeed;
    float turnSpeed;
    
    int health, maxHealth;
    int armor, maxArmor;
    int ammo;
    
    WeaponType weaponType;
    bool hasKeycard;
};
