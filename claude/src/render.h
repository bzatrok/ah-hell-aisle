#pragma once
#include "raylib.h"

struct Map;
struct World;

// Builds the static geometry for the one level, and the two shaders. Call once, after
// the window and the textures exist.
void RenderInit(const Map& map);
void RenderShutdown();

Camera3D SceneCamera(const World& w);

void RenderScene(const World& w, float dt);   // the store, in 3D
void RenderWeapon(const World& w);            // the thing in your hands, in 2D
