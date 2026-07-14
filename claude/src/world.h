#pragma once
#include <string>
#include <vector>

#include "enemy.h"
#include "map.h"
#include "pickup.h"
#include "player.h"

// One line of shop-floor commentary, top of the screen, fades out.
struct HudMessage {
    std::string text;
    float life = 0.0f;
};

// Everything that gets thrown away and rebuilt when you press R.
struct World {
    Map map;
    Player player;
    std::vector<Enemy> enemies;
    std::vector<Pickup> pickups;
    std::vector<Projectile> projectiles;

    int kills = 0;
    int totalEnemies = 0;
    float elapsed = 0.0f;
    bool escaped = false;      // you touched the loading dock door
    float shake = 0.0f;

    std::vector<HudMessage> messages;
    void Message(std::string text);
};

void WorldInit(World& w);
void WorldUpdate(World& w, float dt);
