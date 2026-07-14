#pragma once
#include "raylib.h"

struct World;

enum class EnemyKind : int {
    Winkelwagen = 0,
    Vakkenvuller = 1,
    Zelfscanner = 2,
    Beveiliger = 3,      // advancing marksman: closes in, frozen-ray single shots
    Bedrijfsleider = 4,  // the boss: alternates charging and can barrages
};

enum class EnemyState : unsigned char {
    Idle,    // has not noticed you
    Chase,   // moving, per its own idea of where it wants to be
    Attack,  // committed: winding up, swinging, throwing or burst-firing
    Dying,   // playing frames 3-4-5
    Dead,    // a corpse: does not block, cannot be shot, cannot hurt you
};

struct EnemyStats {
    int health;
    float speed;
    float radius;
    float sight;         // how far off it can notice you at all
    float attackRange;
    float cooldown;
    int damage;
    float spriteSize;    // billboard width and height, world units
};

const EnemyStats& StatsFor(EnemyKind kind);

struct Enemy {
    EnemyKind kind = EnemyKind::Winkelwagen;
    EnemyState state = EnemyState::Idle;
    Vector2 pos{};
    Vector2 strafe{};      // vakkenvuller: which way it is sidestepping right now
    int health = 0;
    float cooldown = 0.0f;
    float stateTimer = 0.0f;
    float animTimer = 0.0f;
    float hurtFlash = 0.0f;
    float strafeTimer = 0.0f;
    int shotsLeft = 0;     // hits still owed by the attack in progress
    float shotTimer = 0.0f;
    Vector2 aimDir{};      // frozen at attack start — the dodgeable-beam mechanic
    float beam = 0.0f;     // >0 while its beam is drawn
    float aimBeam = 0.0f;  // >0 while it is winding up: the telegraph you can dodge
    int phase = 0;         // bedrijfsleider only: 0 = charge, 1 = barrage
    float phaseTimer = 0.0f;

    bool alive() const { return state != EnemyState::Dying && state != EnemyState::Dead; }
    int frame() const;
};

// A real projectile: it flies, walls stop it. The soup can arcs and lands; the
// vuurwerkpijl flies flat and detonates on anything — wall, shopper or timeout.
struct Projectile {
    enum class Kind : unsigned char { SoupCan, Rocket };
    Kind kind = Kind::SoupCan;
    bool ownerIsPlayer = false;
    Vector2 pos{};
    Vector2 vel{};
    float height = 0.0f;   // world Y, for the arc
    float vy = 0.0f;
    float life = 0.0f;
};

Enemy MakeEnemy(EnemyKind kind, Vector2 pos);
void EnemiesUpdate(World& w, float dt);
void ProjectilesUpdate(World& w, float dt);

void EnemyHurt(World& w, Enemy& e, int damage);
void AlertEnemies(World& w, Vector2 from, float radius);   // noise wakes the shop up
