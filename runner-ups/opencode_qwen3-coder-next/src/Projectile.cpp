#include "Projectile.h"
#include <cmath>

void Projectile::init(float x, float z, float angle, int dmg) {
    position = {x, 1.5f, z};
    speed = 12.0f;
    damage = dmg;
    active = true;
}

void Projectile::update(float deltaTime) {
    if (!active) return;
    
    position.x += cos(position.z) * speed * deltaTime;
    position.z += sin(position.z) * speed * deltaTime;
}
