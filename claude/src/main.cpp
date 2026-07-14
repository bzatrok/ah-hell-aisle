#include "assets.h"
#include "audio.h"
#include "config.h"
#include "game.h"
#include "raylib.h"
#include "render.h"

int main() {
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(kScreenW, kScreenH, "AH: Hell Aisle");
    SetTargetFPS(60);

    AssetsLoad();
    AudioInit();

    Game game;
    GameInit(game);
    RenderInit(game.world.map);   // the level never changes, so the geometry is built once

    while (!WindowShouldClose()) {   // Esc quits, per SPEC §4
        // A stall — a window drag, a breakpoint — must not teleport anyone through a
        // shelf, so a frame is never worth more than 50ms of simulation.
        const float dt = (GetFrameTime() > 0.05f) ? 0.05f : GetFrameTime();

        GameUpdate(game, dt);

        BeginDrawing();
        GameDraw(game, dt);
        EndDrawing();
    }

    RenderShutdown();
    AudioShutdown();
    AssetsUnload();
    CloseWindow();
    return 0;
}
