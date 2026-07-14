#include "enemy.h"

#include <algorithm>
#include <cmath>

#include "audio.h"
#include "config.h"
#include "raymath.h"
#include "world.h"

// The first three exactly as SPEC §6 listed them; the last two are the expansion.
//               health  speed  radius  sight  range  cooldown  damage  sprite
static const EnemyStats kStats[5] = {
    {  30,  4.0f,  0.30f,  18.0f,   0.95f,  1.0f,  10,  1.00f },  // Winkelwagen: the rusher
    {  60,  1.8f,  0.30f,  16.0f,  10.00f,  2.0f,  15,  1.35f },  // Vakkenvuller: the zoner
    { 100,  0.4f,  0.36f,  15.0f,  14.00f,  1.5f,   5,  1.30f },  // Zelfscanner:  the turret
    {  80,  2.6f,  0.30f,  17.0f,  12.00f,  1.8f,  10,  1.35f },  // Beveiliger:   the marksman
    { 500,  3.2f,  0.50f,  20.0f,   1.10f,  1.2f,  25,  1.90f },  // Bedrijfsleider: the boss
};

const EnemyStats& StatsFor(EnemyKind kind) { return kStats[(int)kind]; }

// The soup can's flight. Tuned so a can thrown from the far edge of its range still
// clears the shelves and passes under the ceiling.
constexpr float kCanSpeed = 6.0f;
constexpr float kCanGravity = 1.6f;
constexpr float kCanLaunchHeight = 0.75f;

constexpr float kWalkFrameTime = 0.22f;

Enemy MakeEnemy(EnemyKind kind, Vector2 pos) {
    Enemy e;
    e.kind = kind;
    e.pos = pos;
    e.health = StatsFor(kind).health;
    e.strafe = {1.0f, 0.0f};
    return e;
}

int Enemy::frame() const {
    switch (state) {
        case EnemyState::Idle:   return 0;
        case EnemyState::Chase:  return (animTimer < kWalkFrameTime) ? 0 : 1;
        case EnemyState::Attack: return 2;
        case EnemyState::Dying:
            if (stateTimer < 0.14f) return 3;
            if (stateTimer < 0.28f) return 4;
            return 5;
        case EnemyState::Dead:   return 5;
    }
    return 0;
}

// --- shared helpers ----------------------------------------------------------

static void Step(World& w, Enemy& e, Vector2 dir, float dt) {
    if (Vector2Length(dir) < 1e-4f) return;
    const EnemyStats& s = StatsFor(e.kind);
    const Vector2 delta = Vector2Scale(Vector2Normalize(dir), s.speed * dt);

    const Vector2 want = w.map.SlideMove(e.pos, delta, s.radius);

    // You are an obstacle, not a bumper. A charging trolley stops dead against you and
    // swings — it does not shovel you down the aisle.
    const float minDist = s.radius + kPlayerRadius;
    const float after = Vector2Distance(want, w.player.pos);
    if (after >= minDist || after >= Vector2Distance(e.pos, w.player.pos)) {
        e.pos = want;
    }

    e.animTimer += dt;
    if (e.animTimer >= kWalkFrameTime * 2.0f) e.animTimer = 0.0f;
}

static bool Notices(const World& w, const Enemy& e, float dist) {
    return dist < StatsFor(e.kind).sight && w.map.LineOfSight(e.pos, w.player.pos);
}

static void BeginAttack(Enemy& e, int shots) {
    e.state = EnemyState::Attack;
    e.stateTimer = 0.0f;
    e.shotsLeft = shots;
    e.shotTimer = 0.0f;
}

static void EndAttack(Enemy& e) {
    e.state = EnemyState::Chase;
    e.stateTimer = 0.0f;
    e.cooldown = StatsFor(e.kind).cooldown;
    e.aimBeam = 0.0f;
}

// The frozen-ray shot. Aim locks when the attack starts, so during the windup the
// telegraphed line stays put and stepping out of it is a real dodge. The shot then
// tests the player against a corridor around that ray, not their live position.
static void FreezeAim(const World& w, Enemy& e) {
    e.aimDir = Vector2Normalize(Vector2Subtract(w.player.pos, e.pos));
}

static bool FrozenRayHits(const World& w, const Enemy& e, float range) {
    const Vector2 to = Vector2Subtract(w.player.pos, e.pos);
    const float along = Vector2DotProduct(to, e.aimDir);
    const float perp2 = Vector2DotProduct(to, to) - along * along;
    const float halfW = kPlayerRadius + 0.12f;
    return along > 0.0f && along <= range && perp2 <= halfW * halfW;
}

// Lob a soup can at the player, `angleOffset` radians off the direct line. The
// arc is tuned to land where the player stands now.
static void ThrowCan(World& w, const Enemy& e, float angleOffset) {
    const Vector2 toPlayer = Vector2Subtract(w.player.pos, e.pos);
    const float d = fmaxf(0.5f, Vector2Length(toPlayer));
    const float angle = atan2f(toPlayer.y, toPlayer.x) + angleOffset;
    const Vector2 dir = {cosf(angle), sinf(angle)};

    Projectile can;
    can.pos = Vector2Add(e.pos, Vector2Scale(dir, StatsFor(e.kind).radius + 0.1f));
    can.vel = Vector2Scale(dir, kCanSpeed);
    can.height = kCanLaunchHeight;
    can.vy = 0.5f * kCanGravity * (d / kCanSpeed);
    can.life = 5.0f;
    w.projectiles.push_back(can);
}

// --- the rusher --------------------------------------------------------------
// Sees you, picks up speed, and does not stop. Thirty health: it dies to one swing
// and a bit, but it will be on you before you have finished the swing.

static void UpdateWinkelwagen(World& w, Enemy& e, float dist, bool los, float dt) {
    const EnemyStats& s = StatsFor(e.kind);

    switch (e.state) {
        case EnemyState::Idle:
            if (Notices(w, e, dist)) e.state = EnemyState::Chase;
            break;

        case EnemyState::Chase:
            if (dist <= s.attackRange + kPlayerRadius && e.cooldown <= 0.0f && los &&
                !w.player.dead()) {
                BeginAttack(e, 1);
                break;
            }
            Step(w, e, Vector2Subtract(w.player.pos, e.pos), dt);
            break;

        case EnemyState::Attack:
            e.stateTimer += dt;
            if (e.shotsLeft > 0 && e.stateTimer >= 0.16f) {
                e.shotsLeft = 0;
                if (Vector2Distance(e.pos, w.player.pos) <= s.attackRange + kPlayerRadius + 0.2f) {
                    PlayerDamage(w, s.damage);
                }
            }
            if (e.stateTimer >= 0.42f) EndAttack(e);
            break;

        default:
            break;
    }
}

// --- the zoner ---------------------------------------------------------------
// Wants to stand seven metres away with a shelf of soup behind him. Closes if you
// run, backs off if you charge, sidesteps otherwise so the cans come from a
// different angle each time.

static void UpdateVakkenvuller(World& w, Enemy& e, float dist, bool los, float dt) {
    const EnemyStats& s = StatsFor(e.kind);
    const Vector2 toPlayer = Vector2Subtract(w.player.pos, e.pos);

    switch (e.state) {
        case EnemyState::Idle:
            if (Notices(w, e, dist)) e.state = EnemyState::Chase;
            break;

        case EnemyState::Chase: {
            if (los && dist < s.attackRange && e.cooldown <= 0.0f && !w.player.dead()) {
                BeginAttack(e, 1);
                break;
            }

            e.strafeTimer -= dt;
            if (e.strafeTimer <= 0.0f) {
                e.strafeTimer = 1.0f + (float)GetRandomValue(0, 120) / 100.0f;
                e.strafe = Vector2Scale(e.strafe, -1.0f);
            }

            Vector2 wish;
            if (dist > 8.5f || !los) {
                wish = toPlayer;                                   // close the gap
            } else if (dist < 4.5f) {
                wish = Vector2Scale(toPlayer, -1.0f);              // too close, back off
            } else {
                const Vector2 side = {-toPlayer.y, toPlayer.x};    // hold the range, sidestep
                wish = Vector2Scale(side, e.strafe.x);
            }
            const Vector2 before = e.pos;
            Step(w, e, wish, dt);
            if (Vector2Distance(before, e.pos) < s.speed * dt * 0.4f) {
                e.strafeTimer = 0.0f;   // walked into a shelf: try the other way
            }
            break;
        }

        case EnemyState::Attack:
            e.stateTimer += dt;
            if (e.shotsLeft > 0 && e.stateTimer >= 0.34f) {   // the wind-up you can read
                e.shotsLeft = 0;
                ThrowCan(w, e, 0.0f);
                PlaySfxAt(Sfx::CanThrow, dist);
            }
            if (e.stateTimer >= 0.62f) EndAttack(e);
            break;

        default:
            break;
    }
}

// --- the turret --------------------------------------------------------------
// Barely moves. Paints you with a targeting line for a beat before the burst, so
// standing in an open aisle is a decision and not an accident.

static void UpdateZelfscanner(World& w, Enemy& e, float dist, bool los, float dt) {
    const EnemyStats& s = StatsFor(e.kind);
    constexpr float kWindup = 1.0f;

    switch (e.state) {
        case EnemyState::Idle:
            if (Notices(w, e, dist)) e.state = EnemyState::Chase;
            break;

        case EnemyState::Chase:
            if (los && dist < s.attackRange && e.cooldown <= 0.0f && !w.player.dead()) {
                BeginAttack(e, 3);
                FreezeAim(w, e);
                break;
            }
            if (dist > 7.0f) Step(w, e, Vector2Subtract(w.player.pos, e.pos), dt);
            break;

        case EnemyState::Attack:
            e.stateTimer += dt;

            if (!los || w.player.dead()) {   // you broke the beam: it has to start again
                EndAttack(e);
                e.cooldown = 0.6f;
                break;
            }

            if (e.stateTimer < kWindup) {
                e.aimBeam = 1.0f;
                break;
            }

            e.aimBeam = 0.0f;
            e.shotTimer -= dt;
            if (e.shotsLeft > 0 && e.shotTimer <= 0.0f) {
                e.shotsLeft--;
                e.shotTimer = 0.14f;
                e.beam = 0.09f;
                // A miss is still loud: the beam and the shriek fire either way.
                if (los && FrozenRayHits(w, e, s.attackRange + 1.0f)) {
                    PlayerDamage(w, s.damage);
                }
                PlaySfxAt(Sfx::EnemyShoot, dist);
            }
            if (e.shotsLeft <= 0 && e.shotTimer <= 0.0f) EndAttack(e);
            break;

        default:
            break;
    }
}

// --- the marksman --------------------------------------------------------------
// A beveiliger who never clocked out. Closes to mid range, sidesteps while his
// sidearm cools, then paints you for half a second and fires one frozen-ray shot —
// the zelfscanner's mechanic on legs.

static void UpdateBeveiliger(World& w, Enemy& e, float dist, bool los, float dt) {
    const EnemyStats& s = StatsFor(e.kind);
    constexpr float kWindup = 0.5f;

    switch (e.state) {
        case EnemyState::Idle:
            if (Notices(w, e, dist)) e.state = EnemyState::Chase;
            break;

        case EnemyState::Chase: {
            if (los && dist < s.attackRange && e.cooldown <= 0.0f && !w.player.dead()) {
                BeginAttack(e, 1);
                FreezeAim(w, e);
                break;
            }

            e.strafeTimer -= dt;
            if (e.strafeTimer <= 0.0f) {
                e.strafeTimer = 0.8f + (float)GetRandomValue(0, 100) / 100.0f;
                e.strafe = Vector2Scale(e.strafe, -1.0f);
            }

            Vector2 wish;
            const Vector2 toPlayer = Vector2Subtract(w.player.pos, e.pos);
            if (dist > 6.0f || !los) {
                wish = toPlayer;                                   // advance
            } else {
                const Vector2 side = {-toPlayer.y, toPlayer.x};    // hold and sidestep
                wish = Vector2Scale(side, e.strafe.x);
            }
            const Vector2 before = e.pos;
            Step(w, e, wish, dt);
            if (Vector2Distance(before, e.pos) < s.speed * dt * 0.4f) {
                e.strafeTimer = 0.0f;   // walked into a shelf: try the other way
            }
            break;
        }

        case EnemyState::Attack:
            e.stateTimer += dt;

            if (!los || w.player.dead()) {   // you broke his line: he re-aims
                EndAttack(e);
                e.cooldown = 0.8f;
                break;
            }

            if (e.stateTimer < kWindup) {
                e.aimBeam = 1.0f;
                break;
            }

            e.aimBeam = 0.0f;
            if (e.shotsLeft > 0) {
                e.shotsLeft = 0;
                e.beam = 0.09f;
                if (los && FrozenRayHits(w, e, s.attackRange + 1.0f)) {
                    PlayerDamage(w, s.damage);
                }
                PlaySfxAt(Sfx::GuardShot, dist);
            }
            if (e.stateTimer >= kWindup + 0.15f) EndAttack(e);
            break;

        default:
            break;
    }
}

// --- the boss ------------------------------------------------------------------
// De Bedrijfsleider. Alternates on a timer between charging you down like a
// winkelwagen with a salary and standing to hurl fans of soup cans. Level 3's
// keycard is in his pocket; see EnemyHurt.

constexpr float kBossChargeTime = 5.0f;
constexpr float kBossBarrageTime = 3.2f;
constexpr float kBossBarrageCooldown = 1.4f;

static void BossIntro(World& w, Enemy& e) {
    w.Message("DE BEDRIJFSLEIDER: 'WIJ SLUITEN NOOIT.'");
    PlaySfxAt(Sfx::AlertBoss, Vector2Distance(e.pos, w.player.pos));
    e.phase = 0;
    e.phaseTimer = kBossChargeTime;
}

static void UpdateBedrijfsleider(World& w, Enemy& e, float dist, bool los, float dt) {
    const EnemyStats& s = StatsFor(e.kind);

    switch (e.state) {
        case EnemyState::Idle:
            if (Notices(w, e, dist)) {
                e.state = EnemyState::Chase;
                BossIntro(w, e);
            }
            break;

        case EnemyState::Chase:
            e.phaseTimer -= dt;
            if (e.phaseTimer <= 0.0f) {
                e.phase = 1 - e.phase;
                e.phaseTimer = (e.phase == 0) ? kBossChargeTime : kBossBarrageTime;
            }

            if (e.phase == 0) {
                // Charge: winkelwagen logic, at scale.
                if (dist <= s.attackRange + kPlayerRadius && e.cooldown <= 0.0f && los &&
                    !w.player.dead()) {
                    BeginAttack(e, 1);
                    break;
                }
                Step(w, e, Vector2Subtract(w.player.pos, e.pos), dt);
            } else if (los && e.cooldown <= 0.0f && !w.player.dead()) {
                // Barrage: stand and throw. The fan launches from the Attack state.
                BeginAttack(e, 3);
            }
            break;

        case EnemyState::Attack:
            e.stateTimer += dt;
            if (e.phase == 0) {
                // The contact hit, exactly like the trolley's.
                if (e.shotsLeft > 0 && e.stateTimer >= 0.16f) {
                    e.shotsLeft = 0;
                    if (Vector2Distance(e.pos, w.player.pos) <=
                        s.attackRange + kPlayerRadius + 0.2f) {
                        PlayerDamage(w, s.damage);
                    }
                }
                if (e.stateTimer >= 0.42f) EndAttack(e);
            } else {
                // The fan: -15deg / 0 / +15deg, all at once, after a readable windup.
                if (e.shotsLeft > 0 && e.stateTimer >= 0.3f) {
                    e.shotsLeft = 0;
                    for (const float off : {-15.0f * DEG2RAD, 0.0f, 15.0f * DEG2RAD}) {
                        ThrowCan(w, e, off);
                    }
                    PlaySfxAt(Sfx::CanThrow, dist);
                }
                if (e.stateTimer >= 0.6f) {
                    EndAttack(e);
                    e.cooldown = kBossBarrageCooldown;
                }
            }
            break;

        default:
            break;
    }
}

// --- shared frame ------------------------------------------------------------

static void KeepApart(World& w) {
    // Nobody shares a tile with anybody. O(n^2) over two dozen shoppers is free.
    for (size_t i = 0; i < w.enemies.size(); i++) {
        Enemy& a = w.enemies[i];
        if (!a.alive()) continue;

        for (size_t j = i + 1; j < w.enemies.size(); j++) {
            Enemy& b = w.enemies[j];
            if (!b.alive()) continue;

            const float minDist = StatsFor(a.kind).radius + StatsFor(b.kind).radius;
            Vector2 apart = Vector2Subtract(a.pos, b.pos);
            float dist = Vector2Length(apart);
            if (dist >= minDist) continue;

            if (dist < 1e-4f) {   // exactly stacked: shove them off an arbitrary axis
                apart = {1.0f, 0.0f};
                dist = 1.0f;
            }
            const Vector2 push = Vector2Scale(apart, (minDist - dist) * 0.5f / dist);

            const Vector2 aTo = Vector2Add(a.pos, push);
            const Vector2 bTo = Vector2Subtract(b.pos, push);
            if (w.map.Fits(aTo, StatsFor(a.kind).radius)) a.pos = aTo;
            if (w.map.Fits(bTo, StatsFor(b.kind).radius)) b.pos = bTo;
        }
    }
}

void EnemiesUpdate(World& w, float dt) {
    for (Enemy& e : w.enemies) {
        e.hurtFlash = fmaxf(0.0f, e.hurtFlash - dt * 6.0f);
        e.beam = fmaxf(0.0f, e.beam - dt);
        e.cooldown = fmaxf(0.0f, e.cooldown - dt);

        if (e.state == EnemyState::Dead) continue;
        if (e.state == EnemyState::Dying) {
            e.stateTimer += dt;
            if (e.stateTimer >= 0.42f) e.state = EnemyState::Dead;
            continue;
        }

        const float dist = Vector2Distance(e.pos, w.player.pos);
        const bool los = w.map.LineOfSight(e.pos, w.player.pos);

        switch (e.kind) {
            case EnemyKind::Winkelwagen:    UpdateWinkelwagen(w, e, dist, los, dt); break;
            case EnemyKind::Vakkenvuller:   UpdateVakkenvuller(w, e, dist, los, dt); break;
            case EnemyKind::Zelfscanner:    UpdateZelfscanner(w, e, dist, los, dt); break;
            case EnemyKind::Beveiliger:     UpdateBeveiliger(w, e, dist, los, dt); break;
            case EnemyKind::Bedrijfsleider: UpdateBedrijfsleider(w, e, dist, los, dt); break;
        }
    }

    KeepApart(w);
}

// The vuurwerkpijl's blast: linear falloff over the radius, and it does not care
// who is standing in it — enemies and the shooter alike. Walls shield.
static void ExplodeRocket(World& w, Vector2 at) {
    for (Enemy& e : w.enemies) {
        if (!e.alive()) continue;
        const float d = Vector2Distance(e.pos, at);
        if (d > kRocketRadius || !w.map.LineOfSight(at, e.pos)) continue;
        EnemyHurt(w, e, (int)Lerp((float)kRocketDamageMax, (float)kRocketDamageMin,
                                  d / kRocketRadius));
    }
    const float pd = Vector2Distance(w.player.pos, at);
    if (pd <= kRocketRadius && w.map.LineOfSight(at, w.player.pos)) {
        PlayerDamage(w, (int)Lerp((float)kRocketDamageMax, (float)kRocketDamageMin,
                                  pd / kRocketRadius));
    }
    w.shake = fminf(1.2f, w.shake + 0.6f);
    w.player.muzzleFlash = fmaxf(w.player.muzzleFlash, 0.9f);   // the aisle lights up
    PlaySfxAt(Sfx::Explosion, pd);
}

static bool RocketUpdate(World& w, Projectile& p, float dt) {
    // Flat flight. Detonates on the first wall, the first shopper, or timeout.
    const Vector2 next = Vector2Add(p.pos, Vector2Scale(p.vel, dt));
    if (w.map.SolidAt(next) || p.life <= 0.0f) {
        ExplodeRocket(w, p.pos);
        return false;
    }
    p.pos = next;

    for (const Enemy& e : w.enemies) {
        if (!e.alive()) continue;
        if (Vector2Distance(p.pos, e.pos) < StatsFor(e.kind).radius + 0.2f) {
            ExplodeRocket(w, p.pos);
            return false;
        }
    }
    return true;
}

static bool SoupCanUpdate(World& w, Projectile& p, float dt) {
    p.vy -= kCanGravity * dt;
    p.height += p.vy * dt;

    const Vector2 next = Vector2Add(p.pos, Vector2Scale(p.vel, dt));
    if (w.map.SolidAt(next)) return false;   // a can does not go through a shelf
    p.pos = next;

    if (p.height <= 0.05f) return false;     // it fell short
    if (Vector2Distance(p.pos, w.player.pos) < kPlayerRadius + 0.16f) {
        PlayerDamage(w, StatsFor(EnemyKind::Vakkenvuller).damage);
        return false;
    }
    return true;
}

void ProjectilesUpdate(World& w, float dt) {
    for (Projectile& p : w.projectiles) {
        p.life -= dt;

        const bool alive = (p.kind == Projectile::Kind::Rocket)
                               ? RocketUpdate(w, p, dt)
                               : SoupCanUpdate(w, p, dt);
        if (!alive) p.life = 0.0f;
    }

    w.projectiles.erase(
        std::remove_if(w.projectiles.begin(), w.projectiles.end(),
                       [](const Projectile& p) { return p.life <= 0.0f; }),
        w.projectiles.end());
}

void EnemyHurt(World& w, Enemy& e, int damage) {
    if (!e.alive()) return;

    e.health -= damage;
    e.hurtFlash = 1.0f;

    if (e.health > 0) {
        if (e.state == EnemyState::Idle) {
            e.state = EnemyState::Chase;   // that got its attention
            if (e.kind == EnemyKind::Bedrijfsleider) BossIntro(w, e);
        }
        return;
    }

    e.state = EnemyState::Dying;
    e.stateTimer = 0.0f;
    e.aimBeam = 0.0f;
    e.beam = 0.0f;
    w.kills++;

    const float dist = Vector2Distance(e.pos, w.player.pos);
    switch (e.kind) {
        case EnemyKind::Winkelwagen:  PlaySfxAt(Sfx::DeathCart, dist); break;
        case EnemyKind::Vakkenvuller: PlaySfxAt(Sfx::DeathStocker, dist); break;
        case EnemyKind::Zelfscanner:  PlaySfxAt(Sfx::DeathScanner, dist); break;
        case EnemyKind::Beveiliger:   PlaySfxAt(Sfx::DeathGuard, dist); break;
        case EnemyKind::Bedrijfsleider:
            // The pass was in his pocket the whole time.
            PlaySfxAt(Sfx::DeathBoss, dist);
            w.pickups.push_back({PickupKind::Keycard, e.pos, false, 0.0f});
            w.Message("HIJ LAAT DE PAS VALLEN");
            break;
    }
}

void AlertEnemies(World& w, Vector2 from, float radius) {
    for (Enemy& e : w.enemies) {
        if (e.state != EnemyState::Idle) continue;
        if (Vector2Distance(e.pos, from) < radius) e.state = EnemyState::Chase;
    }
}
