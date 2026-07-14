#include "render.h"

#include "raymath.h"
#include "rlgl.h"

#include <algorithm>
#include <cmath>
#include <vector>

// ---------------------------------------------------------------------------
// Doom-style shading: a warm store light, cold flickering freezer light, dim
// magazijn light, all multiplied by distance falloff toward black.
// ---------------------------------------------------------------------------
static Color Shade(const Game& g, Zone z, float dist) {
  float f = 1.0f - dist / cfg::FogDist;
  if (f < 0.07f) f = 0.07f;
  float r = 1.00f, gr = 0.96f, b = 0.88f;
  if (z == Zone::Freezer) {
    r = 0.50f; gr = 0.68f; b = 1.00f;
    f *= 0.55f * g.flicker;
  } else if (z == Zone::Magazijn) {
    r = 1.00f; gr = 0.82f; b = 0.62f;
    f *= 0.80f;
  }
  return Color{ (unsigned char)(255 * r * f), (unsigned char)(255 * gr * f),
                (unsigned char)(255 * b * f), 255 };
}

static float DistTo(Vector2 cam, float x, float z) {
  float dx = x - cam.x, dz = z - cam.y;
  return sqrtf(dx * dx + dz * dz);
}

// ---------------------------------------------------------------------------
// Quad batching: faces are bucketed per texture, then emitted through rlgl.
// ---------------------------------------------------------------------------
struct QuadV {
  Vector3 p[4];
  Vector2 uv[4];
  Color c[4];
};

// One bucket per Tile texture (walls/doors), plus floor and ceiling.
static constexpr int kFloorBucket = 9;
static constexpr int kCeilBucket = 10;
static constexpr int kBuckets = 11;

static Texture2D BucketTexture(const Assets& A, int i) {
  if (i == kFloorBucket) return A.floor;
  if (i == kCeilBucket) return A.ceiling;
  return WallTexture(A, (Tile)i);
}

// Vertical wall face between bottom corners a and b, u along a->b.
static void EmitFace(std::vector<QuadV>& out, Vector2 a, Vector2 b,
                     float yBot, float yTop, float vTop, float vBot,
                     Color ca, Color cb) {
  QuadV q;
  q.p[0] = { a.x, yTop, a.y }; q.uv[0] = { 0, vTop }; q.c[0] = ca;
  q.p[1] = { a.x, yBot, a.y }; q.uv[1] = { 0, vBot }; q.c[1] = ca;
  q.p[2] = { b.x, yBot, b.y }; q.uv[2] = { 1, vBot }; q.c[2] = cb;
  q.p[3] = { b.x, yTop, b.y }; q.uv[3] = { 1, vTop }; q.c[3] = cb;
  out.push_back(q);
}

static bool Walkable(const Map& m, int x, int y) {
  Tile t = m.at(x, y);
  return t == Tile::Empty || t == Tile::DoorKey || t == Tile::DoorExit;
}

static void EmitWallTile(const Game& g, std::vector<QuadV>* buckets,
                         Vector2 cam, int x, int y, Tile t) {
  // A door tile slides upward as it opens; plain walls have yBot 0.
  float yBot = 0, vTop = 0;
  if (t == Tile::DoorKey) {
    for (const Door& d : g.doors) {
      if (d.x == x && d.y == y) {
        if (d.open >= 0.995f) return;
        yBot = d.open;
        vTop = d.open;  // texture rides up with the door slab
      }
    }
  }
  const float x0 = (float)x, x1 = x + 1.0f, z0 = (float)y, z1 = y + 1.0f;
  struct Side { int dx, dy; Vector2 a, b; };
  const Side sides[4] = {
    { 0, -1, { x0, z0 }, { x1, z0 } },  // north face
    { 0, 1, { x1, z1 }, { x0, z1 } },   // south face
    { -1, 0, { x0, z1 }, { x0, z0 } },  // west face
    { 1, 0, { x1, z0 }, { x1, z1 } },   // east face
  };
  for (const Side& s : sides) {
    if (!Walkable(g.map, x + s.dx, y + s.dy)) continue;
    Zone z = g.map.zoneAt(x + s.dx, y + s.dy);  // lit by the room it faces
    Color ca = Shade(g, z, DistTo(cam, s.a.x, s.a.y));
    Color cb = Shade(g, z, DistTo(cam, s.b.x, s.b.y));
    EmitFace(buckets[(int)t], s.a, s.b, yBot, 1.0f, vTop, 1.0f, ca, cb);
  }
}

static void EmitFlatTile(const Game& g, std::vector<QuadV>* buckets,
                         Vector2 cam, int x, int y) {
  Zone zn = g.map.zoneAt(x, y);
  const float x0 = (float)x, x1 = x + 1.0f, z0 = (float)y, z1 = y + 1.0f;
  Color c00 = Shade(g, zn, DistTo(cam, x0, z0));
  Color c10 = Shade(g, zn, DistTo(cam, x1, z0));
  Color c11 = Shade(g, zn, DistTo(cam, x1, z1));
  Color c01 = Shade(g, zn, DistTo(cam, x0, z1));
  QuadV f;
  f.p[0] = { x0, 0, z0 }; f.uv[0] = { 0, 0 }; f.c[0] = c00;
  f.p[1] = { x0, 0, z1 }; f.uv[1] = { 0, 1 }; f.c[1] = c01;
  f.p[2] = { x1, 0, z1 }; f.uv[2] = { 1, 1 }; f.c[2] = c11;
  f.p[3] = { x1, 0, z0 }; f.uv[3] = { 1, 0 }; f.c[3] = c10;
  buckets[kFloorBucket].push_back(f);
  for (int i = 0; i < 4; i++) f.p[i].y = 1.0f;
  buckets[kCeilBucket].push_back(f);
}

static void DrawBuckets(const Assets& A, std::vector<QuadV>* buckets) {
  for (int i = 0; i < kBuckets; i++) {
    if (buckets[i].empty()) continue;
    rlCheckRenderBatchLimit((int)buckets[i].size() * 4);
    rlSetTexture(BucketTexture(A, i).id);
    rlBegin(RL_QUADS);
    for (const QuadV& q : buckets[i]) {
      for (int v = 0; v < 4; v++) {
        rlColor4ub(q.c[v].r, q.c[v].g, q.c[v].b, 255);
        rlTexCoord2f(q.uv[v].x, q.uv[v].y);
        rlVertex3f(q.p[v].x, q.p[v].y, q.p[v].z);
      }
    }
    rlEnd();
    rlSetTexture(0);
  }
}

// ---------------------------------------------------------------------------
// Billboards, sorted far-to-near so alpha edges layer correctly.
// ---------------------------------------------------------------------------
struct Bill {
  float dist;
  Texture2D tex;
  Rectangle src;
  Vector3 pos;
  Vector2 size;
  Color tint;
};

static int EnemyFrame(const Enemy& e) {
  switch (e.state) {
    case EnemyState::Idle: return 0;
    case EnemyState::Active:
      if (e.attackAnim > 0) return 2;
      return e.moved ? ((int)(e.walkAnim * 5.0f) % 2) : 0;
    case EnemyState::Dying: {
      int f = 3 + (int)(e.dieAnim / 0.18f);
      return f > 5 ? 5 : f;
    }
    case EnemyState::Dead: return 5;
  }
  return 0;
}

static Color SpriteTint(const Game& g, Vector2 cam, Vector2 pos) {
  return Shade(g, g.map.zoneAt((int)floorf(pos.x), (int)floorf(pos.y)),
               DistTo(cam, pos.x, pos.y));
}

static void CollectBillboards(const Game& g, const Assets& A, Vector2 cam,
                              std::vector<Bill>& out) {
  for (const Enemy& e : g.enemies) {
    Color tint = SpriteTint(g, cam, e.pos);
    if (e.hurtFlash > 0)
      tint = Color{ 255, (unsigned char)(tint.g / 3), (unsigned char)(tint.b / 3), 255 };
    out.push_back({ DistTo(cam, e.pos.x, e.pos.y), A.enemy[(int)e.type],
                    Rectangle{ (float)EnemyFrame(e) * 64, 0, 64, 64 },
                    Vector3{ e.pos.x, 0.41f, e.pos.y }, Vector2{ 0.82f, 0.82f },
                    tint });
  }
  for (const Pickup& p : g.pickups) {
    if (p.taken) continue;
    float y = 0.18f, s = 0.34f;
    if (p.type == PickupType::Keycard) {  // hover so the objective reads
      y = 0.26f + 0.06f * sinf(g.time * 3.0f);
      s = 0.38f;
    }
    out.push_back({ DistTo(cam, p.pos.x, p.pos.y), A.pickups[(int)p.type],
                    Rectangle{ 0, 0, 32, 32 }, Vector3{ p.pos.x, y, p.pos.y },
                    Vector2{ s, s }, SpriteTint(g, cam, p.pos) });
  }
  for (const SoupCan& c : g.cans) {
    float ft = c.t / c.flightT;
    if (ft > 1) ft = 1;
    float y = 0.55f + c.arcH * sinf(PI * ft);
    out.push_back({ DistTo(cam, c.pos.x, c.pos.y), A.can,
                    Rectangle{ 0, 0, 16, 16 }, Vector3{ c.pos.x, y, c.pos.y },
                    Vector2{ 0.22f, 0.22f }, SpriteTint(g, cam, c.pos) });
  }
}

// ---------------------------------------------------------------------------
void DrawWorld(const Game& g, const Assets& A) {
  const Player& p = g.pl;
  Camera3D cam{};
  cam.position = { p.pos.x, cfg::EyeHeight, p.pos.y };
  cam.target = { p.pos.x + cosf(p.angle), cfg::EyeHeight, p.pos.y + sinf(p.angle) };
  cam.up = { 0, 1, 0 };
  cam.fovy = 60.0f;  // ~90 degrees horizontal at 16:9, Doom's FOV
  cam.projection = CAMERA_PERSPECTIVE;

  BeginMode3D(cam);
  rlDisableBackfaceCulling();

  static std::vector<QuadV> buckets[kBuckets];
  for (auto& b : buckets) b.clear();

  Vector2 camXZ = p.pos;
  const int range = (int)cfg::FogDist + 2;
  int x0 = (int)p.pos.x - range, x1 = (int)p.pos.x + range;
  int y0 = (int)p.pos.y - range, y1 = (int)p.pos.y + range;
  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 > g.map.w - 1) x1 = g.map.w - 1;
  if (y1 > g.map.h - 1) y1 = g.map.h - 1;

  for (int y = y0; y <= y1; y++) {
    for (int x = x0; x <= x1; x++) {
      if (DistTo(camXZ, x + 0.5f, y + 0.5f) > cfg::FogDist + 1.5f) continue;
      Tile t = g.map.at(x, y);
      if (t == Tile::Empty) {
        EmitFlatTile(g, buckets, camXZ, x, y);
      } else {
        EmitWallTile(g, buckets, camXZ, x, y, t);
        if (t == Tile::DoorKey || t == Tile::DoorExit)
          EmitFlatTile(g, buckets, camXZ, x, y);  // doorway floor + ceiling
      }
    }
  }
  DrawBuckets(A, buckets);
  // rlgl rasterises lazily; flush while backface culling is still off, or the
  // ceiling (wound for a from-above view) is culled away at EndMode3D.
  rlDrawRenderBatchActive();

  static std::vector<Bill> bills;
  bills.clear();
  CollectBillboards(g, A, camXZ, bills);
  std::sort(bills.begin(), bills.end(),
            [](const Bill& a, const Bill& b) { return a.dist > b.dist; });
  for (const Bill& b : bills)
    DrawBillboardRec(cam, b.tex, b.src, b.pos, b.size, b.tint);

  for (const Tracer& t : g.tracers)
    DrawLine3D(t.a, t.b, Fade(t.color, Clamp(t.t / 0.08f, 0.0f, 1.0f)));

  rlEnableBackfaceCulling();
  EndMode3D();
}
