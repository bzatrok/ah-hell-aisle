#include "game.h"

#include "audio.h"
#include "hud.h"
#include "raylib.h"
#include "render.h"

namespace {

void Restart(Game& g) {
    WorldStartRun(g.world);
    RenderRebuild(g.world.map);
    g.state = GameState::Playing;
    DisableCursor();
}

}  // namespace

void GameInit(Game& g) {
    WorldStartRun(g.world);      // so the title screen has a world to hold
    g.state = GameState::Title;
    EnableCursor();
}

void GameUpdate(Game& g, float dt) {
    AudioUpdate(dt, g.state == GameState::Playing,
                g.state == GameState::Title ? -1 : g.world.level);

    if (IsKeyPressed(KEY_M)) {
        g.world.Message(AudioToggleMusic() ? "MUZIEK AAN" : "MUZIEK UIT");
    }

#if defined(__EMSCRIPTEN__)
    // In a browser there is no quitting: raylib's web main loop never honours the
    // exit key, and while the mouse is captured Chrome eats the first Esc to
    // release pointer lock. So Esc abandons the run and returns to the title.
    if (g.state != GameState::Title && IsKeyPressed(KEY_ESCAPE)) {
        g.state = GameState::Title;
        EnableCursor();
        return;
    }
#endif

    switch (g.state) {
        case GameState::Title: {
            // Esc is not "any key": on the web it just brought us here. Neither is
            // M — the title advertises it as the music toggle, and it already did
            // that above.
            const int key = GetKeyPressed();
            if ((key != 0 && key != KEY_ESCAPE && key != KEY_M) ||
                IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Restart(g);
            }
            break;
        }

        case GameState::Playing:
            WorldUpdate(g.world, dt);

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
            if (IsKeyPressed(KEY_R)) {
                // The current level again, with what you walked in carrying.
                WorldRestartLevel(g.world);
                RenderRebuild(g.world.map);
                g.state = GameState::Playing;
                DisableCursor();
            }
            break;

        case GameState::Escaped:
            if (IsKeyPressed(KEY_R)) Restart(g);   // a victory lap starts over
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
