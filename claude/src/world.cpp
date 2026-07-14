#include "world.h"

#include "audio.h"
#include "raymath.h"

void World::Message(std::string text) {
    messages.push_back({std::move(text), 3.2f});
    if (messages.size() > 4) messages.erase(messages.begin());
}

void WorldInit(World& w) {
    w = World{};

    std::vector<Spawn> spawns;
    w.map = LoadLevel(spawns);

    for (const Spawn& s : spawns) {
        const Vector2 centre = {s.x + 0.5f, s.y + 0.5f};
        switch (s.kind) {
            case '@':
                w.player.pos = centre;
                w.player.yaw = -PI / 2.0f;   // facing north, into the store
                break;
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
    float phase = 0.0f;
    for (Pickup& p : w.pickups) {
        p.phase = phase;
        phase += 1.1f;
    }

    w.totalEnemies = (int)w.enemies.size();
    w.Message("02:14 - DE WINKEL IS GESLOTEN");
}

void WorldUpdate(World& w, float dt) {
    w.elapsed += dt;

    w.map.Update(dt);
    PlayerUpdate(w, dt);
    EnemiesUpdate(w, dt);
    ProjectilesUpdate(w, dt);
    PickupsUpdate(w, dt);

    w.shake = fmaxf(0.0f, w.shake - dt * 3.0f);

    for (HudMessage& m : w.messages) m.life -= dt;
    while (!w.messages.empty() && w.messages.front().life <= 0.0f) {
        w.messages.erase(w.messages.begin());
    }
}
