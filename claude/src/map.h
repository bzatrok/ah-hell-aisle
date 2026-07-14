#pragma once
#include <vector>
#include "raylib.h"

// The store. A 2D grid of tiles, extruded to full-height walls. A tile is either
// empty floor or a solid block of one texture — that is the whole world model.

enum class Tile : unsigned char {
    Empty = 0,
    Plain,        // wall_plain     — store perimeter
    Checkout,     // wall_checkout  — the lanes you start behind
    ShelfFull,    // wall_shelf_full
    ShelfEmpty,   // wall_shelf_empty
    Freezer,      // wall_freezer
    Magazijn,     // wall_magazijn  — back of house
    DoorKeycard,  // door_keycard   — locked until you have the pass
    DoorExit,     // door_exit      — touch it and you are out
};
constexpr int kTileKindCount = 9;

// Zones exist for one reason: light. Each has its own ambient colour, and the
// freezer's flickers. They mirror the rectangles the level was authored in.
enum class Zone : unsigned char { Store, Checkout, Freezer, Magazijn };

struct Door {
    int x, y;
    bool isExit;      // the loading dock door: it never opens, touching it wins
    bool open;
    float slide;      // 0 = shut, 1 = fully retracted into the ceiling
};

// One character of the authored level that becomes an entity rather than a tile.
struct Spawn {
    int x, y;
    char kind;        // '@' player, '1'..'5' enemies, 'a r l b k f v g p' pickups
};

struct Map {
    static constexpr int W = 40;
    static constexpr int H = 40;

    Tile tiles[H][W]{};
    Zone zones[H][W]{};
    std::vector<Door> doors;

    static bool InBounds(int x, int y) { return x >= 0 && x < W && y >= 0 && y < H; }
    Tile At(int x, int y) const { return InBounds(x, y) ? tiles[y][x] : Tile::Plain; }
    Zone ZoneAt(int x, int y) const { return InBounds(x, y) ? zones[y][x] : Zone::Store; }

    // Solid blocks movement, sight and shots alike — an open door blocks none of them.
    bool Solid(int x, int y) const;
    bool SolidAt(Vector2 p) const { return Solid((int)p.x, (int)p.y); }

    Door* DoorAt(int x, int y);
    const Door* DoorAt(int x, int y) const;

    void Update(float dt);

    // Distance along `dir` (normalised) to the first solid tile, capped at maxDist.
    float RayToWall(Vector2 origin, Vector2 dir, float maxDist) const;
    bool LineOfSight(Vector2 a, Vector2 b) const;

    // Circle-vs-tile, resolved one axis at a time so you slide along walls instead of
    // sticking to them. Never returns a position inside a solid tile.
    Vector2 SlideMove(Vector2 pos, Vector2 delta, float radius) const;
    bool Fits(Vector2 pos, float radius) const;
};

// The run is three hand-authored levels. LoadLevel builds one of them and hands
// back everything that needs to become an entity; LevelIntro is its HUD greeting.
constexpr int kLevelCount = 3;

Map LoadLevel(int level, std::vector<Spawn>& spawns);
const char* LevelIntro(int level);
