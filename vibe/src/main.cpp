// AH: Hell Aisle - Minimal Doom Clone
// C++17 + raylib implementation
// This is a minimal working version demonstrating the core mechanics

#include <raylib.h>
#include <raymath.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <ctime>
#include <cstdlib>

using namespace std;

const int SW = 1024, SH = 768;
const float TS = 1.0f, WH = 4.0f, PH = 1.0f, PR = 0.3f;
const char* AP = "../assets/";

enum TT { T_E, T_SF, T_SE, T_F, T_P, T_C, T_M, T_DK, T_DE, T_FL, T_CE, T_CNT };
const char* WT[T_CNT] = {"", "wall_shelf_full.png", "wall_shelf_empty.png", "wall_freezer.png", 
                         "wall_plain.png", "wall_checkout.png", "wall_magazijn.png", "door_keycard.png", "door_exit.png", 
                         "floor.png", "ceiling.png"};

enum ST { S_N, S_P, S_WW, S_VK, S_ZS, S_SP, S_AF, S_RW, S_LB, S_BK, S_KC, S_WS, S_WP, S_CNT };
const char* SF[S_CNT] = {"", "", "enemy_winkelwagen.png", "enemy_vakkenvuller.png", 
                          "enemy_zelfscanner.png", "proj_soepblik.png", "pickup_appelflap.png",
                          "pickup_rookworst.png", "pickup_labels.png", "pickup_bonuskaart.png",
                          "pickup_keycard.png", "weapon_stokbrood.png", "weapon_prijspistool.png"};

enum GS { G_T, G_P, G_D, G_V };
enum WType { W_S, W_P, W_CNT };
enum EType { E_W, E_K, E_S, E_CNT };
enum PType { P_AF, P_RW, P_LB, P_BK, P_KC, P_CNT };

const float EH[E_CNT] = {30, 60, 100};
const float ES[E_CNT] = {4, 1.8f, 0.4f};
const int ED[E_CNT] = {10, 15, 8};
const float ER[E_CNT] = {1.5f, 10, 14};
const float EC[E_CNT] = {1, 2, 1.5f};
const int WD[W_CNT] = {25, 20};
const float WR[W_CNT] = {1.5f, 1000};
const float WRT[W_CNT] = {0.5f, 0.25f};

struct Map {
    int w, h;
    vector<vector<int>> t;
    vector<vector<bool>> s;
    Map(int ww, int hh) : w(ww), h(hh), t(hh, vector<int>(ww, 0)), s(hh, vector<bool>(ww, false)) {}
    bool isS(int x, int y) const { return (x < 0 || x >= w || y < 0 || y >= h) ? true : s[y][x]; }
};

struct Entity {
    Vector2 pos;
    float r;
    bool active;
    Entity(Vector2 p = {0, 0}, float rr = 0.3f) : pos(p), r(rr), active(true) {}
    virtual ~Entity() {}
    bool coll(const Entity& o) const { return Vector2Distance(pos, o.pos) < (r + o.r); }
};

struct Player;
struct Door;

struct Projectile : Entity {
    Vector2 vel;
    int dmg;
    bool hitscan;
    ST sprite;
    Entity* owner;
    float lifetime, maxLifetime;
    Projectile(Vector2 p, Vector2 v, ST s, int d, bool h, Entity* o)
        : Entity(p, 0.15f), vel(v), dmg(d), hitscan(h), sprite(s), owner(o),
          lifetime(0), maxLifetime(h ? 0.05f : 10.0f) {}
    void update(float dt, Player& pl, vector<Entity*>& ents, Map& map, const vector<Door>& doors);
};

struct Player : Entity {
    float angle;
    int health, armor, ammo[2];
    bool hasKeycard, isDead;
    WType currentWeapon, pendingWeapon;
    float weaponCooldown, weaponAnimTime, bobOffset, bobTimer;
    float moveSpeed = 3.5f, turnSpeed = 2.0f;
    
    Player() : Entity({2, 2}, PR), angle(0), health(100), armor(0), hasKeycard(false), isDead(false),
        currentWeapon(W_S), pendingWeapon(W_S), weaponCooldown(0), weaponAnimTime(0),
        bobOffset(0), bobTimer(0) { ammo[0] = 0; ammo[1] = 40; }
    
    void takeDamage(int d) {
        if (isDead) return;
        if (armor > 0) { armor = max(0, armor - d / 2); d -= d / 2; }
        health = max(0, health - d);
        if (health <= 0) { health = 0; isDead = true; }
    }
    void heal(int a) { if (!isDead) health = min(100, health + a); }
    void addArmor(int a) { if (!isDead) armor = min(100, armor + a); }
    void addAmmo(int a) { ammo[1] = min(200, ammo[1] + a); }
    bool canFire() const { return !isDead && weaponCooldown <= 0 && (currentWeapon != W_P || ammo[1] > 0); }
    void startFire() {
        if (!canFire()) return;
        weaponCooldown = WRT[currentWeapon];
        weaponAnimTime = 0;
        if (currentWeapon == W_P) ammo[1]--;
    }
    void switchWeapon(WType w) { if (w < W_CNT) pendingWeapon = w; }
    void update(float dt, Map& m, const vector<Door>& doors);
};

struct Enemy : Entity {
    EType type;
    float health, attackCooldown, deathTimer;
    bool hasLOS, isDead;
    Vector2 targetPos;
    float animTime, animFrame;
    float noticeRange = 12.0f, attackRange;
    
    Enemy(EType t, Vector2 p) : Entity(p, 0.3f), type(t), health(EH[t]),
        attackCooldown(0), deathTimer(0), hasLOS(false), isDead(false),
        targetPos(p), animTime(0), animFrame(0), attackRange(ER[t]) {}
    
    bool canSeePlayer(const Player& pl, const Map& m, const vector<Door>& doors);
    void update(float dt, Player& pl, Map& m, const vector<Door>& doors, vector<Entity*>& projectiles);
    void takeDamage(int d);
    int getSpriteFrame() const;
};

struct Pickup : Entity {
    PType type;
    bool collected;
    float bobTimer;
    Pickup(PType t, Vector2 p) : Entity(p, 0.25f), type(t), collected(false), bobTimer(0) {}
    void update(float dt, Player& pl);
};

struct Door {
    Vector2 pos;
    int tileX, tileY;
    TT type;
    bool locked, opened;
    Door(int x, int y, TT t, bool l = true) :
        pos({static_cast<float>(x * TS), static_cast<float>(y * TS)}),
        tileX(x), tileY(y), type(t), locked(l), opened(false) {}
    bool tryOpen(Player& pl);
    bool isBlocking() const { return !opened; }
};

struct Renderer {
    Camera3D camera;
    Texture2D wallTextures[T_CNT], spriteTextures[S_CNT], hudPanel, hudFace;
    Texture2D weaponTextures[2];
    Model wallModel, floorModel;
    Material wallMaterials[T_CNT];
    
    Renderer() {
        camera.position = {0, PH, 0};
        camera.target = {0, PH, 1};
        camera.up = {0, 1, 0};
        camera.fovy = 60.0f;
        camera.projection = CAMERA_PERSPECTIVE;
    }
    
    void loadTextures();
    void unloadModels();
    void updateCamera(const Player& pl);
    void drawWorld(const Map& m, const vector<Door>& doors);
    void drawSprites(const vector<Entity*>& ents, const Player& pl);
    void drawWeapon(const Player& pl);
    void drawHUD(const Player& pl, int kills, int total, float time);
    void drawTitle();
    void drawDeath(int kills, int total, float time);
    void drawVictory(int kills, int total, float time);
};

Map createLevel();

// Projectile implementation
void Projectile::update(float dt, Player& pl, vector<Entity*>& ents, Map& map, const vector<Door>& doors) {
    lifetime += dt;
    if (lifetime >= maxLifetime) { active = false; return; }
    if (hitscan) { active = false; return; }
    Vector2 oldPos = pos;
    pos = Vector2Add(pos, Vector2Scale(vel, dt));
    
    // Check if projectile hit a wall
    int tx = static_cast<int>(pos.x / TS), ty = static_cast<int>(pos.y / TS);
    if (tx >= 0 && tx < map.w && ty >= 0 && ty < map.h) {
        if (map.isS(tx, ty)) { active = false; return; }
        for (const Door& d : doors) {
            if (d.tileX == tx && d.tileY == ty && d.isBlocking()) {
                active = false; return;
            }
        }
    }
    
    if (coll(pl) && owner != (Entity*)&pl) { pl.takeDamage(dmg); active = false; return; }
    for (Entity* ee : ents) {
        if (ee != owner && ee != this) {
            if (Enemy* en = dynamic_cast<Enemy*>(ee)) {
                if (coll(*en)) { en->takeDamage(dmg); active = false; return; }
            }
        }
    }
}

// Player implementation
void Player::update(float dt, Map& m, const vector<Door>& doors) {
    if (isDead) return;
    if (pendingWeapon != currentWeapon) currentWeapon = pendingWeapon;
    weaponCooldown = max(0.0f, weaponCooldown - dt);
    weaponAnimTime += dt;
    bobTimer += dt * 5.0f;
    bobOffset = sinf(bobTimer) * 0.05f;
    
    Vector2 moveDir = {0, 0};
    if (IsKeyDown(KEY_W)) moveDir = Vector2Add(moveDir, {cosf(angle), sinf(angle)});
    if (IsKeyDown(KEY_S)) moveDir = Vector2Add(moveDir, {-cosf(angle), -sinf(angle)});
    if (IsKeyDown(KEY_A)) moveDir = Vector2Add(moveDir, {sinf(angle), -cosf(angle)});
    if (IsKeyDown(KEY_D)) moveDir = Vector2Add(moveDir, {-sinf(angle), cosf(angle)});
    
    auto isDoorBlockingAt = [&](int tx, int ty) {
        for (const Door& d : doors) {
            if (d.tileX == tx && d.tileY == ty && !d.opened) return true;
        }
        return false;
    };
    
    if (Vector2Length(moveDir) > 0) {
        moveDir = Vector2Scale(Vector2Normalize(moveDir), moveSpeed * dt);
        Vector2 newPos = Vector2Add(pos, moveDir);
        // Clamp newPos to stay within map bounds
        if (newPos.x < 0) newPos.x = 0;
        if (newPos.y < 0) newPos.y = 0;
        if (newPos.x >= m.w * TS) newPos.x = m.w * TS - 0.001f;
        if (newPos.y >= m.h * TS) newPos.y = m.h * TS - 0.001f;
        
        int nx = static_cast<int>(newPos.x / TS), ny = static_cast<int>(newPos.y / TS);
        if (nx >= 0 && nx < m.w && ny >= 0 && ny < m.h) {
            if (!m.s[ny][nx] && !isDoorBlockingAt(nx, ny)) {
                Vector2 tp = pos;
                tp.x = newPos.x;
                int tpx = static_cast<int>(tp.x / TS), tpy = static_cast<int>(tp.y / TS);
                if (tpx >= 0 && tpx < m.w && tpy >= 0 && tpy < m.h) {
                    if (!m.s[tpy][tpx] && !isDoorBlockingAt(tpx, tpy)) pos.x = newPos.x;
                }
                tp = pos;
                tp.y = newPos.y;
                tpx = static_cast<int>(tp.x / TS); tpy = static_cast<int>(tp.y / TS);
                if (tpx >= 0 && tpx < m.w && tpy >= 0 && tpy < m.h) {
                    if (!m.s[tpy][tpx] && !isDoorBlockingAt(tpx, tpy)) pos.y = newPos.y;
                }
            }
        }
    }
    
    float mouseDelta = GetMouseDelta().x;
    if (std::isfinite(mouseDelta)) {
        angle += mouseDelta * 0.002f * turnSpeed * dt * 60.0f;
    }
    if (IsKeyDown(KEY_LEFT)) angle -= turnSpeed * dt;
    if (IsKeyDown(KEY_RIGHT)) angle += turnSpeed * dt;
    if (!std::isfinite(angle)) angle = 0;
    while (angle < -PI) angle += 2 * PI;
    while (angle > PI) angle -= 2 * PI;
}

// Enemy implementation
bool Enemy::canSeePlayer(const Player& pl, const Map& m, const vector<Door>& doors) {
    if (isDead) return false;
    Vector2 toPlayer = Vector2Subtract(pl.pos, pos);
    float dist = Vector2Length(toPlayer);
    if (dist > noticeRange || dist < 0.01f) return false;
    toPlayer = Vector2Normalize(toPlayer);
    Vector2 checkPos = Vector2Add(pos, Vector2Scale(toPlayer, r));
    float checkDist = dist - r - pl.r;
    while (checkDist > 0.1f) {
        int tx = static_cast<int>(checkPos.x / TS), ty = static_cast<int>(checkPos.y / TS);
        if (tx >= 0 && tx < m.w && ty >= 0 && ty < m.h) {
            if (m.isS(tx, ty)) return false;
            for (const Door& d : doors) {
                if (d.tileX == tx && d.tileY == ty && d.isBlocking()) return false;
            }
        }
        checkPos = Vector2Add(checkPos, Vector2Scale(toPlayer, 0.1f));
        checkDist -= 0.1f;
    }
    return true;
}

void Enemy::update(float dt, Player& pl, Map& m, const vector<Door>& doors, vector<Entity*>& projectiles) {
    if (isDead) { deathTimer += dt; return; }
    hasLOS = canSeePlayer(pl, m, doors);
    if (hasLOS) targetPos = pl.pos;
    if (!hasLOS) return;
    
    // Helper to check if a tile is blocked by wall or door
    auto isTileBlocking = [&](int tx, int ty) -> bool {
        if (tx < 0 || tx >= m.w || ty < 0 || ty >= m.h) return true;
        if (m.isS(tx, ty)) return true;
        for (const Door& d : doors) {
            if (d.tileX == tx && d.tileY == ty && d.isBlocking()) return true;
        }
        return false;
    };
    
    Vector2 toTarget = Vector2Subtract(targetPos, pos);
    float dist = Vector2Length(toTarget);
    
    switch (type) {
        case E_W: // Winkelwagen - melee
            if (dist <= attackRange + 0.5f && attackCooldown <= 0) {
                pl.takeDamage(ED[type]);
                attackCooldown = EC[type];
            } else if (dist > 0.01f) {
                toTarget = Vector2Normalize(toTarget);
                Vector2 moveDir = Vector2Scale(toTarget, ES[type] * dt);
                Vector2 newPos = Vector2Add(pos, moveDir);
                int nx = static_cast<int>(newPos.x / TS), ny = static_cast<int>(newPos.y / TS);
                if (!isTileBlocking(nx, ny)) {
                    pos = newPos;
                } else {
                    Vector2 tp = pos;
                    tp.x = newPos.x;
                    int tpx = static_cast<int>(tp.x / TS), tpy = static_cast<int>(tp.y / TS);
                    if (!isTileBlocking(tpx, tpy)) pos.x = newPos.x;
                    tp = pos;
                    tp.y = newPos.y;
                    tpx = static_cast<int>(tp.x / TS); tpy = static_cast<int>(tp.y / TS);
                    if (!isTileBlocking(tpx, tpy)) pos.y = newPos.y;
                }
            }
            break;
            
        case E_K: // Vakkenvuller - ranged
            if (dist <= attackRange && attackCooldown <= 0) {
                if (dist > 0.01f) {
                    Vector2 throwDir = Vector2Normalize(toTarget);
                    throwDir = Vector2Scale(throwDir, 3.0f);
                    projectiles.push_back(new Projectile(
                        Vector2Add(pos, Vector2Scale(throwDir, 0.5f)), throwDir, S_SP, 15, false, this));
                }
                attackCooldown = EC[type];
            } else if (dist < attackRange * 0.7f) {
                if (dist > 0.01f) {
                    toTarget = Vector2Normalize(toTarget);
                    Vector2 moveDir = Vector2Scale(toTarget, -ES[type] * 0.5f * dt);
                    Vector2 newPos = Vector2Add(pos, moveDir);
                    if (!isTileBlocking(static_cast<int>(newPos.x / TS), static_cast<int>(newPos.y / TS)))
                        pos = newPos;
                }
            } else {
                if (dist > 0.01f) {
                    toTarget = Vector2Normalize(toTarget);
                    Vector2 moveDir = Vector2Scale(toTarget, ES[type] * dt);
                    Vector2 newPos = Vector2Add(pos, moveDir);
                    if (!isTileBlocking(static_cast<int>(newPos.x / TS), static_cast<int>(newPos.y / TS)))
                        pos = newPos;
                }
            }
            break;
            
        case E_S: // Zelfscanner - turret
            if (dist <= attackRange && attackCooldown <= 0 && dist > 0.01f) {
                Vector2 shootDir = Vector2Normalize(toTarget);
                bool hitPlayer = true;
                Vector2 checkPos = Vector2Add(pos, Vector2Scale(shootDir, 0.5f));
                float checkDist = dist;
                while (checkDist > 0.2f) {
                    int tx = static_cast<int>(checkPos.x / TS), ty = static_cast<int>(checkPos.y / TS);
                    if (tx >= 0 && tx < m.w && ty >= 0 && ty < m.h) {
                        if (m.isS(tx, ty)) {
                            hitPlayer = false; break;
                        }
                        for (const Door& d : doors) {
                            if (d.tileX == tx && d.tileY == ty && d.isBlocking()) {
                                hitPlayer = false; break;
                            }
                        }
                        if (!hitPlayer) break;
                    }
                    checkPos = Vector2Add(checkPos, Vector2Scale(shootDir, 0.2f));
                    checkDist -= 0.2f;
                }
                if (hitPlayer) pl.takeDamage(ED[type]);
                attackCooldown = EC[type];
            }
            break;
        default: break;
    }
    
    attackCooldown = max(0.0f, attackCooldown - dt);
    animTime += dt * 5.0f;
    animFrame = (hasLOS && type != E_S) ? (static_cast<int>(fmodf(animTime, 2.0f)) == 0 ? 0 : 1) : 0;
}

void Enemy::takeDamage(int d) {
    if (isDead) return;
    health -= d;
    if (health <= 0) { health = 0; isDead = true; animFrame = 3; }
}

int Enemy::getSpriteFrame() const {
    if (isDead) {
        if (deathTimer < 0.3f) return 3;
        if (deathTimer < 0.6f) return 4;
        return 5;
    }
    return animFrame;
}

// Pickup implementation
void Pickup::update(float dt, Player& pl) {
    if (collected) return;
    bobTimer += dt * 3.0f;
    if (coll(pl)) {
        switch (type) {
            case P_AF: pl.heal(25); break;
            case P_RW: pl.heal(50); break;
            case P_LB: pl.addAmmo(20); break;
            case P_BK: pl.addArmor(50); break;
            case P_KC: pl.hasKeycard = true; break;
            default: break;
        }
        collected = true;
    }
}

// Door implementation
bool Door::tryOpen(Player& pl) {
    if (opened) return true;
    if (locked) {
        if (type == T_DK && pl.hasKeycard) { opened = true; return true; }
        return false;
    }
    if (type == T_DE) { opened = true; return true; }
    return false;
}

// Renderer implementation
void Renderer::loadTextures() {
    // Load wall textures
    for (int i = 1; i < T_CNT; i++)
        wallTextures[i] = LoadTexture(TextFormat("%s%s", AP, WT[i]));
    for (int i = 2; i < S_CNT; i++)
        spriteTextures[i] = LoadTexture(TextFormat("%s%s", AP, SF[i]));
    hudPanel = LoadTexture(TextFormat("%shud_panel.png", AP));
    hudFace = LoadTexture(TextFormat("%shud_face.png", AP));
    weaponTextures[0] = LoadTexture(TextFormat("%sweapon_stokbrood.png", AP));
    weaponTextures[1] = LoadTexture(TextFormat("%sweapon_prijspistool.png", AP));
    
    // Create wall model - a cube mesh
    Mesh cubeMesh = GenMeshCube(1.0f, WH, 1.0f);
    wallModel = LoadModelFromMesh(cubeMesh);
    
    // Create floor model - thin cube for floor/ceiling
    Mesh floorMesh = GenMeshCube(1.0f, 0.01f, 1.0f);
    floorModel = LoadModelFromMesh(floorMesh);
    
    // Create materials for each wall type
    for (int i = 1; i < T_CNT; i++) {
        wallMaterials[i] = LoadMaterialDefault();
        wallMaterials[i].maps[MATERIAL_MAP_DIFFUSE].texture = wallTextures[i];
        wallMaterials[i].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
    }
}

void Renderer::updateCamera(const Player& pl) {
    camera.position = Vector3{pl.pos.x, PH + pl.bobOffset, pl.pos.y};
    camera.target = Vector3{pl.pos.x + cosf(pl.angle), PH + pl.bobOffset, pl.pos.y + sinf(pl.angle)};
}

void Renderer::drawWorld(const Map& m, const vector<Door>& doors) {
    BeginMode3D(camera);
    
    // Floor and ceiling - use textured models
    for (int y = 0; y < m.h; y++) {
        for (int x = 0; x < m.w; x++) {
            if (m.t[y][x] == T_E) continue;
            
            // Check if this tile is in the freezer section
            bool isFreezer = false;
            if (x >= 25 && x < 35 && y >= 5 && y < 15) {
                isFreezer = true;
            }
            
            // Floor
            if (m.t[y][x] != T_E) {
                if (T_FL > 0 && T_FL < T_CNT && wallTextures[T_FL].id != 0) {
                    floorModel.materials[0] = wallMaterials[T_FL];
                    Vector3 floorPos = {static_cast<float>(x * TS + 0.5f), 0, static_cast<float>(y * TS + 0.5f)};
                    Color tint = isFreezer ? DARKBLUE : WHITE;
                    floorModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = tint;
                    DrawModel(floorModel, floorPos, 1.0f, WHITE);
                } else {
                    DrawCubeV({static_cast<float>(x * TS + 0.5f), 0, static_cast<float>(y * TS + 0.5f)}, {1, 0.01f, 1}, BLANK);
                }
                
                // Ceiling
                if (T_CE > 0 && T_CE < T_CNT && wallTextures[T_CE].id != 0) {
                    floorModel.materials[0] = wallMaterials[T_CE];
                    Vector3 ceilPos = {static_cast<float>(x * TS + 0.5f), WH, static_cast<float>(y * TS + 0.5f)};
                    Color tint = isFreezer ? ColorAlpha(DARKBLUE, 0.7f) : WHITE;
                    floorModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = tint;
                    DrawModel(floorModel, ceilPos, 1.0f, WHITE);
                } else {
                    DrawCubeV({static_cast<float>(x * TS + 0.5f), WH, static_cast<float>(y * TS + 0.5f)}, {1, 0.01f, 1}, DARKGRAY);
                }
            }
        }
    }
    
    // Walls
    for (int y = 0; y < m.h; y++) {
        for (int x = 0; x < m.w; x++) {
            if (!m.s[y][x] || m.t[y][x] == T_E) continue;
            bool isDoor = false;
            TT dt = static_cast<TT>(m.t[y][x]);
            for (const Door& d : doors) {
                if (d.tileX == x && d.tileY == y && !d.opened) { isDoor = true; dt = d.type; break; }
            }
            
            // Check if this wall is in the freezer section for lighting
            bool isFreezer = (x >= 25 && x < 35 && y >= 5 && y < 15);
            
            // Use textured model for walls
            if (dt > T_E && dt < T_CNT && wallMaterials[dt].maps[MATERIAL_MAP_DIFFUSE].texture.id != 0) {
                wallModel.materials[0] = wallMaterials[dt];
                Vector3 wallPos = {static_cast<float>(x * TS), 0, static_cast<float>(y * TS)};
                Color tint = (dt == T_F && isFreezer) ? ColorAlpha(DARKBLUE, 0.8f) : WHITE;
                wallModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = tint;
                DrawModel(wallModel, wallPos, 1.0f, WHITE);
            } else {
                // Fallback to colored cube
                Color wc = GRAY;
                switch (dt) {
                    case T_SF: wc = BROWN; break;
                    case T_SE: wc = DARKBROWN; break;
                    case T_F: wc = SKYBLUE; break;
                    case T_P: wc = LIGHTGRAY; break;
                    case T_C: wc = BEIGE; break;
                    case T_M: wc = DARKGRAY; break;
                    case T_DK: wc = ORANGE; break;
                    case T_DE: wc = GREEN; break;
                    default: wc = GRAY; break;
                }
                DrawCubeV({static_cast<float>(x * TS), 0, static_cast<float>(y * TS)}, {1, WH, 1}, wc);
            }
        }
    }
    EndMode3D();
}

void Renderer::drawSprites(const vector<Entity*>& ents, const Player& pl) {
    BeginMode3D(camera);
    
    // Sort entities by distance from camera for proper depth sorting
    vector<Entity*> sortedEnts = ents;
    sort(sortedEnts.begin(), sortedEnts.end(), [this](Entity* a, Entity* b) {
        Vector3 aPos = {a->pos.x, 0, a->pos.y};
        Vector3 bPos = {b->pos.x, 0, b->pos.y};
        float distA = Vector3Distance(aPos, camera.position);
        float distB = Vector3Distance(bPos, camera.position);
        return distA > distB; // Draw farther entities first
    });
    
    for (Entity* ee : sortedEnts) {
        if (!ee->active) continue;
        if (Enemy* en = dynamic_cast<Enemy*>(ee)) {
            if (en->isDead && en->deathTimer >= 1.0f) continue;
            int s2 = 0;
            switch (en->type) {
                case E_W: s2 = S_WW; break;
                case E_K: s2 = S_VK; break;
                default: s2 = S_ZS; break;
            }
            Rectangle sr = {static_cast<float>(en->getSpriteFrame() * 64), 0, 64, 64};
            DrawBillboardRec(camera, spriteTextures[s2], sr, {ee->pos.x, PH / 2.0f, ee->pos.y}, {1, 1}, WHITE);
        }
        else if (Pickup* pk = dynamic_cast<Pickup*>(ee)) {
            if (pk->collected) continue;
            int s2 = 0;
            switch (pk->type) {
                case P_AF: s2 = S_AF; break;
                case P_RW: s2 = S_RW; break;
                case P_LB: s2 = S_LB; break;
                case P_BK: s2 = S_BK; break;
                default: s2 = S_KC; break;
            }
            float bob = 0.5f + sinf(pk->bobTimer * 2.0f) * 0.1f + 0.5f;
            DrawBillboardRec(camera, spriteTextures[s2], {0, 0, 32, 32}, {ee->pos.x, bob, ee->pos.y}, {0.5f, 0.5f}, WHITE);
        }
        else if (Projectile* pr = dynamic_cast<Projectile*>(ee)) {
            DrawBillboardRec(camera, spriteTextures[pr->sprite], {0, 0, 16, 16}, {ee->pos.x, 0.5f, ee->pos.y}, {0.5f, 0.5f}, WHITE);
        }
    }
    EndMode3D();
}

void Renderer::drawWeapon(const Player& pl) {
    if (pl.isDead) return;
    int wf = 0;
    if (pl.weaponAnimTime > 0.0f && pl.weaponAnimTime < 0.3f) wf = 1;
    else if (pl.weaponAnimTime >= 0.3f && pl.weaponAnimTime < 0.6f) wf = 2;
    Rectangle sr = {static_cast<float>(wf * 192), 0, 192, 144};
    DrawTextureRec(weaponTextures[pl.currentWeapon], sr,
        {static_cast<float>(SW / 2 - 192 * 0.4f / 2 + 50), static_cast<float>(SH - 144 * 0.4f - 30 + pl.bobOffset * 10)}, WHITE);
}

void Renderer::drawHUD(const Player& pl, int kills, int total, float time) {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    DrawTexturePro(hudPanel, {0, 0, 320, 48}, {0, static_cast<float>(sh - 48), static_cast<float>(sw), 48}, {0, 0}, 0, WHITE);
    DrawText(TextFormat("HP: %d%%", pl.health), 20, sh - 40, 20, WHITE);
    DrawText(TextFormat("AMMO: %d", pl.ammo[1]), 120, sh - 40, 20, WHITE);
    DrawText(TextFormat("ARMOR: %d%%", pl.armor), 220, sh - 40, 20, WHITE);
    if (pl.hasKeycard) DrawTexturePro(spriteTextures[S_KC], {0, 0, 32, 32},
        {static_cast<float>(sw - 40), static_cast<float>(sh - 40), 32, 32}, {0, 0}, 0, WHITE);
    int ff = pl.health <= 0 ? 2 : (pl.health < 50 ? 1 : 0);
    DrawTextureRec(hudFace, {static_cast<float>(ff * 48), 0, 48, 56}, {static_cast<float>(sw - 60), static_cast<float>(sh - 48)}, WHITE);
}

void Renderer::drawTitle() {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    ClearBackground(BLACK);
    DrawText("AH: HELL AISLE", sw / 2 - MeasureText("AH: HELL AISLE", 60) / 2, sh / 3, 60, RED);
    DrawText("A Doom clone in an Albert Heijn", sw / 2 - MeasureText("A Doom clone in an Albert Heijn", 24) / 2, sh / 2, 24, WHITE);
    DrawText("Press any key to start", sw / 2 - MeasureText("Press any key to start", 24) / 2, sh * 2 / 3, 24, GREEN);
    DrawText("WASD: Move | Mouse: Look | LMB/Ctrl: Fire | 1/2: Weapons | E: Use | Esc: Quit",
        sw / 2 - MeasureText("WASD: Move | Mouse: Look | LMB/Ctrl: Fire | 1/2: Weapons | E: Use | Esc: Quit", 20) / 2, sh * 5 / 6, 20, WHITE);
}

void Renderer::drawDeath(int kills, int total, float time) {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    ClearBackground(BLACK);
    DrawText("GESLOTEN", sw / 2 - MeasureText("GESLOTEN", 80) / 2, sh / 3, 80, RED);
    DrawText(TextFormat("Time: %.1fs | Kills: %d/%d", time, kills, total),
        sw / 2 - MeasureText(TextFormat("Time: %.1fs | Kills: %d/%d", time, kills, total), 30) / 2, sh / 2 + 20, 30, WHITE);
    DrawText("Press R to restart", sw / 2 - MeasureText("Press R to restart", 24) / 2, sh * 2 / 3, 24, GREEN);
}

void Renderer::drawVictory(int kills, int total, float time) {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    ClearBackground(BLACK);
    DrawText("YOU ESCAPED!", sw / 2 - MeasureText("YOU ESCAPED!", 80) / 2, sh / 3, 80, GREEN);
    DrawText(TextFormat("Time: %.1fs | Kills: %d/%d", time, kills, total),
        sw / 2 - MeasureText(TextFormat("Time: %.1fs | Kills: %d/%d", time, kills, total), 30) / 2, sh / 2 + 20, 30, WHITE);
    DrawText("Press R to restart", sw / 2 - MeasureText("Press R to restart", 24) / 2, sh * 2 / 3, 24, GREEN);
}

void Renderer::unloadModels() {
    if (wallModel.meshCount > 0) {
        UnloadModel(wallModel);
        wallModel = {0};
    }
    if (floorModel.meshCount > 0) {
        UnloadModel(floorModel);
        floorModel = {0};
    }
    for (int i = 0; i < T_CNT; i++) {
        if (wallMaterials[i].maps[MATERIAL_MAP_DIFFUSE].texture.id != 0) {
            UnloadMaterial(wallMaterials[i]);
            wallMaterials[i] = {0};
        }
    }
}

// Create level
Map createLevel() {
    Map m(48, 48);
    for (int y = 0; y < 48; y++)
        for (int x = 0; x < 48; x++)
            { m.t[y][x] = T_E; m.s[y][x] = false; }
    for (int x = 0; x < 48; x++)
        { m.t[0][x] = T_P; m.s[0][x] = true; m.t[47][x] = T_P; m.s[47][x] = true; }
    for (int y = 1; y < 47; y++)
        { m.t[y][0] = T_P; m.s[y][0] = true; m.t[y][47] = T_P; m.s[y][47] = true; }
    for (int x = 1; x < 10; x++)
        for (int y = 1; y < 8; y++)
            if (x == 1 || y == 1 || x == 9 || y == 7)
                { m.t[y][x] = T_C; m.s[y][x] = true; }
    for (int x = 10; x < 35; x++)
        for (int y = 1; y < 35; y++)
            if ((x % 4 == 0 && y % 6 == 3) || (y % 4 == 0 && x % 6 == 3))
                { m.t[y][x] = T_SF; m.s[y][x] = true; }
    for (int x = 25; x < 35; x++)
        for (int y = 5; y < 15; y++)
            { m.t[y][x] = T_F; m.s[y][x] = (x == 25 || x == 34 || y == 5 || y == 14); }
    for (int x = 35; x < 47; x++)
        for (int y = 1; y < 25; y++)
            m.s[y][x] = (x == 35 || x == 46 || y == 1 || y == 24);
    m.t[20][35] = T_DK; m.s[20][35] = true;
    m.t[2][46] = T_DE; m.s[2][46] = true;
    m.t[8][15] = T_SE; m.s[8][15] = true;
    m.t[20][25] = T_SE; m.s[20][25] = true;
    m.t[15][20] = T_SE; m.s[15][20] = true;
    return m;
}

// Main game loop
int main() {
    InitWindow(SW, SH, "AH: Hell Aisle");
    SetTargetFPS(60);
    
    // Seed random number generator for spread
    srand(static_cast<unsigned int>(time(nullptr)));

    GS state = G_T;
    float gameTime = 0;
    int kills = 0;
    bool cursorHidden = false;
    Map map = createLevel();
    vector<Entity*> entities;
    vector<Door> doors;
    vector<Enemy*> enemies;
    vector<Pickup*> pickups;
    vector<Entity*> projectiles;
    Player* player = nullptr;
    Renderer renderer;
    renderer.loadTextures();

    doors.emplace_back(35, 20, T_DK, true);
    doors.emplace_back(46, 2, T_DE, false);
    entities.push_back(player = new Player());
    player->pos = {3, 3};

    enemies.push_back(new Enemy(E_W, {12.5f, 5.5f}));
    enemies.push_back(new Enemy(E_W, {18.5f, 12.5f}));
    enemies.push_back(new Enemy(E_W, {22.5f, 8.5f}));
    enemies.push_back(new Enemy(E_W, {14.5f, 20.5f}));
    enemies.push_back(new Enemy(E_W, {28.5f, 25.5f}));
    enemies.push_back(new Enemy(E_W, {10.5f, 28.5f}));
    enemies.push_back(new Enemy(E_K, {25.5f, 3.5f}));
    enemies.push_back(new Enemy(E_K, {20.5f, 18.5f}));
    enemies.push_back(new Enemy(E_K, {32.5f, 12.5f}));
    enemies.push_back(new Enemy(E_K, {12.5f, 25.5f}));
    enemies.push_back(new Enemy(E_K, {30.5f, 22.5f}));
    enemies.push_back(new Enemy(E_S, {15.5f, 5.5f}));
    enemies.push_back(new Enemy(E_S, {28.5f, 8.5f}));
    enemies.push_back(new Enemy(E_S, {18.5f, 30.5f}));
    enemies.push_back(new Enemy(E_S, {10.5f, 15.5f}));
    enemies.push_back(new Enemy(E_W, {32.5f, 5.5f}));
    enemies.push_back(new Enemy(E_K, {15.5f, 10.5f}));
    enemies.push_back(new Enemy(E_S, {25.5f, 25.5f}));
    enemies.push_back(new Enemy(E_W, {30.5f, 30.5f}));
    enemies.push_back(new Enemy(E_K, {8.5f, 8.5f}));
    for (Enemy* e : enemies) entities.push_back(e);

    pickups.push_back(new Pickup(P_AF, {5.5f, 3.5f}));
    pickups.push_back(new Pickup(P_AF, {25.5f, 20.5f}));
    pickups.push_back(new Pickup(P_RW, {12.5f, 32.5f}));
    pickups.push_back(new Pickup(P_RW, {30.5f, 5.5f}));
    pickups.push_back(new Pickup(P_LB, {8.5f, 12.5f}));
    pickups.push_back(new Pickup(P_LB, {22.5f, 28.5f}));
    pickups.push_back(new Pickup(P_LB, {18.5f, 8.5f}));
    pickups.push_back(new Pickup(P_BK, {15.5f, 25.5f}));
    pickups.push_back(new Pickup(P_BK, {28.5f, 15.5f}));
    pickups.push_back(new Pickup(P_KC, {20.5f, 3.5f}));
    for (Pickup* p : pickups) entities.push_back(p);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        gameTime += dt;

        if (state == G_T) {
            if (IsKeyPressed(KEY_ESCAPE)) break;
            bool anyKey = false;
            if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_ENTER))
                anyKey = true;
            for (int k = 0; k < 256; k++)
                if (IsKeyDown(k)) { anyKey = true; break; }
            if (anyKey) {
                state = G_P;
                HideCursor();
                DisableCursor();
                cursorHidden = true;
                gameTime = 0;
                kills = 0;
                player->pos = {3, 3};
                player->health = 100;
                player->armor = 0;
                player->hasKeycard = false;
                player->isDead = false;
                player->currentWeapon = W_S;
                player->pendingWeapon = W_S;
                player->weaponCooldown = 0;
                player->ammo[1] = 40;
                for (Enemy* e : enemies) { e->health = EH[e->type]; e->isDead = false; e->deathTimer = 0; }
                for (Pickup* p : pickups) p->collected = false;
                for (Entity* pr : projectiles) delete pr;
                projectiles.clear();
                for (Door& d : doors) d.opened = false;
            }
        }
        else if (state == G_P) {
            if (!cursorHidden) { HideCursor(); DisableCursor(); cursorHidden = true; }
            if (IsKeyPressed(KEY_ESCAPE)) { state = G_T; ShowCursor(); EnableCursor(); cursorHidden = false; continue; }
            if (IsKeyPressed(KEY_ONE)) player->switchWeapon(W_S);
            if (IsKeyPressed(KEY_TWO)) player->switchWeapon(W_P);

            if ((IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsKeyDown(KEY_LEFT_CONTROL)) && player->canFire()) {
                player->startFire();
                if (player->currentWeapon == W_P) {
                    // Add small random spread to Prijspistool
                    float spread = (rand() / (float)RAND_MAX) * 0.05f - 0.025f; // +/- ~2.86 degrees
                    Vector2 shootDir = {cosf(player->angle + spread), sinf(player->angle + spread)};
                    for (Enemy* e : enemies) {
                        if (e->isDead) continue;
                        Vector2 toEnemy = Vector2Subtract(e->pos, player->pos);
                        float dist = Vector2Length(toEnemy);
                        if (dist > WR[1] || dist < 0.01f) continue;
                        toEnemy = Vector2Normalize(toEnemy);
                        float angleToEnemy = atan2f(toEnemy.y, toEnemy.x);
                        if (!std::isfinite(angleToEnemy)) continue;
                        float angleDiff = angleToEnemy - player->angle;
                        if (!std::isfinite(angleDiff)) continue;
                        while (angleDiff > PI) angleDiff -= 2 * PI;
                        while (angleDiff < -PI) angleDiff += 2 * PI;
                        if (fabsf(angleDiff) < 0.1f) {
                            bool blocked = false;
                            Vector2 checkPos = Vector2Add(player->pos, Vector2Scale(shootDir, 0.5f));
                            float checkDist = dist - player->r - e->r;
                            while (checkDist > 0.2f) {
                                int tx = static_cast<int>(checkPos.x / TS), ty = static_cast<int>(checkPos.y / TS);
                                if (tx >= 0 && tx < map.w && ty >= 0 && ty < map.h) {
                                    if (map.isS(tx, ty)) { blocked = true; break; }
                                    for (const Door& d : doors) {
                                        if (d.tileX == tx && d.tileY == ty && d.isBlocking()) {
                                            blocked = true; break;
                                        }
                                    }
                                    if (blocked) break;
                                }
                                checkPos = Vector2Add(checkPos, Vector2Scale(shootDir, 0.2f));
                                checkDist -= 0.2f;
                            }
                            if (!blocked) { e->takeDamage(WD[1]); if (e->isDead) kills++; break; }
                        }
                    }
                } else {
                    for (Enemy* e : enemies) {
                        if (e->isDead) continue;
                        Vector2 toEnemy = Vector2Subtract(e->pos, player->pos);
                        float dist = Vector2Length(toEnemy);
                        if (dist <= WR[0] + e->r && dist > 0.01f) {
                            float angleToEnemy = atan2f(toEnemy.y, toEnemy.x);
                            float angleDiff = angleToEnemy - player->angle;
                            while (angleDiff > PI) angleDiff -= 2 * PI;
                            while (angleDiff < -PI) angleDiff += 2 * PI;
                            if (fabsf(angleDiff) < 0.5f) { e->takeDamage(WD[0]); if (e->isDead) kills++; }
                        }
                    }
                }
            }

            player->update(dt, map, doors);
            for (Enemy* e : enemies)
                if (!e->isDead) e->update(dt, *player, map, doors, projectiles);
            for (int i = 0; i < projectiles.size(); i++)
                if (Projectile* pp = dynamic_cast<Projectile*>(projectiles[i])) {
                    pp->update(dt, *player, entities, map, doors);
                    if (!pp->active) { delete pp; projectiles.erase(projectiles.begin() + i); i--; }
                }
            for (Pickup* p : pickups) p->update(dt, *player);

            // Check if player is touching exit door
            for (Door& d : doors) {
                if (d.type == T_DE && !d.opened) {
                    int dx = abs(d.tileX - static_cast<int>(player->pos.x / TS));
                    int dy = abs(d.tileY - static_cast<int>(player->pos.y / TS));
                    if ((dx <= 1 && dy <= 1) || Vector2Distance(player->pos, d.pos) < 1.0f) {
                        if (d.tryOpen(*player)) {
                            state = G_V; ShowCursor(); EnableCursor(); cursorHidden = false;
                        }
                    }
                }
            }

            if (IsKeyPressed(KEY_E))
                for (Door& d : doors) {
                    if ((abs(d.tileX - static_cast<int>(player->pos.x / TS)) <= 1 &&
                         abs(d.tileY - static_cast<int>(player->pos.y / TS)) <= 1) ||
                        Vector2Distance(player->pos, d.pos) < 1.5f)
                        d.tryOpen(*player);
                }

            if (player->isDead) { state = G_D; ShowCursor(); EnableCursor(); cursorHidden = false; }
        }
        else if (state == G_D || state == G_V) {
            if (IsKeyPressed(KEY_R)) {
                state = G_P;
                HideCursor();
                DisableCursor();
                cursorHidden = true;
                gameTime = 0;
                kills = 0;
                player->pos = {3, 3};
                player->health = 100;
                player->armor = 0;
                player->hasKeycard = false;
                player->isDead = false;
                player->currentWeapon = W_S;
                player->pendingWeapon = W_S;
                player->weaponCooldown = 0;
                player->ammo[1] = 40;
                for (Enemy* e : enemies) { e->health = EH[e->type]; e->isDead = false; e->deathTimer = 0; }
                for (Pickup* p : pickups) p->collected = false;
                for (Entity* pr : projectiles) delete pr;
                projectiles.clear();
                for (Door& d : doors) d.opened = false;
            }
            if (IsKeyPressed(KEY_ESCAPE)) { state = G_T; ShowCursor(); EnableCursor(); cursorHidden = false; }
        }

        renderer.updateCamera(*player);
        BeginDrawing();
        ClearBackground(DARKGRAY);

        if (state == G_T) renderer.drawTitle();
        else if (state == G_D) {
            renderer.drawWorld(map, doors);
            renderer.drawSprites(entities, *player);
            renderer.drawHUD(*player, kills, static_cast<int>(enemies.size()), gameTime);
            renderer.drawDeath(kills, static_cast<int>(enemies.size()), gameTime);
        }
        else if (state == G_V) {
            renderer.drawWorld(map, doors);
            renderer.drawSprites(entities, *player);
            renderer.drawHUD(*player, kills, static_cast<int>(enemies.size()), gameTime);
            renderer.drawVictory(kills, static_cast<int>(enemies.size()), gameTime);
        }
        else {
            renderer.drawWorld(map, doors);
            renderer.drawSprites(entities, *player);
            renderer.drawWeapon(*player);
            renderer.drawHUD(*player, kills, static_cast<int>(enemies.size()), gameTime);
        }

        EndDrawing();
    }

    renderer.unloadModels();
    for (int i = 0; i < T_CNT; i++)
        if (renderer.wallTextures[i].id) UnloadTexture(renderer.wallTextures[i]);
    for (int i = 0; i < S_CNT; i++)
        if (renderer.spriteTextures[i].id) UnloadTexture(renderer.spriteTextures[i]);
    for (int i = 0; i < 2; i++)
        if (renderer.weaponTextures[i].id) UnloadTexture(renderer.weaponTextures[i]);
    if (renderer.hudPanel.id) UnloadTexture(renderer.hudPanel);
    if (renderer.hudFace.id) UnloadTexture(renderer.hudFace);
    for (Entity* e : entities) delete e;
    for (Entity* p : projectiles) delete p;

    CloseWindow();
    return 0;
}
