#pragma once
#include "raylib.h"

// The outbound half of the net bridge: thin typed wrappers over EM_JS calls into
// the shell's window.__net (web/index.html), which packs them into relay frames.
// Native builds get no-op stubs — the arena is unreachable there, but everything
// still compiles and links. The inbound half needs no header: it is the
// EMSCRIPTEN_KEEPALIVE net_* exports in web_net.cpp, called only from JS.

void JsNetState(Vector2 pos, float yaw, int health, int weapon, int flags);

// to = 0 broadcasts to the room; a peer id targets one member (used for the
// host's pickup-state catch-up to a late joiner).
void JsNetEvent(int to, int type, float a, float b, float c, float d);

// The host's enemy stream. Begin/flush bracket one 10 Hz tick so JS can pack
// every slot into a single relay frame instead of sixteen.
void JsNetEnemyBegin();
void JsNetEnemy(int idx, int kind, Vector2 pos, int health, int state,
                Vector2 aim, int flags);
void JsNetEnemyFlush();
