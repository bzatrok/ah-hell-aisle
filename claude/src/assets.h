#pragma once
#include "raylib.h"
#include "map.h"

// Every texture in ../assets/, loaded once. A single global instead of threading a
// handle through every draw call: this game has exactly one of everything.
struct Assets {
    Texture2D wall[kTileKindCount];   // indexed by Tile; the Empty slot is unused
    Texture2D floorTex;
    Texture2D ceilingTex;
    Texture2D enemy[3];               // indexed by EnemyKind
    Texture2D pickup[9];              // indexed by PickupKind
    Texture2D weapon[4];              // indexed by WeaponId
    Texture2D soupCan;
    Texture2D rocket;
    Texture2D hudPanel;
    Texture2D hudFace;
};

extern Assets gAssets;

void AssetsLoad();
void AssetsUnload();
