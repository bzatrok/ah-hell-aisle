#pragma once

#include <raylib.h>

struct Projectile {
    Vector3 position;
    float speed;
    int damage;
    bool active;
    
    Texture2D sprite;
    
    void init(float x, float z, float angle, int dmg);
    void update(float deltaTime);
};
