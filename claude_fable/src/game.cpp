#include "game.h"

#include "assets.h"
#include "raymath.h"

#include <cmath>

// ---------------------------------------------------------------------------
// Enemy stat table (SPEC.md §6)
// ---------------------------------------------------------------------------
struct EnemyStats {
  float speed, damage, range, cooldown, notice;
};
static const EnemyStats kStats[3] = {
    // speed damage range cooldown notice
    { 4.0f, 10.0f, 0.62f, 1.0f, 13.0f },  // Winkelwagen: melee rusher
    { 1.8f, 15.0f, 10.0f, 2.0f, 11.0f },  // Vakkenvuller: soup-can zoner
    { 0.4f,  8.0f, 14.0f, 1.5f, 14.0f },  // Zelfscanner: hitscan turret
};

static constexpr float kEnemyRadius = 0.30f;
static constexpr float kEnemyHitRadius = 0.36f;  // generous for hitscan feel

// ---------------------------------------------------------------------------
// Collision / raycasts
// ---------------------------------------------------------------------------
bool TileSolid(const Game& g, int x, int y) {
  Tile t = g.map.at(x, y);
  if (t == Tile::Empty) return false;
  if (t == Tile::DoorKey) {
    for (const Door& d : g.doors)
      if (d.x == x && d.y == y) return d.open < 0.75f;
  }
  return true;  // walls and the exit door (it never opens; touching it wins)
}

bool CircleFits(const Game& g, Vector2 p, float r) {
  int x0 = (int)floorf(p.x - r), x1 = (int)floorf(p.x + r);
  int y0 = (int)floorf(p.y - r), y1 = (int)floorf(p.y + r);
  for (int ty = y0; ty <= y1; ty++) {
    for (int tx = x0; tx <= x1; tx++) {
      if (!TileSolid(g, tx, ty)) continue;
      float cx = Clamp(p.x, (float)tx, (float)tx + 1);
      float cy = Clamp(p.y, (float)ty, (float)ty + 1);
      float dx = p.x - cx, dy = p.y - cy;
      if (dx * dx + dy * dy < r * r) return false;
    }
  }
  return true;
}

// Axis-separated move so circles slide along walls.
static Vector2 MoveWithCollision(const Game& g, Vector2 p, Vector2 d, float r) {
  Vector2 np = p;
  if (CircleFits(g, Vector2{ p.x + d.x, np.y }, r)) np.x = p.x + d.x;
  if (CircleFits(g, Vector2{ np.x, p.y + d.y }, r)) np.y = p.y + d.y;
  return np;
}

float RaycastWall(const Game& g, Vector2 o, Vector2 dir, float maxDist) {
  int mx = (int)floorf(o.x), my = (int)floorf(o.y);
  float ddx = (dir.x == 0) ? 1e30f : fabsf(1.0f / dir.x);
  float ddy = (dir.y == 0) ? 1e30f : fabsf(1.0f / dir.y);
  int sx = dir.x < 0 ? -1 : 1, sy = dir.y < 0 ? -1 : 1;
  float tx = (dir.x < 0 ? (o.x - mx) : (mx + 1 - o.x)) * ddx;
  float ty = (dir.y < 0 ? (o.y - my) : (my + 1 - o.y)) * ddy;
  float d = 0;
  while (d <= maxDist) {
    if (tx < ty) { d = tx; tx += ddx; mx += sx; }
    else         { d = ty; ty += ddy; my += sy; }
    if (TileSolid(g, mx, my)) return d;
  }
  return maxDist;
}

bool LineOfSight(const Game& g, Vector2 a, Vector2 b) {
  float dist = Vector2Distance(a, b);
  if (dist < 0.001f) return true;
  Vector2 dir = Vector2Scale(Vector2Subtract(b, a), 1.0f / dist);
  return RaycastWall(g, a, dir, dist) >= dist - 0.001f;
}

void ShowMessage(Game& g, const char* text) {
  g.msg = text;
  g.msgT = 2.2f;
}

// ---------------------------------------------------------------------------
// Damage
// ---------------------------------------------------------------------------
static void DamagePlayer(Game& g, const Assets& A, float dmg) {
  Player& p = g.pl;
  float absorbed = fminf(p.armour, dmg * 0.5f);  // armour soaks 50% until gone
  p.armour -= absorbed;
  p.hp -= dmg - absorbed;
  p.hurtFlash = 0.5f;
  Sfx(A, A.sHurt);
  if (p.hp <= 0) {
    p.hp = 0;
    g.state = GameState::Dead;
    EnableCursor();
    Sfx(A, A.sLose);
  }
}

static void DamageEnemy(Game& g, const Assets& A, Enemy& e, float dmg) {
  e.hp -= dmg;
  e.hurtFlash = 0.18f;
  if (e.state == EnemyState::Idle) {  // getting shot wakes it up
    e.state = EnemyState::Active;
    e.lastSeen = g.pl.pos;
  }
  if (e.hp <= 0) {
    e.state = EnemyState::Dying;
    e.dieAnim = 0;
    g.kills++;
    Sfx(A, A.sEnemyDie);
  } else {
    Sfx(A, A.sEnemyHit);
  }
}

static bool EnemyTargetable(const Enemy& e) {
  return e.state == EnemyState::Idle || e.state == EnemyState::Active;
}

// ---------------------------------------------------------------------------
// Player weapons
// ---------------------------------------------------------------------------
static void AddTracer(Game& g, Vector3 a, Vector3 b, Color c) {
  g.tracers.push_back({ a, b, c, 0.08f });
}

static void FirePrijspistool(Game& g, const Assets& A) {
  Player& p = g.pl;
  float spread = (GetRandomValue(-1000, 1000) / 1000.0f) * cfg::PistSpreadDeg * DEG2RAD;
  Vector2 dir = Vector2Rotate(Vector2{ cosf(p.angle), sinf(p.angle) }, spread);

  float wallDist = RaycastWall(g, p.pos, dir, 60.0f);
  Enemy* best = nullptr;
  float bestT = wallDist;
  for (Enemy& e : g.enemies) {
    if (!EnemyTargetable(e)) continue;
    Vector2 rel = Vector2Subtract(e.pos, p.pos);
    float t = rel.x * dir.x + rel.y * dir.y;
    if (t < 0.05f || t > bestT) continue;
    float perp = fabsf(rel.x * dir.y - rel.y * dir.x);
    if (perp < kEnemyHitRadius) { best = &e; bestT = t; }
  }
  Vector2 muzzle = Vector2Add(p.pos, Vector2Scale(dir, 0.3f));
  Vector2 end = Vector2Add(p.pos, Vector2Scale(dir, bestT));
  AddTracer(g, Vector3{ muzzle.x, 0.42f, muzzle.y },
            Vector3{ end.x, 0.45f, end.y }, Color{ 255, 235, 130, 255 });
  if (best) DamageEnemy(g, A, *best, cfg::PistDamage);
}

static void StokbroodImpact(Game& g, const Assets& A) {
  Player& p = g.pl;
  Vector2 fwd{ cosf(p.angle), sinf(p.angle) };
  Enemy* best = nullptr;
  float bestDist = cfg::StokRange + kEnemyRadius;
  for (Enemy& e : g.enemies) {
    if (!EnemyTargetable(e)) continue;
    float dist = Vector2Distance(p.pos, e.pos);
    if (dist > bestDist) continue;
    Vector2 to = Vector2Normalize(Vector2Subtract(e.pos, p.pos));
    bool inCone = (to.x * fwd.x + to.y * fwd.y) > cosf(cfg::StokConeDeg * DEG2RAD);
    if ((inCone || dist < 0.6f) && LineOfSight(g, p.pos, e.pos)) {
      best = &e;
      bestDist = dist;
    }
  }
  if (best) DamageEnemy(g, A, *best, cfg::StokDamage);
}

// ---------------------------------------------------------------------------
// Doors / progression
// ---------------------------------------------------------------------------
static void WinLevel(Game& g, const Assets& A) {
  g.state = GameState::Won;
  EnableCursor();
  Sfx(A, A.sWin);
}

static void UseDoorTile(Game& g, const Assets& A, int tx, int ty, bool bump) {
  for (Door& d : g.doors) {
    if (d.x != tx || d.y != ty) continue;
    if (d.type == Tile::DoorExit) {
      WinLevel(g, A);
      return;
    }
    if (d.open > 0.01f) return;  // already opening
    if (bump && g.bumpMsgCd > 0) return;
    if (g.pl.hasKeycard) {
      d.opening = true;
      ShowMessage(g, "MAGAZIJN ONTGRENDELD");
      Sfx(A, A.sDoorOpen);
    } else {
      ShowMessage(g, "GESLOTEN - BEDRIJFSLEIDERSPAS VEREIST");
      Sfx(A, A.sLocked);
      g.bumpMsgCd = 1.2f;
    }
    return;
  }
}

static void UpdateDoors(Game& g, float dt) {
  for (Door& d : g.doors)
    if (d.opening && d.open < 1.0f) d.open = fminf(1.0f, d.open + dt / 0.8f);
}

// ---------------------------------------------------------------------------
// Player
// ---------------------------------------------------------------------------
static void UpdatePlayer(Game& g, const Assets& A, float dt) {
  Player& p = g.pl;

  // -- turning: arrows and/or mouse X --
  float turn = 0;
  if (IsKeyDown(KEY_RIGHT)) turn += cfg::TurnSpeed * dt;
  if (IsKeyDown(KEY_LEFT)) turn -= cfg::TurnSpeed * dt;
  if (g.captureGrace > 0) g.captureGrace--;
  else turn += GetMouseDelta().x * cfg::MouseSens;
  p.angle += turn;

  Vector2 fwd{ cosf(p.angle), sinf(p.angle) };
  Vector2 right{ -fwd.y, fwd.x };

  // -- movement with wall slide; alive enemies also block --
  Vector2 wish{ 0, 0 };
  if (IsKeyDown(KEY_W)) wish = Vector2Add(wish, fwd);
  if (IsKeyDown(KEY_S)) wish = Vector2Subtract(wish, fwd);
  if (IsKeyDown(KEY_D)) wish = Vector2Add(wish, right);
  if (IsKeyDown(KEY_A)) wish = Vector2Subtract(wish, right);
  bool moving = (wish.x != 0 || wish.y != 0);
  if (moving) {
    wish = Vector2Normalize(wish);
    Vector2 delta = Vector2Scale(wish, cfg::MoveSpeed * dt);
    Vector2 np = MoveWithCollision(g, p.pos, delta, cfg::PlayerRadius);
    // don't walk into (only out of) an alive enemy's circle
    for (const Enemy& e : g.enemies) {
      if (!EnemyTargetable(e)) continue;
      float dNew = Vector2Distance(np, e.pos);
      if (dNew < 0.52f && dNew < Vector2Distance(p.pos, e.pos)) { np = p.pos; break; }
    }
    p.pos = np;

    // bumping a door counts as using it (locked message / opens with card)
    Vector2 probe = Vector2Add(p.pos, Vector2Scale(wish, cfg::PlayerRadius + 0.30f));
    Tile pt = g.map.at((int)floorf(probe.x), (int)floorf(probe.y));
    if (pt == Tile::DoorKey || pt == Tile::DoorExit)
      UseDoorTile(g, A, (int)floorf(probe.x), (int)floorf(probe.y), true);
    if (g.state != GameState::Playing) return;
  }

  // -- weapon bob --
  p.bobAmount = Lerp(p.bobAmount, moving ? 1.0f : 0.0f, 10.0f * dt);
  if (moving) p.bobT += 7.5f * dt;

  // -- use key --
  if (IsKeyPressed(KEY_E)) {
    for (float reach : { 0.6f, 1.1f }) {
      Vector2 probe = Vector2Add(p.pos, Vector2Scale(fwd, reach));
      int tx = (int)floorf(probe.x), ty = (int)floorf(probe.y);
      Tile t = g.map.at(tx, ty);
      if (t == Tile::DoorKey || t == Tile::DoorExit) {
        UseDoorTile(g, A, tx, ty, false);
        break;
      }
    }
    if (g.state != GameState::Playing) return;
  }

  // -- weapon select / fire --
  if (IsKeyPressed(KEY_ONE)) p.weapon = 1;
  if (IsKeyPressed(KEY_TWO)) p.weapon = 2;

  p.fireCd -= dt;
  p.attackAnim -= dt;
  if (p.meleeT >= 0) {
    p.meleeT -= dt;
    if (p.meleeT < 0) StokbroodImpact(g, A);
  }

  bool fire = IsMouseButtonDown(MOUSE_BUTTON_LEFT) ||
              IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
  if (fire && p.fireCd <= 0) {
    if (p.weapon == 1) {
      p.fireCd = cfg::StokCd;
      p.attackDur = 0.35f;
      p.attackAnim = p.attackDur;
      p.meleeT = cfg::StokImpactDelay;
      Sfx(A, A.sSwing);
    } else if (p.ammo > 0) {
      p.fireCd = cfg::PistCd;
      p.attackDur = 0.16f;
      p.attackAnim = p.attackDur;
      p.ammo--;
      FirePrijspistool(g, A);
      Sfx(A, A.sShoot);
    } else {
      p.fireCd = 0.3f;
      ShowMessage(g, "GEEN LABELS");
      Sfx(A, A.sClick);
    }
  }
}

// ---------------------------------------------------------------------------
// Enemies
// ---------------------------------------------------------------------------
static bool EnemyMove(const Game& g, Enemy& e, Vector2 dir, float speed, float dt) {
  Vector2 delta = Vector2Scale(dir, speed * dt);
  Vector2 np = MoveWithCollision(g, e.pos, delta, kEnemyRadius);
  bool moved = fabsf(np.x - e.pos.x) + fabsf(np.y - e.pos.y) > 1e-5f;
  e.pos = np;
  if (moved) e.moved = true;
  return moved;
}

static void UpdateWinkelwagen(Game& g, const Assets& A, Enemy& e, float dt,
                              bool los, float dist) {
  const EnemyStats& st = kStats[(int)e.type];
  if (e.stagger > 0) { e.stagger -= dt; return; }

  // charge in a straight line; only re-aim periodically or when blocked
  e.repathT -= dt;
  if (e.repathT <= 0) {
    Vector2 to = Vector2Subtract(e.lastSeen, e.pos);
    if (Vector2Length(to) > 0.05f) e.moveDir = Vector2Normalize(to);
    e.repathT = 0.35f;
  }
  if (dist > 0.55f || !los) {
    if (!EnemyMove(g, e, e.moveDir, st.speed, dt)) e.repathT = 0;
  }
  if (los && dist < st.range && e.attackCd <= 0) {  // rammed the player
    DamagePlayer(g, A, st.damage);
    e.attackCd = st.cooldown;
    e.stagger = 0.4f;
    e.attackAnim = 0.3f;
  }
}

static void ThrowSoupCan(Game& g, const Assets& A, const Enemy& e) {
  Vector2 to = Vector2Subtract(g.pl.pos, e.pos);
  float dist = Vector2Length(to);
  if (dist < 0.1f) return;
  Vector2 dir = Vector2Scale(to, 1.0f / dist);
  const float speed = 5.2f;
  SoupCan c{};
  c.pos = Vector2Add(e.pos, Vector2Scale(dir, 0.35f));
  c.vel = Vector2Scale(dir, speed);
  c.flightT = dist / speed + 0.05f;
  c.arcH = 0.25f + 0.04f * dist;
  g.cans.push_back(c);
  Sfx(A, A.sThrow);
}

static void UpdateVakkenvuller(Game& g, const Assets& A, Enemy& e, float dt,
                               bool los, float dist) {
  const EnemyStats& st = kStats[(int)e.type];
  if (e.attackAnim > 0) {  // mid-throw; release the can on the attack frame
    float prev = e.attackAnim;
    e.attackAnim -= dt;
    if (prev > 0.25f && e.attackAnim <= 0.25f) ThrowSoupCan(g, A, e);
    return;
  }
  if (los && dist < st.range && e.attackCd <= 0) {
    e.attackAnim = 0.5f;
    e.attackCd = st.cooldown;
    return;
  }
  // the zoner: hold mid range, strafe, lob
  if (!los) {
    Vector2 to = Vector2Subtract(e.lastSeen, e.pos);
    if (Vector2Length(to) > 0.4f) EnemyMove(g, e, Vector2Normalize(to), st.speed, dt);
  } else if (dist > 7.0f) {
    EnemyMove(g, e, Vector2Normalize(Vector2Subtract(g.pl.pos, e.pos)), st.speed, dt);
  } else if (dist < 4.0f) {
    Vector2 away = Vector2Normalize(Vector2Subtract(e.pos, g.pl.pos));
    if (!EnemyMove(g, e, away, st.speed, dt)) e.strafeSign = -e.strafeSign;
  } else {
    Vector2 to = Vector2Normalize(Vector2Subtract(g.pl.pos, e.pos));
    Vector2 side{ -to.y * e.strafeSign, to.x * e.strafeSign };
    if (!EnemyMove(g, e, side, st.speed * 0.7f, dt)) e.strafeSign = -e.strafeSign;
  }
}

static void ZelfscannerShot(Game& g, const Assets& A, Enemy& e) {
  float spread = (GetRandomValue(-1000, 1000) / 1000.0f) * 2.2f * DEG2RAD;
  Vector2 dir = Vector2Rotate(
      Vector2Normalize(Vector2Subtract(g.pl.pos, e.pos)), spread);
  float wallDist = RaycastWall(g, e.pos, dir, 20.0f);
  Vector2 rel = Vector2Subtract(g.pl.pos, e.pos);
  float t = rel.x * dir.x + rel.y * dir.y;
  float perp = fabsf(rel.x * dir.y - rel.y * dir.x);
  bool hit = t > 0 && t < wallDist && perp < 0.33f;
  float endT = hit ? t : wallDist;
  Vector2 end = Vector2Add(e.pos, Vector2Scale(dir, endT));
  AddTracer(g, Vector3{ e.pos.x, 0.45f, e.pos.y },
            Vector3{ end.x, hit ? 0.5f : 0.45f, end.y }, Color{ 255, 60, 60, 255 });
  Sfx(A, A.sScanner);
  if (hit) DamagePlayer(g, A, kStats[(int)e.type].damage);
}

static void UpdateZelfscanner(Game& g, const Assets& A, Enemy& e, float dt,
                              bool los, float dist) {
  const EnemyStats& st = kStats[(int)e.type];
  if (los && dist < st.range) {
    e.losTimer = 0;
    if (e.burstLeft > 0) {
      e.burstT -= dt;
      if (e.burstT <= 0) {
        ZelfscannerShot(g, A, e);
        e.burstLeft--;
        e.burstT = 0.13f;
        if (e.burstLeft == 0) e.attackCd = st.cooldown;
      }
    } else if (e.attackCd <= 0) {  // wind up a 3-shot burst
      e.burstLeft = 3;
      e.burstT = 0.28f;
      e.attackAnim = 0.75f;
    }
  } else {
    e.burstLeft = 0;
    e.losTimer += dt;
    if (e.losTimer > 1.5f && Vector2Distance(e.pos, e.lastSeen) > 0.5f)
      EnemyMove(g, e, Vector2Normalize(Vector2Subtract(e.lastSeen, e.pos)), st.speed, dt);
  }
}

static void SeparateEnemies(Game& g) {
  for (size_t i = 0; i < g.enemies.size(); i++) {
    Enemy& a = g.enemies[i];
    if (!EnemyTargetable(a)) continue;
    for (size_t j = i + 1; j < g.enemies.size(); j++) {
      Enemy& b = g.enemies[j];
      if (!EnemyTargetable(b)) continue;
      float d = Vector2Distance(a.pos, b.pos);
      if (d >= 0.55f || d < 1e-4f) continue;
      Vector2 dir = Vector2Scale(Vector2Subtract(b.pos, a.pos), 1.0f / d);
      Vector2 push = Vector2Scale(dir, (0.55f - d) * 0.5f);
      Vector2 na = Vector2Subtract(a.pos, push);
      Vector2 nb = Vector2Add(b.pos, push);
      if (CircleFits(g, na, kEnemyRadius)) a.pos = na;
      if (CircleFits(g, nb, kEnemyRadius)) b.pos = nb;
    }
  }
}

static void UpdateEnemies(Game& g, const Assets& A, float dt) {
  for (Enemy& e : g.enemies) {
    e.moved = false;
    e.hurtFlash -= dt;
    if (e.state == EnemyState::Dying) {
      e.dieAnim += dt;
      if (e.dieAnim > 0.54f) e.state = EnemyState::Dead;
      continue;
    }
    if (e.state == EnemyState::Dead) continue;

    float dist = Vector2Distance(e.pos, g.pl.pos);
    bool los = LineOfSight(g, e.pos, g.pl.pos);
    const EnemyStats& st = kStats[(int)e.type];

    if (e.state == EnemyState::Idle) {
      if (los && dist < st.notice) {
        e.state = EnemyState::Active;
        e.lastSeen = g.pl.pos;
        e.repathT = 0;
      } else {
        continue;
      }
    }
    if (los) e.lastSeen = g.pl.pos;

    e.attackCd -= dt;
    if (e.type != EnemyType::Vakkenvuller) e.attackAnim -= dt;

    switch (e.type) {
      case EnemyType::Winkelwagen:  UpdateWinkelwagen(g, A, e, dt, los, dist); break;
      case EnemyType::Vakkenvuller: UpdateVakkenvuller(g, A, e, dt, los, dist); break;
      case EnemyType::Zelfscanner:  UpdateZelfscanner(g, A, e, dt, los, dist); break;
    }
    if (g.state != GameState::Playing) return;  // player died mid-update
    if (e.moved) e.walkAnim += dt;
  }
  SeparateEnemies(g);
}

// ---------------------------------------------------------------------------
// Soup cans, pickups, exit
// ---------------------------------------------------------------------------
static void UpdateCans(Game& g, const Assets& A, float dt) {
  for (SoupCan& c : g.cans) {
    c.t += dt;
    c.pos = Vector2Add(c.pos, Vector2Scale(c.vel, dt));
    if (TileSolid(g, (int)floorf(c.pos.x), (int)floorf(c.pos.y))) { c.dead = true; continue; }
    if (c.t >= c.flightT) { c.dead = true; continue; }
    if (Vector2Distance(c.pos, g.pl.pos) < 0.45f) {
      c.dead = true;
      DamagePlayer(g, A, kStats[(int)EnemyType::Vakkenvuller].damage);
      if (g.state != GameState::Playing) return;
    }
  }
  for (size_t i = g.cans.size(); i-- > 0;)
    if (g.cans[i].dead) g.cans.erase(g.cans.begin() + i);
}

static void CheckPickups(Game& g, const Assets& A) {
  Player& p = g.pl;
  for (Pickup& pk : g.pickups) {
    if (pk.taken || Vector2Distance(pk.pos, p.pos) > 0.55f) continue;
    bool consumed = true;
    switch (pk.type) {
      case PickupType::Appelflap:
        consumed = p.hp < cfg::MaxHealth;
        if (consumed) { p.hp = fminf(cfg::MaxHealth, p.hp + 25); ShowMessage(g, "APPELFLAP +25"); }
        break;
      case PickupType::Rookworst:
        consumed = p.hp < cfg::MaxHealth;
        if (consumed) { p.hp = fminf(cfg::MaxHealth, p.hp + 50); ShowMessage(g, "ROOKWORST +50"); }
        break;
      case PickupType::Labels:
        consumed = p.ammo < cfg::MaxAmmo;
        if (consumed) {
          p.ammo = (p.ammo + 20 > cfg::MaxAmmo) ? cfg::MaxAmmo : p.ammo + 20;
          ShowMessage(g, "+20 LABELS");
        }
        break;
      case PickupType::Bonuskaart:
        consumed = p.armour < cfg::MaxArmour;
        if (consumed) { p.armour = fminf(cfg::MaxArmour, p.armour + 50); ShowMessage(g, "BONUSKAART: +50 PANTSER"); }
        break;
      case PickupType::Keycard:
        p.hasKeycard = true;
        ShowMessage(g, "BEDRIJFSLEIDERSPAS GEVONDEN");
        break;
    }
    if (!consumed) continue;  // full health/ammo/armour: leave it (SPEC §7)
    pk.taken = true;
    p.pickupFlash = 0.30f;
    Sfx(A, pk.type == PickupType::Keycard ? A.sKeycard : A.sPickup);
  }
}

static void CheckExitTouch(Game& g, const Assets& A) {
  for (const Door& d : g.doors) {
    if (d.type != Tile::DoorExit) continue;
    Vector2 centre{ d.x + 0.5f, d.y + 0.5f };
    if (Vector2Distance(centre, g.pl.pos) < 0.95f) { WinLevel(g, A); return; }
  }
}

// ---------------------------------------------------------------------------
// Frame update
// ---------------------------------------------------------------------------
void UpdateGame(Game& g, const Assets& A, float dt) {
  g.time += dt;

  // freezer strip-light flicker, with an occasional hard dropout
  g.flicker = 0.82f + 0.18f * sinf(g.time * 7.3f) * sinf(g.time * 3.1f);
  if (fmodf(g.time, 4.7f) < 0.07f) g.flicker *= 0.4f;

  g.msgT -= dt;
  g.bumpMsgCd -= dt;
  g.pl.hurtFlash -= dt;
  g.pl.pickupFlash -= dt;
  for (Tracer& t : g.tracers) t.t -= dt;
  for (size_t i = g.tracers.size(); i-- > 0;)
    if (g.tracers[i].t <= 0) g.tracers.erase(g.tracers.begin() + i);

  UpdatePlayer(g, A, dt);
  if (g.state != GameState::Playing) return;
  UpdateDoors(g, dt);
  UpdateEnemies(g, A, dt);
  if (g.state != GameState::Playing) return;
  UpdateCans(g, A, dt);
  if (g.state != GameState::Playing) return;
  CheckPickups(g, A);
  CheckExitTouch(g, A);
}
