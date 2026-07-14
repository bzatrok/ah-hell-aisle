#include <raylib.h>
#include "Common.h"
#include "Game.h"
#include "Level.h"

float deltaTimeGlobal = 0.0f;

int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "AH: Hell Aisle");
    SetTargetFPS(60);
    
    Game game;
    game.init();
    
    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();
        
        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        game.update(deltaTime);
        game.draw();
        
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}
