#pragma once
#include "assets.h"
#include "game.h"

// Draws the 3D view: extruded tile walls, floor/ceiling, doors, billboarded
// enemies/pickups/projectiles and tracers. Everything 2D (weapon, HUD,
// screens) lives in hud.h.
void DrawWorld(const Game& g, const Assets& A);
