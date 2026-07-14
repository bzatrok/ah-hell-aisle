#pragma once
#include "world.h"

// Three states, as SPEC §9 asks for: a title card, the shop, and whichever way it
// ended. There is one level and it is always the same one, so a restart is just a
// fresh World.
enum class GameState { Title, Playing, Dead, Escaped };

struct Game {
    GameState state = GameState::Title;
    World world;
};

void GameInit(Game& g);
void GameUpdate(Game& g, float dt);
void GameDraw(Game& g, float dt);
