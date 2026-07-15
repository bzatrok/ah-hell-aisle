#pragma once
#include <string>
#include <vector>

#include "raylib.h"

struct World;

// The multiplayer arena's state. One authority rule everywhere: you own what you
// can lie about least. Movement and health are the *owner's* word (I stream my
// position, I apply damage to myself); hits are the *shooter's* word (my ray
// crossed you, I tell you); the monsters are the *host's* word (oldest member
// simulates them, everyone else puppets). The relay only routes.

// Another shopper in the arena, as last heard over the wire. Positions arrive at
// ~15 Hz and are interpolated between the last two packets; the rest applies as
// received.
struct RemotePlayer {
    int id = 0;
    unsigned char colorIdx = 0;
    unsigned char weapon = 0;
    bool alive = true;
    int health = 100;

    Vector2 pos{};           // interpolated — the position everyone reads
    Vector2 fromPos{};       // segment start (where we were when the packet came)
    Vector2 toPos{};         // segment end (the packet's position)
    float fromYaw = 0.0f;
    float toYaw = 0.0f;
    float yaw = 0.0f;
    float lerpT = 1.0f;      // 0..1 across the current segment
    float lerpDur = 0.1f;    // measured packet gap, clamped
    float sinceLast = 0.0f;  // time since the last state packet

    float fireAnim = 0.0f;   // drives the attack frame, set by Fired events
    float hurtFlash = 0.0f;
    float corpseTimer = -1.0f;  // >=0: dead, counts up through the dying frames

    bool moving = false;     // the current segment covers real ground
    float animTimer = 0.0f;  // walk cycle, ticked locally while moving
};

// Event types crossing the JS bridge, both directions. The JS protocol layer in
// web/index.html mirrors these numbers — change them together.
enum class NetEvent : int {
    Fired = 0,          // a=weaponId                        anyone -> everyone
    HitMe = 1,          // a=damage, b=1: the shop did it    shooter/host -> victim
    EnemyHit = 2,       // a=enemyIdx, b=damage              shooter -> host
    Died = 3,           // a=killerId (0 = the shop)         victim -> everyone
    PickupTaken = 4,    // a=pickupIdx                       taker -> everyone
    PickupRespawn = 5,  // a=pickupIdx                       host -> everyone
    Rocket = 6,         // a,b=pos c,d=vel                   shooter -> everyone
};

struct PendingEvent {
    NetEvent type;
    int from = 0;   // sender id, stamped by the relay
    float a = 0, b = 0, c = 0, d = 0;
};

// Latest streamed state per enemy slot, host -> puppets. Last write wins: a
// burst of packets between frames collapses to the newest, which is the point.
struct NetEnemyIn {
    bool fresh = false;
    unsigned char kind = 0;
    unsigned char state = 0;
    int health = 0;
    Vector2 pos{};
    Vector2 aim{};
    bool aimBeam = false;
};

constexpr int kNetMaxEnemies = 16;
constexpr int kNetMaxPickups = 48;
constexpr float kNetRespawnDelay = 2.5f;     // my death -> back on the floor
constexpr float kNetPickupRespawn = 18.0f;   // host: taken -> back on the floor

struct NetState {
    bool active = false;      // in the arena (or committed to entering it)
    bool isHost = false;      // I run the monsters
    int myId = 0;
    int hostId = 0;
    bool startRequested = false;   // set by the shell, consumed by the title screen

    std::vector<RemotePlayer> peers;
    std::vector<PendingEvent> inbox;       // events awaiting a World to apply to
    std::vector<std::string> feed;         // JS-composed lines awaiting World::Message
    std::string scoreline;                 // JS-composed standings, drawn by the HUD

    NetEnemyIn enemyIn[kNetMaxEnemies];

    int frags = 0;
    int deaths = 0;
    float respawnTimer = 0.0f;   // >0: I'm dead, counting down
    int lastHitBy = 0;           // credit for my next death goes here
    float lastHitByAge = 0.0f;   // ...unless it happened too long ago

    float stateAccum = 0.0f;     // paces my 15 Hz state stream
    float enemyAccum = 0.0f;     // paces the host's 10 Hz enemy stream
    float pickupTimer[kNetMaxPickups] = {};   // host: countdown to respawn
    float trolleyTimer = 0.0f;   // host: paces winkelwagen restocking
    bool hostArmed = false;      // host duties initialised (fresh or inherited)
    std::vector<int> snapshotQueue;   // host: joiners owed the pickup state
};

extern NetState gNet;

// True once the shell has put us in (or committed us to) the arena.
inline bool NetArena() { return gNet.active; }

RemotePlayer* NetFindPeer(int id);

// The shell's "enter the arena" edge — consumed by the title screen, like the
// web_input Consume* family.
bool NetConsumeStartRequest();

// Local exit (Esc): tell the shell to hang up and forget the room.
void NetLeave();

// Clears match state for a fresh arena entry; identity (myId/isHost) survives.
void NetResetMatch();

// The per-frame pump, arena only: drains inbound events into the world, ticks
// peer interpolation and corpse timers, streams my state, and — on the host —
// streams the monsters and respawns pickups. Call order inside WorldUpdate:
// after PlayerUpdate, so the state that streams out is this frame's.
void NetUpdate(World& w, float dt);

// Shooter-side damage out (victims apply on receipt), plus the rest of the
// outbound vocabulary. All no-ops outside the arena.
void NetSendFired(int weaponId);
void NetSendHitPlayer(int targetId, int damage);
void NetSendShopHit(int targetId, int damage);   // a monster's hit: no kill credit
void NetSendDied(int killerId);
void NetSendRocket(Vector2 pos, Vector2 vel);

// Enemy damage routes to whoever runs the sim: applied straight to the world on
// the host, sent to the host from anyone else.
void NetDamageEnemy(World& w, int enemyIdx, int damage);

// I walked over pickup `idx`: tell the room, and start its respawn clock if the
// clock lives here (host).
void NetOnLocalPickupTaken(int idx);
