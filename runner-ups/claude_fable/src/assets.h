#pragma once
#include "raylib.h"

#include "game.h"

// Textures loaded read-only from ../assets/ (see assets/MANIFEST.md) plus a
// handful of synthesised sound effects — no audio files are shipped.
struct Assets {
  Texture2D wallPlain{}, wallShelfFull{}, wallShelfEmpty{}, wallFreezer{},
      wallCheckout{}, wallMagazijn{}, doorKeycard{}, doorExit{}, floor{},
      ceiling{};
  Texture2D enemy[3]{};    // indexed by EnemyType
  Texture2D can{};
  Texture2D pickups[5]{};  // indexed by PickupType
  Texture2D weaponStok{}, weaponPist{};
  Texture2D hudPanel{}, hudFace{};

  Sound sShoot{}, sClick{}, sSwing{}, sEnemyHit{}, sEnemyDie{}, sHurt{},
      sPickup{}, sKeycard{}, sDoorOpen{}, sLocked{}, sScanner{}, sThrow{},
      sWin{}, sLose{};
  bool audio = false;
};

Assets LoadAssets();
void UnloadAssets(Assets& A);
void Sfx(const Assets& A, const Sound& s);
Texture2D WallTexture(const Assets& A, Tile t);
