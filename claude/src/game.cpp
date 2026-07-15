#include "game.h"

#include "audio.h"
#include "hud.h"
#include "net.h"
#include "raylib.h"
#include "render.h"
#include "web_input.h"

namespace {

void Restart(Game& g) {
    WorldStartRun(g.world);
    RenderRebuild(g.world.map);
    g.state = GameState::Playing;
    DisableCursor();
}

void EnterArena(Game& g) {
    WorldStartArena(g.world);
    RenderRebuild(g.world.map);
    g.state = GameState::Playing;
    DisableCursor();
}

// Arena deaths don't end anything: the corpse lies there for a beat (the camera
// fall PlayerUpdate already plays), then a fresh body walks out of a quiet '@'.
// gNet.respawnTimer doubles as the just-died edge: 0 means death hasn't been
// noticed yet, so the Died broadcast fires exactly once per death.
void UpdateArenaRespawn(World& w, float dt) {
    if (!w.player.dead()) return;

    if (gNet.respawnTimer <= 0.0f) {
        NetSendDied(gNet.lastHitBy);
        gNet.respawnTimer = kNetRespawnDelay;
    } else {
        gNet.respawnTimer -= dt;
        if (gNet.respawnTimer <= 0.0f) {
            WorldArenaRespawn(w);
            gNet.respawnTimer = 0.0f;   // armed for the next death
        }
    }
}

}  // namespace

void GameInit(Game& g) {
    WorldStartRun(g.world);      // so the title screen has a world to hold
    g.state = GameState::Title;
    EnableCursor();
}

void GameUpdate(Game& g, float dt) {
#if defined(__EMSCRIPTEN__)
    // A phone held portrait: the shell shows its rotate overlay and raises this
    // flag. Freeze everything — world, input, even the music feed — until the
    // phone turns back. The draw underneath the overlay is harmless.
    // Except in the arena: a live match waits for nobody (Doom rules), so the
    // sim runs on and the rotated phone just stands there, vulnerable.
    if (gWebInput.paused && g.world.mode != Mode::Arena) return;
#endif

    AudioUpdate(dt, g.state == GameState::Playing,
                g.state == GameState::Title ? -1 : g.world.level);

    if (IsKeyPressed(KEY_M)) {
        g.world.Message(AudioToggleMusic() ? "MUZIEK AAN" : "MUZIEK UIT");
    }

#if defined(__EMSCRIPTEN__)
    // In a browser there is no quitting: raylib's web main loop never honours the
    // exit key, and while the mouse is captured Chrome eats the first Esc to
    // release pointer lock. So Esc abandons the run and returns to the title —
    // and from the arena it also hangs up the room.
    if (g.state != GameState::Title && IsKeyPressed(KEY_ESCAPE)) {
        if (g.world.mode == Mode::Arena) NetLeave();
        g.state = GameState::Title;
        EnableCursor();
        return;
    }
#endif

    switch (g.state) {
        case GameState::Title: {
            // The shell joined a room and wants the arena — that outranks keys.
            if (NetConsumeStartRequest()) {
                EnterArena(g);
                break;
            }

            // Esc is not "any key": on the web it just brought us here. Neither is
            // M — the title advertises it as the music toggle, and it already did
            // that above.
            const int key = GetKeyPressed();
            if ((key != 0 && key != KEY_ESCAPE && key != KEY_M) ||
                IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || WebConsumeFirePressed()) {
                Restart(g);
            }
            break;
        }

        case GameState::Playing:
            WorldUpdate(g.world, dt);

            if (g.world.mode == Mode::Arena) {
                UpdateArenaRespawn(g.world, dt);
                break;   // no dock-door ending, no Dead screen — only respawns
            }

            if (g.world.escaped) {
                if (g.world.level + 1 < kLevelCount) {
                    // Through the dock door, into the next building.
                    WorldNextLevel(g.world);
                    RenderRebuild(g.world.map);
                    PlaySfx(Sfx::LevelDone);
                } else {
                    g.state = GameState::Escaped;
                    EnableCursor();
                    PlaySfx(Sfx::Escaped);
                }
            } else if (g.world.player.dead()) {
                g.state = GameState::Dead;
                EnableCursor();
            }
            break;

        case GameState::Dead:
            // The shop carries on without you. It just does not take your input.
            WorldUpdate(g.world, dt);
            if (IsKeyPressed(KEY_R) || WebConsumeFirePressed()) {
                // The current level again, with what you walked in carrying.
                WorldRestartLevel(g.world);
                RenderRebuild(g.world.map);
                g.state = GameState::Playing;
                DisableCursor();
            }
            break;

        case GameState::Escaped:
            // A tap restarts too, same as the Dead screen.
            if (IsKeyPressed(KEY_R) || WebConsumeFirePressed()) Restart(g);
            break;
    }
}

void GameDraw(Game& g, float dt) {
    if (g.state == GameState::Title) {
        ScreenTitle();
        return;
    }

    RenderScene(g.world, dt);
    RenderWeapon(g.world);
    HudDraw(g.world);

    if (g.state == GameState::Dead) ScreenDead(g.world);
    if (g.state == GameState::Escaped) ScreenEscaped(g.world);
}
