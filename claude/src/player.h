#pragma once
#include <cmath>

#include "config.h"
#include "raylib.h"

struct World;

enum class WeaponId : int { Stokbrood = 0, Prijspistool = 1 };

struct Player {
    Vector2 pos{};              // world X, world Z
    float yaw = 0.0f;           // radians; 0 looks down +X
    int health = kMaxHealth;
    int armour = 0;
    int ammo = kStartAmmo;
    bool hasKeycard = false;

    WeaponId weapon = WeaponId::Stokbrood;
    float fireCooldown = 0.0f;  // enforces the rate of fire; spamming the key does nothing
    float fireAnim = 0.0f;      // counts down through the 3-frame swing/shot

    float bobPhase = 0.0f;
    float bobAmount = 0.0f;     // 0 standing, 1 walking flat out
    float hurtFlash = 0.0f;     // red over the screen
    float muzzleFlash = 0.0f;   // lights the aisle for a frame or two
    float deathFall = 0.0f;     // camera sinks to the floor when you go down
    float lockedNag = 0.0f;     // stops the locked-door line repeating every frame

    bool dead() const { return health <= 0; }
    Vector2 forward() const { return {cosf(yaw), sinf(yaw)}; }
    Vector2 right() const { return {-sinf(yaw), cosf(yaw)}; }
    float eyeHeight() const { return kEyeH - deathFall * (kEyeH - 0.16f); }
    int weaponFrame() const;
};

void PlayerUpdate(World& w, float dt);
void PlayerDamage(World& w, int damage);
