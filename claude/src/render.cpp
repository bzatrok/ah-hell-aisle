#include "render.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

#include "assets.h"
#include "config.h"
#include "raymath.h"
#include "world.h"

// ---------------------------------------------------------------------------
// The look of the place.
//
// Walls, floor and ceiling are baked once into a handful of static meshes — one per
// wall texture, one for all the floor, one for all the ceiling — so the whole store
// is about eight draw calls. Light is baked into the vertex colours: each surface
// takes the ambient colour of the zone it faces, so the freezer is cold and dim and
// the magazijn is sodium-yellow, without a single dynamic light.
//
// Two things are not baked, and they are the two the shader exists for: distance fog
// (you cannot see the end of the aisle) and the freezer's failing strip lights. A
// surface with vertex alpha below 0.75 is tagged as freezer, and dims with the
// flicker uniform.
// ---------------------------------------------------------------------------

namespace {

constexpr float kFogDensity = 0.0105f;   // exp(-d^2 * k): the aisle fades out around ten tiles

const char* kWorldVS = R"(#version 330
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 vertexColor;
uniform mat4 mvp;
uniform mat4 matModel;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragWorld;
void main()
{
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    fragWorld = vec3(matModel*vec4(vertexPosition, 1.0));
    gl_Position = mvp*vec4(vertexPosition, 1.0);
}
)";

const char* kWorldFS = R"(#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragWorld;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec3 viewPos;
uniform float flicker;
uniform float flash;
out vec4 finalColor;
void main()
{
    vec4 texel = texture(texture0, fragTexCoord)*colDiffuse;

    // Vertex colour is the baked zone light. Alpha under 0.75 tags a freezer surface,
    // which rides the flicker.
    vec3 light = fragColor.rgb*mix(1.0, flicker, step(fragColor.a, 0.75));

    float dist = length(fragWorld - viewPos);
    light *= exp(-dist*dist*0.0105);
    light += vec3(flash*exp(-dist*0.55));

    finalColor = vec4(texel.rgb*light, 1.0);
}
)";

// Sprites are cutouts: throw away the transparent pixels so every billboard writes
// real depth and occludes the ones behind it, whatever order they arrive in.
const char* kSpriteFS = R"(#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
out vec4 finalColor;
void main()
{
    vec4 texel = texture(texture0, fragTexCoord);
    if (texel.a < 0.5) discard;
    finalColor = vec4(texel.rgb*fragColor.rgb*colDiffuse.rgb, 1.0);
}
)";

// --- baked lighting ---------------------------------------------------------

Vector3 ZoneAmbient(Zone z) {
    switch (z) {
        case Zone::Checkout: return {0.62f, 0.65f, 0.68f};   // the lights they leave on
        case Zone::Freezer:  return {0.21f, 0.31f, 0.45f};   // cold, and half of it broken
        case Zone::Magazijn: return {0.38f, 0.32f, 0.24f};   // one sodium lamp, no windows
        case Zone::Store:
        default:             return {0.50f, 0.51f, 0.53f};
    }
}

// Deterministic per-tile grime, so a few of the strip lights are simply out.
float TileNoise(int x, int y) {
    unsigned h = (unsigned)x * 73856093u ^ (unsigned)y * 19349663u;
    h ^= h >> 13;
    h *= 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xFFFFu) / 65535.0f;
}

Color Bake(Zone zone, float shade) {
    Vector3 c = ZoneAmbient(zone);
    const unsigned char alpha = (zone == Zone::Freezer) ? 128 : 255;   // the flicker tag
    return Color{
        (unsigned char)(Clamp(c.x * shade, 0.0f, 1.0f) * 255.0f),
        (unsigned char)(Clamp(c.y * shade, 0.0f, 1.0f) * 255.0f),
        (unsigned char)(Clamp(c.z * shade, 0.0f, 1.0f) * 255.0f),
        alpha,
    };
}

// --- mesh building ----------------------------------------------------------

struct MeshBuilder {
    std::vector<float> verts;
    std::vector<float> uvs;
    std::vector<unsigned char> cols;

    // Corners wind counter-clockwise as seen from the visible side.
    void Quad(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color col) {
        const Vector3 tri[6] = {a, b, c, a, c, d};
        const Vector2 uv[6] = {{0, 1}, {1, 1}, {1, 0}, {0, 1}, {1, 0}, {0, 0}};
        for (int i = 0; i < 6; i++) {
            verts.insert(verts.end(), {tri[i].x, tri[i].y, tri[i].z});
            uvs.insert(uvs.end(), {uv[i].x, uv[i].y});
            cols.insert(cols.end(), {col.r, col.g, col.b, col.a});
        }
    }

    bool empty() const { return verts.empty(); }

    Mesh Upload() const {
        Mesh m{};
        m.vertexCount = (int)(verts.size() / 3);
        m.triangleCount = m.vertexCount / 3;
        m.vertices = (float*)MemAlloc((unsigned)verts.size() * sizeof(float));
        m.texcoords = (float*)MemAlloc((unsigned)uvs.size() * sizeof(float));
        m.colors = (unsigned char*)MemAlloc((unsigned)cols.size());
        memcpy(m.vertices, verts.data(), verts.size() * sizeof(float));
        memcpy(m.texcoords, uvs.data(), uvs.size() * sizeof(float));
        memcpy(m.colors, cols.data(), cols.size());
        UploadMesh(&m, false);
        return m;
    }
};

// Doorways count as open space for geometry: the door itself is a separate box that
// slides, so the tiles around it need their faces, floor and ceiling like any other
// gap — otherwise lifting the magazijn door reveals a hole in the world.
bool OpenSpace(Tile t) {
    return t == Tile::Empty || t == Tile::DoorKeycard || t == Tile::DoorExit;
}

// A tile's four side faces, each emitted only where it meets open floor, and each lit
// by the zone on the side you would see it from.
void AddWallFaces(MeshBuilder& mb, const Map& map, int x, int y) {
    const float x0 = (float)x, x1 = x0 + 1.0f;
    const float z0 = (float)y, z1 = z0 + 1.0f;
    const float h = kWallH;

    struct Side { int dx, dy; float shade; };
    const Side sides[4] = {{0, -1, 0.88f}, {0, 1, 0.88f}, {-1, 0, 1.06f}, {1, 0, 1.06f}};

    for (const Side& s : sides) {
        const int nx = x + s.dx, ny = y + s.dy;
        if (!Map::InBounds(nx, ny)) continue;
        if (!OpenSpace(map.At(nx, ny))) continue;   // hidden face, never emit it

        const Color col = Bake(map.ZoneAt(nx, ny), s.shade);
        if (s.dy < 0)      mb.Quad({x1, 0, z0}, {x0, 0, z0}, {x0, h, z0}, {x1, h, z0}, col);
        else if (s.dy > 0) mb.Quad({x0, 0, z1}, {x1, 0, z1}, {x1, h, z1}, {x0, h, z1}, col);
        else if (s.dx < 0) mb.Quad({x0, 0, z0}, {x0, 0, z1}, {x0, h, z1}, {x0, h, z0}, col);
        else               mb.Quad({x1, 0, z1}, {x1, 0, z0}, {x1, h, z0}, {x1, h, z1}, col);
    }
}

// --- renderer state ---------------------------------------------------------

struct DoorMesh {
    Mesh mesh{};
    Material material{};
    int x = 0, y = 0;
};

struct Renderer {
    Shader world{};
    Shader sprite{};
    int locViewPos = -1;
    int locFlicker = -1;
    int locFlash = -1;

    Mesh walls[kTileKindCount]{};
    bool hasWall[kTileKindCount]{};
    Material wallMat[kTileKindCount]{};

    Mesh floorMesh{};
    Mesh ceilMesh{};
    Material floorMat{};
    Material ceilMat{};

    std::vector<DoorMesh> doors;

    float flicker = 1.0f;
    float flickerNext = 0.0f;
    float time = 0.0f;
};

Renderer g;

Material MakeMaterial(Texture2D tex, Shader shader) {
    Material m = LoadMaterialDefault();
    m.shader = shader;
    m.maps[MATERIAL_MAP_DIFFUSE].texture = tex;
    m.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
    return m;
}

// The two doors are the only geometry that moves: they slide up into the ceiling.
void BuildDoor(const Map& map, const Door& d) {
    MeshBuilder mb;
    const float x0 = (float)d.x, x1 = x0 + 1.0f;
    const float z0 = (float)d.y, z1 = z0 + 1.0f;
    const float h = kWallH;
    const Color side = Bake(map.ZoneAt(d.x, d.y), 1.0f);
    const Color under = Bake(map.ZoneAt(d.x, d.y), 0.55f);

    mb.Quad({x1, 0, z0}, {x0, 0, z0}, {x0, h, z0}, {x1, h, z0}, side);
    mb.Quad({x0, 0, z1}, {x1, 0, z1}, {x1, h, z1}, {x0, h, z1}, side);
    mb.Quad({x0, 0, z0}, {x0, 0, z1}, {x0, h, z1}, {x0, h, z0}, side);
    mb.Quad({x1, 0, z1}, {x1, 0, z0}, {x1, h, z0}, {x1, h, z1}, side);
    mb.Quad({x0, 0, z0}, {x1, 0, z0}, {x1, 0, z1}, {x0, 0, z1}, under);   // seen once it lifts

    DoorMesh dm;
    dm.mesh = mb.Upload();
    dm.material = MakeMaterial(
        d.isExit ? gAssets.wall[(int)Tile::DoorExit] : gAssets.wall[(int)Tile::DoorKeycard],
        g.world);
    dm.x = d.x;
    dm.y = d.y;
    g.doors.push_back(dm);
}

// --- sprites ----------------------------------------------------------------

struct Billboard {
    Texture2D tex;
    Rectangle src;
    Vector3 pos;
    Vector2 size;
    Color tint;
    float distSq;
};

// The same light the shader applies to the walls, done on the CPU for a billboard so
// sprites sit in the room instead of on top of it.
Color LightFor(const World& w, Vector3 at, Vector3 eye) {
    const Zone zone = w.map.ZoneAt((int)at.x, (int)at.z);
    Vector3 c = ZoneAmbient(zone);
    if (zone == Zone::Freezer) c = Vector3Scale(c, g.flicker);

    const float dist = Vector3Distance(at, eye);
    const float fog = expf(-dist * dist * kFogDensity);
    const float flash = w.player.muzzleFlash * expf(-dist * 0.55f);

    return Color{
        (unsigned char)(Clamp(c.x * fog + flash, 0.0f, 1.0f) * 255.0f),
        (unsigned char)(Clamp(c.y * fog + flash, 0.0f, 1.0f) * 255.0f),
        (unsigned char)(Clamp(c.z * fog + flash, 0.0f, 1.0f) * 255.0f),
        255,
    };
}

Color FlashWhite(Color base, float amount) {
    if (amount <= 0.0f) return base;
    const float t = Clamp(amount, 0.0f, 1.0f);
    return Color{
        (unsigned char)Lerp(base.r, 255.0f, t),
        (unsigned char)Lerp(base.g, 255.0f, t),
        (unsigned char)Lerp(base.b, 255.0f, t),
        255,
    };
}

// NOTE: DrawBillboardPro with a zero origin anchors the quad's BOTTOM edge at
// `position` — it does not centre on it. That suits us: the art is drawn with its feet
// on the bottom row of every frame, corpses included, so the floor position is the
// position, and a corpse lies on the floor for free.
constexpr float kCanSize = 0.26f;

void CollectSprites(const World& w, Vector3 eye, std::vector<Billboard>& out) {
    for (const Enemy& e : w.enemies) {
        const float size = StatsFor(e.kind).spriteSize;
        const Vector3 at = {e.pos.x, 0.0f, e.pos.y};
        const Color tint = FlashWhite(LightFor(w, at, eye), e.hurtFlash);
        out.push_back({gAssets.enemy[(int)e.kind],
                       {e.frame() * 64.0f, 0.0f, 64.0f, 64.0f},
                       at,
                       {size, size},
                       tint,
                       Vector3DistanceSqr(at, eye)});
    }

    for (const Pickup& p : w.pickups) {
        if (p.taken) continue;
        const float size = 0.42f;
        const Vector3 at = {p.pos.x, 0.05f + sinf(p.phase) * 0.04f, p.pos.y};   // it bobs
        out.push_back({gAssets.pickup[(int)p.kind],
                       {0.0f, 0.0f, 32.0f, 32.0f},
                       at,
                       {size, size},
                       LightFor(w, at, eye),
                       Vector3DistanceSqr(at, eye)});
    }

    for (const Projectile& p : w.projectiles) {
        // Centre the can on its flight path rather than hanging it below.
        const Vector3 at = {p.pos.x, p.height - kCanSize * 0.5f, p.pos.y};
        out.push_back({gAssets.soupCan,
                       {0.0f, 0.0f, 16.0f, 16.0f},
                       at,
                       {kCanSize, kCanSize},
                       LightFor(w, at, eye),
                       Vector3DistanceSqr(at, eye)});
    }
}

// The zelfscanner's beam: a thin line while it is choosing you, a fat one while it is
// shooting you. Both are stopped by shelves because the depth buffer says so.
void DrawBeams(const World& w, Vector3 eye) {
    for (const Enemy& e : w.enemies) {
        if (!e.alive()) continue;
        const Vector3 muzzle = {e.pos.x, 0.95f, e.pos.y};

        if (e.beam > 0.0f) {
            for (int i = 0; i < 3; i++) {
                const float o = (i - 1) * 0.012f;
                DrawLine3D({muzzle.x, muzzle.y + o, muzzle.z},
                           {eye.x, eye.y + o - 0.05f, eye.z},
                           Color{255, 90, 60, 255});
            }
        } else if (e.aimBeam > 0.0f) {
            DrawLine3D(muzzle, {eye.x, eye.y - 0.05f, eye.z}, Color{220, 40, 30, 110});
        }
    }
}

}  // namespace

// --- lifecycle --------------------------------------------------------------

void RenderInit(const Map& map) {
    g.world = LoadShaderFromMemory(kWorldVS, kWorldFS);
    g.sprite = LoadShaderFromMemory(nullptr, kSpriteFS);   // raylib's default vertex stage
    g.locViewPos = GetShaderLocation(g.world, "viewPos");
    g.locFlicker = GetShaderLocation(g.world, "flicker");
    g.locFlash = GetShaderLocation(g.world, "flash");

    // One mesh per wall texture: every solid tile of that kind contributes its exposed
    // faces, and the whole store draws in a handful of calls.
    MeshBuilder walls[kTileKindCount];
    MeshBuilder floorB;
    MeshBuilder ceilB;

    for (int y = 0; y < Map::H; y++) {
        for (int x = 0; x < Map::W; x++) {
            const Tile t = map.At(x, y);

            if (!OpenSpace(t)) {
                AddWallFaces(walls[(int)t], map, x, y);
                continue;
            }

            const Zone zone = map.ZoneAt(x, y);
            const float grime = TileNoise(x, y);
            const float dead = (grime < 0.14f) ? 0.45f : 0.92f + grime * 0.16f;

            const float x0 = (float)x, x1 = x0 + 1.0f;
            const float z0 = (float)y, z1 = z0 + 1.0f;
            floorB.Quad({x0, 0, z0}, {x0, 0, z1}, {x1, 0, z1}, {x1, 0, z0},
                        Bake(zone, 0.62f * dead));
            ceilB.Quad({x0, kWallH, z0}, {x1, kWallH, z0}, {x1, kWallH, z1}, {x0, kWallH, z1},
                       Bake(zone, 0.86f * dead));
        }
    }

    for (int i = 0; i < kTileKindCount; i++) {
        if (walls[i].empty()) continue;
        g.walls[i] = walls[i].Upload();
        g.wallMat[i] = MakeMaterial(gAssets.wall[i], g.world);
        g.hasWall[i] = true;
    }

    g.floorMesh = floorB.Upload();
    g.ceilMesh = ceilB.Upload();
    g.floorMat = MakeMaterial(gAssets.floorTex, g.world);
    g.ceilMat = MakeMaterial(gAssets.ceilingTex, g.world);

    for (const Door& d : map.doors) BuildDoor(map, d);
}

void RenderShutdown() {
    for (int i = 0; i < kTileKindCount; i++) {
        if (g.hasWall[i]) UnloadMesh(g.walls[i]);
    }
    UnloadMesh(g.floorMesh);
    UnloadMesh(g.ceilMesh);
    for (DoorMesh& d : g.doors) UnloadMesh(d.mesh);
    g.doors.clear();

    UnloadShader(g.world);
    UnloadShader(g.sprite);
}

// --- frame ------------------------------------------------------------------

Camera3D SceneCamera(const World& w) {
    const Player& p = w.player;

    float bobY = sinf(p.bobPhase * 2.0f) * 0.018f * p.bobAmount;
    float shakeX = 0.0f, shakeY = 0.0f;
    if (w.shake > 0.0f) {
        shakeX = sinf(g.time * 61.0f) * w.shake * 0.035f;
        shakeY = sinf(g.time * 47.0f) * w.shake * 0.035f;
    }

    Camera3D cam{};
    cam.position = {p.pos.x + shakeX, p.eyeHeight() + bobY + shakeY, p.pos.y};
    cam.target = Vector3Add(cam.position, {p.forward().x, 0.0f, p.forward().y});
    cam.up = {0.0f, 1.0f, 0.0f};
    cam.fovy = kFovY;
    cam.projection = CAMERA_PERSPECTIVE;
    return cam;
}

void RenderScene(const World& w, float dt) {
    g.time += dt;

    // The freezer's strip lights are on their way out.
    if (g.time >= g.flickerNext) {
        const bool dropout = GetRandomValue(0, 100) < 16;
        g.flicker = dropout ? 0.30f + (float)GetRandomValue(0, 25) / 100.0f : 1.0f;
        g.flickerNext = g.time + (dropout ? 0.03f : 0.10f) +
                        (float)GetRandomValue(0, 30) / 100.0f;
    }

    const Camera3D cam = SceneCamera(w);
    const float flash = w.player.muzzleFlash * 0.9f;

    SetShaderValue(g.world, g.locViewPos, &cam.position, SHADER_UNIFORM_VEC3);
    SetShaderValue(g.world, g.locFlicker, &g.flicker, SHADER_UNIFORM_FLOAT);
    SetShaderValue(g.world, g.locFlash, &flash, SHADER_UNIFORM_FLOAT);

    ClearBackground(BLACK);
    BeginMode3D(cam);

    const Matrix identity = MatrixIdentity();
    for (int i = 0; i < kTileKindCount; i++) {
        if (g.hasWall[i]) DrawMesh(g.walls[i], g.wallMat[i], identity);
    }
    DrawMesh(g.floorMesh, g.floorMat, identity);
    DrawMesh(g.ceilMesh, g.ceilMat, identity);

    for (const DoorMesh& dm : g.doors) {
        const Door* d = w.map.DoorAt(dm.x, dm.y);
        const float lift = d ? d->slide * kWallH : 0.0f;
        DrawMesh(dm.mesh, dm.material, MatrixTranslate(0.0f, lift, 0.0f));
    }

    DrawBeams(w, cam.position);

    // Back to front. The cutout shader means the depth buffer would sort them anyway,
    // but drawing far-to-near keeps the soft edges of the art honest.
    std::vector<Billboard> sprites;
    sprites.reserve(w.enemies.size() + w.pickups.size() + w.projectiles.size());
    CollectSprites(w, cam.position, sprites);
    std::sort(sprites.begin(), sprites.end(),
              [](const Billboard& a, const Billboard& b) { return a.distSq > b.distSq; });

    BeginShaderMode(g.sprite);
    for (const Billboard& b : sprites) {
        DrawBillboardPro(cam, b.tex, b.src, b.pos, {0.0f, 1.0f, 0.0f}, b.size, {0.0f, 0.0f},
                         0.0f, b.tint);
    }
    EndShaderMode();

    EndMode3D();
}

void RenderWeapon(const World& w) {
    const Player& p = w.player;
    if (p.dead()) return;

    const Texture2D tex = gAssets.weapon[(int)p.weapon];
    const float scale = 2.15f;
    const float frameW = 192.0f, frameH = 144.0f;
    const Rectangle src = {p.weaponFrame() * frameW, 0.0f, frameW, frameH};

    const float bobX = sinf(p.bobPhase) * 13.0f * p.bobAmount;
    const float bobY = fabsf(sinf(p.bobPhase * 2.0f)) * 10.0f * p.bobAmount;
    const float kick = (p.fireAnim > 0.0f) ? 12.0f : 0.0f;

    const float dw = frameW * scale, dh = frameH * scale;
    const Rectangle dst = {
        kScreenW * 0.5f - dw * 0.5f + 26.0f + bobX,          // bottom-centre-right, as drawn
        kScreenH - kHudH - dh + bobY + kick,
        dw, dh,
    };

    // Your hands are lit by the room you are standing in, and by your own muzzle.
    const Vector3 here = {p.pos.x, kEyeH, p.pos.y};
    const Color tint = FlashWhite(LightFor(w, here, here), p.muzzleFlash * 0.8f);

    DrawTexturePro(tex, src, dst, {0.0f, 0.0f}, 0.0f, tint);
}
