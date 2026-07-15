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

// Solo is the three-level run; Arena is the multiplayer floor — no keycard, no
// exit door, deaths respawn instead of ending anything.
enum class Mode : unsigned char { Solo, Arena };

// The run: three levels crossed in sequence, one World for all of them.
struct World {
    Mode mode = Mode::Solo;
    Map map;
    Player player;
    std::vector<Enemy> enemies;
    std::vector<Pickup> pickups;
    std::vector<Projectile> projectiles;
    std::vector<Vector2> arenaSpawns;   // the arena's '@' set: respawn candidates

    int level = 0;             // index into the run, 0-based
    int kills = 0;             // all three accumulate across levels: the
    int totalEnemies = 0;      // end screens score the run, not the level
    float elapsed = 0.0f;
    bool escaped = false;      // you touched this level's loading dock door
    float shake = 0.0f;

    // Taken on level entry; R rewinds to it, so dying costs you the level and
    // nothing else. Entering level 3 on 4 health is your problem — Doom rules.
    PlayerLoadout entryLoadout{};
    int entryKills = 0;
    int entryTotalEnemies = 0;
    float entryElapsed = 0.0f;

    std::vector<HudMessage> messages;
    void Message(std::string text);
};

void WorldStartRun(World& w);       // level 1, factory loadout
void WorldNextLevel(World& w);      // carry the loadout through the dock door
void WorldRestartLevel(World& w);   // R: this level again, entry loadout
void WorldUpdate(World& w, float dt);

void WorldStartArena(World& w);     // the multiplayer floor, factory loadout
void WorldArenaRespawn(World& w);   // fresh body at the least-crowded '@'
