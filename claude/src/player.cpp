#include "player.h"

#include <cmath>

#include "audio.h"
#include "raymath.h"
#include "world.h"

static float WeaponAnimTotal(WeaponId id) {
    switch (id) {
        case WeaponId::Stokbrood:       return kMeleeAnim;
        case WeaponId::Prijspistool:    return kGunAnim;
        case WeaponId::Statiegeldkanon: return kScatterAnim;
        case WeaponId::Vuurwerkpijl:    return kRocketAnim;
    }
    return kGunAnim;
}

int Player::weaponFrame() const {
    if (fireAnim <= 0.0f) return 0;
    const float t = 1.0f - fireAnim / WeaponAnimTotal(weapon);
    return (t < 0.45f) ? 1 : 2;   // 1 = windup, 2 = impact
}

// The selected weapon's ammo pool, or null for the stokbrood (bread is free).
int* PlayerAmmoPool(Player& p, WeaponId id) {
    switch (id) {
        case WeaponId::Prijspistool:    return &p.ammo;
        case WeaponId::Statiegeldkanon: return &p.flessen;
        case WeaponId::Vuurwerkpijl:    return &p.vuurwerk;
        default:                        return nullptr;
    }
}

// --- doors -------------------------------------------------------------------

static Door* DoorNear(World& w, float reach, bool needFacing) {
    Door* best = nullptr;
    float bestDist = reach;
    for (Door& d : w.map.doors) {
        const Vector2 centre = {d.x + 0.5f, d.y + 0.5f};
        const float dist = Vector2Distance(w.player.pos, centre);
        if (dist >= bestDist) continue;
        if (needFacing && dist > 0.75f) {
            const Vector2 to = Vector2Normalize(Vector2Subtract(centre, w.player.pos));
            if (Vector2DotProduct(to, w.player.forward()) < 0.35f) continue;
        }
        bestDist = dist;
        best = &d;
    }
    return best;
}

static void TryOpen(World& w, Door& d) {
    if (d.isExit) return;   // the loading dock is handled by walking into it
    if (d.open) return;

    if (w.player.hasKeycard) {
        d.open = true;
        PlaySfx(Sfx::DoorOpen);
        w.Message("PAS GEACCEPTEERD - MAGAZIJN OPEN");
        AlertEnemies(w, {d.x + 0.5f, d.y + 0.5f}, 9.0f);
    } else if (w.player.lockedNag <= 0.0f) {
        w.player.lockedNag = 1.4f;
        PlaySfx(Sfx::DoorLocked);
        w.Message("OP SLOT - JE HEBT DE PAS VAN DE BEDRIJFSLEIDER NODIG");
    }
}

static void UpdateDoors(World& w, float dt) {
    Player& p = w.player;
    p.lockedNag = fmaxf(0.0f, p.lockedNag - dt);

    // Contact: bumping the magazijn door opens it if you have the pass, and tells you
    // off if you do not. Touching the loading dock door is how you win.
    if (Door* touched = DoorNear(w, 1.05f, false)) {
        if (touched->isExit) {
            w.escaped = true;
            return;
        }
        TryOpen(w, *touched);
    }

    if (IsKeyPressed(KEY_E)) {
        if (Door* used = DoorNear(w, 1.8f, true)) {
            TryOpen(w, *used);
        }
    }
}

// --- shooting ----------------------------------------------------------------

static void SwingStokbrood(World& w) {
    const Player& p = w.player;
    const Vector2 fwd = p.forward();
    bool connected = false;

    for (Enemy& e : w.enemies) {
        if (!e.alive()) continue;
        const Vector2 to = Vector2Subtract(e.pos, p.pos);
        const float dist = Vector2Length(to);
        if (dist > kMeleeRange + StatsFor(e.kind).radius) continue;
        if (dist > 0.01f) {
            const float cosAngle = Vector2DotProduct(Vector2Scale(to, 1.0f / dist), fwd);
            if (cosAngle < cosf(kMeleeArc)) continue;
        }
        if (!w.map.LineOfSight(p.pos, e.pos)) continue;

        EnemyHurt(w, e, kMeleeDamage);
        connected = true;
    }

    PlaySfx(Sfx::Swing);
    if (connected) PlaySfx(Sfx::Hit, 0.9f);
    AlertEnemies(w, p.pos, kMeleeNoiseRange);
}

// One hitscan ray from the player at `angle`. Stops at the first thing it meets,
// shelf or shopper; returns the shopper if that came first.
static Enemy* HitscanRay(World& w, float angle) {
    const Player& p = w.player;
    const Vector2 dir = {cosf(angle), sinf(angle)};

    float nearest = w.map.RayToWall(p.pos, dir, kGunRange);
    Enemy* hit = nullptr;

    for (Enemy& e : w.enemies) {
        if (!e.alive()) continue;
        const Vector2 toEnemy = Vector2Subtract(e.pos, p.pos);
        const float along = Vector2DotProduct(toEnemy, dir);
        if (along < 0.0f) continue;

        const float radius = StatsFor(e.kind).radius + 0.1f;
        const float perp2 = Vector2DotProduct(toEnemy, toEnemy) - along * along;
        if (perp2 > radius * radius) continue;

        const float entry = fmaxf(0.0f, along - sqrtf(radius * radius - perp2));
        if (entry < nearest) {
            nearest = entry;
            hit = &e;
        }
    }
    return hit;
}

static float SpreadAngle(float base, float spread) {
    return base + ((float)GetRandomValue(-100, 100) / 100.0f) * spread;
}

static void FirePrijspistool(World& w) {
    Player& p = w.player;
    Enemy* hit = HitscanRay(w, SpreadAngle(p.yaw, kGunSpread));

    p.ammo--;
    p.muzzleFlash = 1.0f;
    PlaySfx(Sfx::Shot);
    if (hit) {
        EnemyHurt(w, *hit, kGunDamage);
        PlaySfx(Sfx::Hit, 0.9f);
    }
    AlertEnemies(w, p.pos, kGunNoiseRange);
}

static void FireStatiegeldkanon(World& w) {
    Player& p = w.player;
    bool connected = false;
    for (int i = 0; i < kScatterPellets; i++) {
        if (Enemy* hit = HitscanRay(w, SpreadAngle(p.yaw, kScatterSpread))) {
            EnemyHurt(w, *hit, kScatterDamage);
            connected = true;
        }
    }

    p.flessen--;
    p.muzzleFlash = 1.0f;
    PlaySfx(Sfx::Scattergun);
    if (connected) PlaySfx(Sfx::Hit, 0.9f);
    AlertEnemies(w, p.pos, kGunNoiseRange);
}

static void FireVuurwerkpijl(World& w) {
    Player& p = w.player;
    const Vector2 fwd = p.forward();

    Projectile r;
    r.kind = Projectile::Kind::Rocket;
    r.ownerIsPlayer = true;
    r.pos = Vector2Add(p.pos, Vector2Scale(fwd, kPlayerRadius + 0.15f));
    r.vel = Vector2Scale(fwd, kRocketSpeed);
    r.height = kRocketLaunchH;
    r.life = 5.0f;
    w.projectiles.push_back(r);

    p.vuurwerk--;
    p.muzzleFlash = 0.7f;
    PlaySfx(Sfx::RocketLaunch);
    AlertEnemies(w, p.pos, kGunNoiseRange);
}

static void SelectWeapon(Player& p, WeaponId id) {
    if (!p.hasWeapon[(int)id] || p.weapon == id) return;
    p.weapon = id;
    PlaySfx(Sfx::WeaponSwitch, 0.8f);
}

static void UpdateWeapon(World& w, float dt) {
    Player& p = w.player;

    if (IsKeyPressed(KEY_ONE)) SelectWeapon(p, WeaponId::Stokbrood);
    if (IsKeyPressed(KEY_TWO)) SelectWeapon(p, WeaponId::Prijspistool);
    if (IsKeyPressed(KEY_THREE)) SelectWeapon(p, WeaponId::Statiegeldkanon);
    if (IsKeyPressed(KEY_FOUR)) SelectWeapon(p, WeaponId::Vuurwerkpijl);

    p.fireCooldown = fmaxf(0.0f, p.fireCooldown - dt);
    p.fireAnim = fmaxf(0.0f, p.fireAnim - dt);

    const bool firing = IsMouseButtonDown(MOUSE_BUTTON_LEFT) ||
                        IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    if (!firing || p.fireCooldown > 0.0f) return;

    if (p.weapon == WeaponId::Stokbrood) {
        p.fireCooldown = kMeleeCooldown;
        p.fireAnim = kMeleeAnim;
        SwingStokbrood(w);
        return;
    }

    int* pool = PlayerAmmoPool(p, p.weapon);
    if (*pool <= 0) {
        p.fireCooldown = kGunCooldown;
        PlaySfx(Sfx::DryFire, 0.7f);
        return;
    }

    switch (p.weapon) {
        case WeaponId::Prijspistool:
            p.fireCooldown = kGunCooldown;
            p.fireAnim = kGunAnim;
            FirePrijspistool(w);
            break;
        case WeaponId::Statiegeldkanon:
            p.fireCooldown = kScatterCooldown;
            p.fireAnim = kScatterAnim;
            FireStatiegeldkanon(w);
            break;
        case WeaponId::Vuurwerkpijl:
            p.fireCooldown = kRocketCooldown;
            p.fireAnim = kRocketAnim;
            FireVuurwerkpijl(w);
            break;
        default:
            break;
    }
}

// --- movement ----------------------------------------------------------------

// Can the player stand at `to`? Walls are absolute. A living body blocks you too, but
// only in the direction that would push you further into it: if you are somehow
// already overlapping one, you can always walk back out.
static bool WalkableForPlayer(const World& w, Vector2 from, Vector2 to) {
    if (!w.map.Fits(to, kPlayerRadius)) return false;

    for (const Enemy& e : w.enemies) {
        if (!e.alive()) continue;
        const float minDist = kPlayerRadius + StatsFor(e.kind).radius;
        const float after = Vector2Distance(to, e.pos);
        if (after >= minDist) continue;
        if (after < Vector2Distance(from, e.pos)) return false;
    }
    return true;
}

static void UpdateMovement(World& w, float dt) {
    Player& p = w.player;

    p.yaw += GetMouseDelta().x * kMouseSens;
    if (IsKeyDown(KEY_LEFT)) p.yaw -= kTurnSpeed * dt;
    if (IsKeyDown(KEY_RIGHT)) p.yaw += kTurnSpeed * dt;

    Vector2 wish{};
    if (IsKeyDown(KEY_W)) wish = Vector2Add(wish, p.forward());
    if (IsKeyDown(KEY_S)) wish = Vector2Subtract(wish, p.forward());
    if (IsKeyDown(KEY_D)) wish = Vector2Add(wish, p.right());
    if (IsKeyDown(KEY_A)) wish = Vector2Subtract(wish, p.right());

    const bool moving = Vector2Length(wish) > 0.01f;
    if (moving) {
        const Vector2 step = Vector2Scale(Vector2Normalize(wish), kMoveSpeed * dt);

        // Walls and bodies both stop you, one axis at a time so you slide along either.
        // A blocked axis never rejects a move that increases the gap, so nothing can
        // wedge you permanently inside a trolley.
        const Vector2 tryX = {p.pos.x + step.x, p.pos.y};
        if (WalkableForPlayer(w, p.pos, tryX)) p.pos = tryX;
        const Vector2 tryY = {p.pos.x, p.pos.y + step.y};
        if (WalkableForPlayer(w, p.pos, tryY)) p.pos = tryY;
    }

    p.bobAmount = Lerp(p.bobAmount, moving ? 1.0f : 0.0f, fminf(1.0f, dt * 9.0f));
    if (moving) p.bobPhase += dt * 8.5f;
}

// --- frame -------------------------------------------------------------------

void PlayerUpdate(World& w, float dt) {
    Player& p = w.player;

    p.hurtFlash = fmaxf(0.0f, p.hurtFlash - dt * 3.6f);
    p.muzzleFlash = fmaxf(0.0f, p.muzzleFlash - dt * 9.0f);

    if (p.dead()) {
        p.deathFall = fminf(1.0f, p.deathFall + dt * 2.2f);
        return;
    }
    if (w.escaped) return;

    UpdateMovement(w, dt);
    UpdateWeapon(w, dt);
    UpdateDoors(w, dt);
}

void PlayerDamage(World& w, int damage) {
    Player& p = w.player;
    if (p.dead() || w.escaped || damage <= 0) return;

    // Armour eats half of everything until it runs out.
    int absorbed = 0;
    if (p.armour > 0) {
        absorbed = damage / 2;
        if (absorbed > p.armour) absorbed = p.armour;
        p.armour -= absorbed;
    }
    p.health -= (damage - absorbed);
    p.hurtFlash = 1.0f;
    w.shake = fminf(1.2f, w.shake + damage * 0.035f);

    if (p.health <= 0) {
        p.health = 0;
        PlaySfx(Sfx::Died);
    } else {
        PlaySfx(Sfx::Hurt, 0.85f);
    }
}
