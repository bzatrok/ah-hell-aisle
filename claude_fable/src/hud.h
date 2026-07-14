#pragma once
#include "assets.h"
#include "game.h"

// Screen-space drawing: first-person weapon, status bar, damage/pickup
// flashes, HUD messages, and the title / end screens.
void DrawWeapon(const Game& g, const Assets& A);
void DrawStatusBar(const Game& g, const Assets& A);
void DrawPlayOverlays(const Game& g);  // flashes, messages, crosshair
void DrawTitleScreen(const Assets& A, float t);
void DrawEndScreen(const Game& g);
