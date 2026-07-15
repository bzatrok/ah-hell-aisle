#include "web_net.h"

#include "net.h"

// The net bridge, in the mould of web_input.cpp: JS calls the Module._net_*
// exports below to push the room into gNet, and the JsNet* wrappers call back
// out through EM_JS into window.__net (web/index.html), which owns the actual
// WebSocket and the JSON. Single-threaded either side of the boundary (ASYNCIFY
// yields between frames), so plain reads and writes are safe throughout.

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>

extern "C" {

// --- lifecycle ---------------------------------------------------------------

// The shell has a welcome frame: we know who we are. Flips startRequested, which
// the title screen consumes to enter the arena.
EMSCRIPTEN_KEEPALIVE void net_begin_arena(int myId, int hostId) {
    gNet.myId = myId;
    gNet.hostId = hostId;
    gNet.isHost = (hostId == myId);
    gNet.active = true;
    gNet.startRequested = true;
}

// Host re-election (someone left) — also how a peer learns it now runs the shop.
EMSCRIPTEN_KEEPALIVE void net_set_host(int hostId) {
    gNet.hostId = hostId;
    gNet.isHost = (hostId == gNet.myId);
}

EMSCRIPTEN_KEEPALIVE void net_peer_join(int id, int colorIdx) {
    if (NetFindPeer(id)) return;
    RemotePlayer p;
    p.id = id;
    p.colorIdx = (unsigned char)colorIdx;
    gNet.peers.push_back(p);
    // The host owes every joiner the current pickup state (NetUpdate sends it).
    if (gNet.isHost) gNet.snapshotQueue.push_back(id);
}

EMSCRIPTEN_KEEPALIVE void net_peer_left(int id) {
    for (size_t i = 0; i < gNet.peers.size(); i++) {
        if (gNet.peers[i].id == id) {
            gNet.peers.erase(gNet.peers.begin() + i);
            return;
        }
    }
}

// --- streams -------------------------------------------------------------------

EMSCRIPTEN_KEEPALIVE void net_player_state(int id, float x, float y, float yaw,
                                           int health, int weapon, int flags) {
    RemotePlayer* p = NetFindPeer(id);
    if (!p) {   // state can outrun the join notice; conjure the peer
        net_peer_join(id, 0);
        p = NetFindPeer(id);
        p->pos = p->fromPos = p->toPos = {x, y};
    }

    const bool nowAlive = (flags & 1) != 0;
    if (nowAlive && !p->alive) {
        // Respawn: they came back somewhere else — walking the corpse there
        // would be worse than the pop.
        p->alive = true;
        p->corpseTimer = -1.0f;
        p->pos = p->fromPos = p->toPos = {x, y};
        p->yaw = p->fromYaw = p->toYaw = yaw;
        p->lerpT = 1.0f;
    } else if (p->alive) {
        p->fromPos = p->pos;
        p->toPos = {x, y};
        p->fromYaw = p->yaw;
        p->toYaw = yaw;
        p->lerpT = 0.0f;
        p->lerpDur = p->sinceLast < 0.033f ? 0.033f
                   : p->sinceLast > 0.25f  ? 0.25f
                                           : p->sinceLast;
    }
    // A dead peer's packets keep the bookkeeping warm but never move the corpse.

    p->sinceLast = 0.0f;
    p->health = health;
    p->weapon = (unsigned char)weapon;
}

EMSCRIPTEN_KEEPALIVE void net_enemy_state(int idx, int kind, float x, float y,
                                          int health, int state, float aimx,
                                          float aimy, int flags) {
    if (idx < 0 || idx >= kNetMaxEnemies) return;
    NetEnemyIn& in = gNet.enemyIn[idx];
    in.fresh = true;
    in.kind = (unsigned char)kind;
    in.state = (unsigned char)state;
    in.health = health;
    in.pos = {x, y};
    in.aim = {aimx, aimy};
    in.aimBeam = (flags & 1) != 0;
}

// --- events --------------------------------------------------------------------

EMSCRIPTEN_KEEPALIVE void net_event(int type, int from, float a, float b, float c,
                                    float d) {
    gNet.inbox.push_back({(NetEvent)type, from, a, b, c, d});
}

// --- strings ---------------------------------------------------------------------

// The one string crossing: JS writes UTF-8 into this buffer (stringToUTF8), then
// rings one of the two bells. Kill feed and standings both come composed from JS,
// which owns the names.
static char gMsgBuf[192];

EMSCRIPTEN_KEEPALIVE char* net_msg_buf(void) { return gMsgBuf; }

EMSCRIPTEN_KEEPALIVE void net_post_message(void) {
    gNet.feed.push_back(gMsgBuf);
}

EMSCRIPTEN_KEEPALIVE void net_set_scoreline(void) {
    gNet.scoreline = gMsgBuf;
}

}  // extern "C"

// --- outbound ------------------------------------------------------------------

EM_JS(void, js_net_state, (float x, float y, float yaw, int health, int weapon,
                           int flags), {
    if (window.__net) __net.state(x, y, yaw, health, weapon, flags);
});

EM_JS(void, js_net_event, (int to, int type, float a, float b, float c, float d), {
    if (window.__net) __net.event(to, type, a, b, c, d);
});

EM_JS(void, js_net_enemy_begin, (), {
    if (window.__net) __net.enemyBegin();
});

EM_JS(void, js_net_enemy, (int idx, int kind, float x, float y, int health,
                           int state, float aimx, float aimy, int flags), {
    if (window.__net) __net.enemy(idx, kind, x, y, health, state, aimx, aimy, flags);
});

EM_JS(void, js_net_enemy_flush, (), {
    if (window.__net) __net.enemyFlush();
});

EM_JS(void, js_net_leave, (), {
    if (window.__net) __net.leave();
});

void JsNetState(Vector2 pos, float yaw, int health, int weapon, int flags) {
    js_net_state(pos.x, pos.y, yaw, health, weapon, flags);
}

void JsNetLeave() { js_net_leave(); }

void JsNetEvent(int to, int type, float a, float b, float c, float d) {
    js_net_event(to, type, a, b, c, d);
}

void JsNetEnemyBegin() { js_net_enemy_begin(); }

void JsNetEnemy(int idx, int kind, Vector2 pos, int health, int state, Vector2 aim,
                int flags) {
    js_net_enemy(idx, kind, pos.x, pos.y, health, state, aim.x, aim.y, flags);
}

void JsNetEnemyFlush() { js_net_enemy_flush(); }

#else  // native: the arena is unreachable, the link still needs the symbols

void JsNetState(Vector2, float, int, int, int) {}
void JsNetLeave() {}
void JsNetEvent(int, int, float, float, float, float) {}
void JsNetEnemyBegin() {}
void JsNetEnemy(int, int, Vector2, int, int, Vector2, int) {}
void JsNetEnemyFlush() {}

#endif
