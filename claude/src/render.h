#pragma once
#include "raylib.h"

struct Map;
struct World;

// Builds the shaders and the level's static geometry. RenderInit once, after the
// window and the textures exist; RenderRebuild on every level (re)load.
void RenderInit(const Map& map);
void RenderRebuild(const Map& map);
void RenderShutdown();

Camera3D SceneCamera(const World& w);

void RenderScene(const World& w, float dt);   // the store, in 3D
void RenderWeapon(const World& w);            // the thing in your hands, in 2D
