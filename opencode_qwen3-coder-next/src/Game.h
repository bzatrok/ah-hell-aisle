#pragma once

#include "GameState.h"
#include <vector>

struct Player;
struct Weapon;
struct TextureManager;
struct Pickup;
struct Enemy;
struct Door;

#include "WeaponType.h"
#include "Player.h"
#include "Weapon.h"
#include "TextureManager.h"
#include "Pickup.h"
#include "Enemy.h"
#include "Door.h"

struct RayResult {
    bool hit;
    float distance;
    int wallX, wallZ;
    int side;
};

struct Game {
    GameState state;
    
    Player player;
    Weapon weapon;
    TextureManager texMan;
    std::vector<Pickup> pickups;
    std::vector<Enemy> enemies;
    std::vector<Door> doors;
    
    void init();
    void update(float deltaTime);
    void draw();
    void handleInput(float dt);
    void reset();
    
    RayResult castRay(float ox, float oy, float dirX, float dirY);
    
    void drawTitle();
    void drawGameWorld();
    void drawDeathScreen();
    void drawVictoryScreen();
};
