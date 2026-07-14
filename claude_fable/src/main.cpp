// AH: Hell Aisle — a Doom 2 clone set in an Albert Heijn after closing time.
// Entry point: window, game-state machine, main loop.
#include "raylib.h"

#include "assets.h"
#include "game.h"
#include "hud.h"
#include "render.h"

static void StartRun(Game& g) {
  Game fresh{};
  LoadLevel(fresh);
  fresh.state = GameState::Playing;
  fresh.captureGrace = 3;  // swallow the mouse jump from cursor capture
  g = fresh;
  DisableCursor();
}

int main() {
  SetTraceLogLevel(LOG_WARNING);
  SetConfigFlags(FLAG_VSYNC_HINT);
  InitWindow(cfg::ScreenW, cfg::ScreenH, "AH: Hell Aisle");
  SetTargetFPS(60);
  InitAudioDevice();  // optional: the game runs silent if this fails

  Assets assets = LoadAssets();
  Game game{};

  while (!WindowShouldClose()) {  // ESC quits
    float dt = GetFrameTime();
    if (dt > 1.0f / 30.0f) dt = 1.0f / 30.0f;

    switch (game.state) {
      case GameState::Title:
        if (GetKeyPressed() != 0 || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
          StartRun(game);
        break;
      case GameState::Playing:
        UpdateGame(game, assets, dt);
        break;
      case GameState::Dead:
      case GameState::Won:
        if (IsKeyPressed(KEY_R)) StartRun(game);
        break;
    }

    BeginDrawing();
    ClearBackground(BLACK);
    if (game.state == GameState::Title) {
      DrawTitleScreen(assets, (float)GetTime());
    } else {
      DrawWorld(game, assets);
      DrawWeapon(game, assets);
      DrawStatusBar(game, assets);
      if (game.state == GameState::Playing) DrawPlayOverlays(game);
      else DrawEndScreen(game);
    }
    EndDrawing();
  }

  UnloadAssets(assets);
  CloseAudioDevice();
  CloseWindow();
  return 0;
}
