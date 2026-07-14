#pragma once

#include <vector>

#include "Player.h"
#include "Weapon.h"
#include "Pickup.h"
#include "Projectile.h"
#include "Door.h"
#include "TextureManager.h"
#include "Enemy.h"

#define MAP_WIDTH 40
#define MAP_HEIGHT 40

enum TileType {
    TILE_EMPTY = 0,
    TILE_WALL_SHELF_FULL = 1,
    TILE_WALL_SHELF_EMPTY = 2,
    TILE_WALL_FREEZER = 3,
    TILE_WALL_PLAIN = 4,
    TILE_WALL_CHECKOUT = 5,
    TILE_WALL_MAGAZIJN = 6
};

extern const unsigned short levelMap[MAP_WIDTH * MAP_HEIGHT];
extern std::vector<Enemy> enemies;
extern std::vector<Pickup> pickups;
extern std::vector<Door> doors;

void initLevel();
bool isSolidWall(int x, int z);
bool isDoor(int x, int z);
