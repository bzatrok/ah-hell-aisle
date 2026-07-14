#pragma once
#include "raylib.h"

#include <string>
#include <vector>

// Core types plus the tuning table. Every number SPEC.md fixes lives in cfg so
// the sim code reads like the spec.

namespace cfg {
// player (§4)
constexpr float PlayerRadius = 0.30f;
constexpr float EyeHeight    = 0.50f;
constexpr float MoveSpeed    = 3.5f;
constexpr float TurnSpeed    = 120.0f * DEG2RAD;
constexpr float MouseSens    = 0.0032f;
constexpr float MaxHealth    = 100.0f;
constexpr float MaxArmour    = 100.0f;
constexpr int   MaxAmmo      = 200;
constexpr int   StartAmmo    = 40;

// weapons (§5)
constexpr float StokDamage      = 25.0f;
constexpr float StokRange       = 1.5f;
constexpr float StokCd          = 0.50f;  // ~2 swings/sec
constexpr float StokConeDeg     = 25.0f;
constexpr float StokImpactDelay = 0.12f;  // damage lands on the impact frame
constexpr float PistDamage      = 20.0f;
constexpr float PistCd          = 0.25f;  // ~4 shots/sec
constexpr float PistSpreadDeg   = 1.6f;

// rendering
constexpr float FogDist = 26.0f;
constexpr int   ScreenW = 1280;
constexpr int   ScreenH = 720;
constexpr int   HudH    = 144;
}  // namespace cfg

enum class Tile : unsigned char {
  Empty, Plain, ShelfFull, ShelfEmpty, Freezer, Checkout, Magazijn,
  DoorKey, DoorExit,
};

enum class Zone : unsigned char { Store, Freezer, Magazijn };

struct Door {
  int x = 0, y = 0;
  Tile type = Tile::DoorKey;
  float open = 0.0f;  // 0 closed .. 1 fully raised
  bool opening = false;
};

enum class EnemyType : int { Winkelwagen = 0, Vakkenvuller = 1, Zelfscanner = 2 };
enum class EnemyState { Idle, Active, Dying, Dead };

struct Enemy {
  EnemyType type{};
  EnemyState state = EnemyState::Idle;
  Vector2 pos{};
  Vector2 moveDir{};
  Vector2 lastSeen{};   // last position the player was spotted at
  float hp = 0;
  float attackCd = 0, attackAnim = 0;
  float walkAnim = 0, dieAnim = 0, hurtFlash = 0;
  float repathT = 0, stagger = 0, losTimer = 0;
  int burstLeft = 0;
  float burstT = 0;
  int strafeSign = 1;
  bool moved = false;  // walked this frame; drives the walk animation
};

enum class PickupType { Appelflap, Rookworst, Labels, Bonuskaart, Keycard };
struct Pickup {
  PickupType type{};
  Vector2 pos{};
  bool taken = false;
};

struct SoupCan {
  Vector2 pos{}, vel{};
  float t = 0, flightT = 1, arcH = 0.4f;
  bool dead = false;
};

struct Tracer {
  Vector3 a{}, b{};
  Color color{};
  float t = 0;
};

struct Player {
  Vector2 pos{};
  float angle = 0;  // yaw, radians
  float hp = cfg::MaxHealth, armour = 0;
  int ammo = cfg::StartAmmo;
  bool hasKeycard = false;
  int weapon = 1;  // 1 = stokbrood, 2 = prijspistool
  float fireCd = 0, attackAnim = 0, attackDur = 1;
  float meleeT = -1;  // countdown to the swing's impact moment
  float bobT = 0, bobAmount = 0;
  float hurtFlash = 0, pickupFlash = 0;
};

struct Map {
  int w = 0, h = 0;
  std::vector<Tile> tiles;
  std::vector<Zone> zones;

  Tile at(int x, int y) const {
    if (x < 0 || y < 0 || x >= w || y >= h) return Tile::Plain;
    return tiles[(size_t)y * w + x];
  }
  Zone zoneAt(int x, int y) const {
    if (x < 0 || y < 0 || x >= w || y >= h) return Zone::Store;
    return zones[(size_t)y * w + x];
  }
};

enum class GameState { Title, Playing, Dead, Won };

struct Game {
  Map map;
  Player pl;
  std::vector<Enemy> enemies;
  std::vector<Pickup> pickups;
  std::vector<SoupCan> cans;
  std::vector<Tracer> tracers;
  std::vector<Door> doors;
  GameState state = GameState::Title;
  float time = 0;
  int kills = 0, totalEnemies = 0;
  std::string msg;
  float msgT = 0;
  float bumpMsgCd = 0;
  float flicker = 1;      // freezer light flicker, recomputed each frame
  int captureGrace = 0;   // frames to ignore mouse delta after cursor capture
};

struct Assets;  // assets.h

// level.cpp
void LoadLevel(Game& g);

// game.cpp — simulation
void UpdateGame(Game& g, const Assets& A, float dt);
bool TileSolid(const Game& g, int x, int y);  // walls + closed doors
bool CircleFits(const Game& g, Vector2 p, float r);
float RaycastWall(const Game& g, Vector2 o, Vector2 dir, float maxDist);
bool LineOfSight(const Game& g, Vector2 a, Vector2 b);
void ShowMessage(Game& g, const char* text);
