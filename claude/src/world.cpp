#include "world.h"

#include "audio.h"
#include "net.h"
#include "raymath.h"

void World::Message(std::string text) {
    messages.push_back({std::move(text), 3.2f});
    if (messages.size() > 4) messages.erase(messages.begin());
}

// One authored character becoming an entity. '@' is not handled here: who it
// places (the solo player) or collects (arena respawn points) is the caller's.
static void SpawnEntity(World& w, const Spawn& s) {
    const Vector2 centre = {s.x + 0.5f, s.y + 0.5f};
    switch (s.kind) {
        case '1': w.enemies.push_back(MakeEnemy(EnemyKind::Winkelwagen, centre)); break;
        case '2': w.enemies.push_back(MakeEnemy(EnemyKind::Vakkenvuller, centre)); break;
        case '3': w.enemies.push_back(MakeEnemy(EnemyKind::Zelfscanner, centre)); break;
        case '4': w.enemies.push_back(MakeEnemy(EnemyKind::Beveiliger, centre)); break;
        case '5': w.enemies.push_back(MakeEnemy(EnemyKind::Bedrijfsleider, centre)); break;
        case 'a': w.pickups.push_back({PickupKind::Appelflap, centre, false, 0.0f}); break;
        case 'r': w.pickups.push_back({PickupKind::Rookworst, centre, false, 0.0f}); break;
        case 'l': w.pickups.push_back({PickupKind::Labels, centre, false, 0.0f}); break;
        case 'b': w.pickups.push_back({PickupKind::Bonuskaart, centre, false, 0.0f}); break;
        case 'k': w.pickups.push_back({PickupKind::Keycard, centre, false, 0.0f}); break;
        case 'f': w.pickups.push_back({PickupKind::Flessen, centre, false, 0.0f}); break;
        case 'v': w.pickups.push_back({PickupKind::Vuurwerk, centre, false, 0.0f}); break;
        case 'g': w.pickups.push_back({PickupKind::WeaponScatter, centre, false, 0.0f}); break;
        case 'p': w.pickups.push_back({PickupKind::WeaponRocket, centre, false, 0.0f}); break;
        default: break;
    }
}

// Stagger the pickup bob so the shop does not pulse in unison.
static void StaggerPickups(World& w) {
    float phase = 0.0f;
    for (Pickup& p : w.pickups) {
        p.phase = phase;
        phase += 1.1f;
    }
}

// Rebuilds map and entities for w.level. The player keeps their loadout but
// nothing else: keycard, flashes and momentum all reset with the room.
static void LoadCurrentLevel(World& w) {
    w.enemies.clear();
    w.pickups.clear();
    w.projectiles.clear();
    w.messages.clear();
    w.escaped = false;
    w.shake = 0.0f;

    Player fresh;
    ApplyLoadout(fresh, CaptureLoadout(w.player));
    w.player = fresh;

    std::vector<Spawn> spawns;
    w.map = LoadLevel(w.level, spawns);

    for (const Spawn& s : spawns) {
        if (s.kind == '@') {
            w.player.pos = {s.x + 0.5f, s.y + 0.5f};
            w.player.yaw = -PI / 2.0f;   // facing north, into the store
        } else {
            SpawnEntity(w, s);
        }
    }

    StaggerPickups(w);

    w.totalEnemies += (int)w.enemies.size();
    w.Message(LevelIntro(w.level));
}

static void TakeEntrySnapshot(World& w) {
    w.entryLoadout = CaptureLoadout(w.player);
    w.entryKills = w.kills;
    w.entryTotalEnemies = w.totalEnemies;
    w.entryElapsed = w.elapsed;
}

void WorldStartRun(World& w) {
    w = World{};
    TakeEntrySnapshot(w);
    LoadCurrentLevel(w);
}

void WorldNextLevel(World& w) {
    w.level++;
    TakeEntrySnapshot(w);
    LoadCurrentLevel(w);
}

void WorldRestartLevel(World& w) {
    w.kills = w.entryKills;
    w.totalEnemies = w.entryTotalEnemies;
    w.elapsed = w.entryElapsed;
    ApplyLoadout(w.player, w.entryLoadout);
    LoadCurrentLevel(w);
}

void WorldUpdate(World& w, float dt) {
    w.elapsed += dt;

    w.map.Update(dt);
    PlayerUpdate(w, dt);
    // In the arena only the host runs the monsters' minds; everyone else
    // receives them through the stream and puppets them (NetUpdate).
    if (w.mode == Mode::Solo || gNet.isHost) EnemiesUpdate(w, dt);
    ProjectilesUpdate(w, dt);
    PickupsUpdate(w, dt);
    if (w.mode == Mode::Arena) NetUpdate(w, dt);

    w.shake = fmaxf(0.0f, w.shake - dt * 3.0f);

    for (HudMessage& m : w.messages) m.life -= dt;
    while (!w.messages.empty() && w.messages.front().life <= 0.0f) {
        w.messages.erase(w.messages.begin());
    }
}

// --- the arena ----------------------------------------------------------------

// The '@' with the most breathing room: furthest from the nearest living threat,
// shoppers and monsters alike. Classic deathmatch placement — never in a fight,
// never in the same corner twice in a row unless the whole floor is hot.
static Vector2 ArenaSpawnPos(const World& w) {
    Vector2 best = {Map::W * 0.5f, Map::H * 0.5f};
    float bestScore = -1.0f;
    for (const Vector2& s : w.arenaSpawns) {
        float nearest = 1e9f;
        for (const RemotePlayer& p : gNet.peers) {
            if (p.alive) nearest = fminf(nearest, Vector2Distance(s, p.pos));
        }
        for (const Enemy& e : w.enemies) {
            if (e.alive()) nearest = fminf(nearest, Vector2Distance(s, e.pos));
        }
        if (nearest > bestScore) {
            bestScore = nearest;
            best = s;
        }
    }
    return best;
}

void WorldArenaRespawn(World& w) {
    Player fresh;   // factory loadout: stokbrood, prijspistool, 40 labels
    fresh.pos = ArenaSpawnPos(w);
    // Face the middle of the shop — wherever you wake up, the fight is that way.
    fresh.yaw = atan2f(Map::H * 0.5f - fresh.pos.y, Map::W * 0.5f - fresh.pos.x);
    w.player = fresh;
}

void WorldStartArena(World& w) {
    w = World{};
    w.mode = Mode::Arena;

    std::vector<Spawn> spawns;
    w.map = LoadArena(spawns);

    for (const Spawn& s : spawns) {
        if (s.kind == '@') {
            w.arenaSpawns.push_back({s.x + 0.5f, s.y + 0.5f});
        } else {
            if (s.kind == '1') {   // remember the docks the trolleys restock from
                w.arenaEnemySpawns.push_back({s.x + 0.5f, s.y + 0.5f});
            }
            SpawnEntity(w, s);
        }
    }

    StaggerPickups(w);
    NetResetMatch();
    WorldArenaRespawn(w);

    w.Message("04:44 - NACHTDIENST");
    w.Message("DE ANDEREN ZIJN OOK NOG BINNEN");
}
