#include <raylib.h>
#include <cmath>

#pragma once

struct Player;

enum class EnemyType {
    WINKELWAGEN,
    VAKKENVULLER,
    ZELFSCANNER
};

struct Enemy {
    Vector3 pos;
    Vector3 dir;
    
    int health, damage;
    float speed;
    
    float viewDistance;
    float attackCooldown;
    float lastAttackTime;
    bool alive;
    EnemyType type;
    
    void init(EnemyType t, float x, float z);
    void update(float deltaTime, float playerX, float playerZ, Player* playerPtr);
    void takeDamage(int amount);
};

#include "Player.h"

inline void Enemy::init(EnemyType t, float x, float z) {
    pos = {x, 2.0f, z};
    
    static const int enemyHealth[] = {30, 60, 100};
    static const float enemySpeed[] = {4.0f, 1.8f, 0.4f};
    static const int enemyDamage[] = {4, 8, 20};
    
    type = t;
    health = enemyHealth[(int)t];
    speed = enemySpeed[(int)t];
    damage = enemyDamage[(int)t];
    
    dir = {1.0f, 0.0f, 0.0f};
    
    attackCooldown = (t == EnemyType::WINKELWAGEN) ? 1.0f : ((t == EnemyType::VAKKENVULLER) ? 2.0f : 1.5f);
    viewDistance = (t == EnemyType::WINKELWAGEN) ? 8.0f : ((t == EnemyType::VAKKENVULLER) ? 10.0f : 14.0f);
    
    lastAttackTime = 0.0f;
    alive = true;
}

inline void Enemy::update(float deltaTime, float playerX, float playerZ, Player* playerPtr) {
    if (!alive || !playerPtr) return;
    
    float dx = playerX - pos.x;
    float dz = playerZ - pos.z;
    float dist = sqrt(dx * dx + dz * dz);
    
    bool canSeePlayer = (dist < viewDistance);
    
    if (canSeePlayer && dist > 1.0f) {
        float dirX = dx / dist;
        float dirZ = dz / dist;
        
        pos.x += dirX * speed * deltaTime;
        pos.z += dirZ * speed * deltaTime;
        
        float angle = atan2(dz, dx);
        dir = {cos(angle), 0.0f, sin(angle)};
    }
    
    if (dist <= 1.5f && playerPtr->health > 0) {
        lastAttackTime += deltaTime;
        if (lastAttackTime >= attackCooldown) {
            lastAttackTime = 0.0f;
            playerPtr->health -= damage;
        }
    }
}

inline void Enemy::takeDamage(int amount) {
    health -= amount;
    if (health <= 0) {
        alive = false;
    }
}
