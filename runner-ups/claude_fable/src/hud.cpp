#include "hud.h"

#include <cmath>
#include <cstdio>

static constexpr int W = cfg::ScreenW;
static constexpr int H = cfg::ScreenH;

static void DrawTextCentredX(const char* text, int cx, int y, int size, Color c) {
  DrawText(text, cx - MeasureText(text, size) / 2, y, size, c);
}

// ---------------------------------------------------------------------------
// First-person weapon: 192x144 frames, art anchored bottom-centre-right.
// ---------------------------------------------------------------------------
void DrawWeapon(const Game& g, const Assets& A) {
  const Player& p = g.pl;
  Texture2D tex = (p.weapon == 1) ? A.weaponStok : A.weaponPist;

  int frame = 0;
  if (p.attackAnim > 0) {
    float elapsed = p.attackDur - p.attackAnim;
    if (p.weapon == 1) frame = elapsed < 0.12f ? 1 : (elapsed < 0.28f ? 2 : 0);
    else               frame = elapsed < 0.07f ? 1 : (elapsed < 0.14f ? 2 : 0);
  }

  float bobX = sinf(p.bobT) * 13.0f * p.bobAmount;
  float bobY = fabsf(cosf(p.bobT)) * 10.0f * p.bobAmount;
  const float scale = 3.0f;
  const float dw = 192 * scale, dh = 144 * scale;
  Rectangle src{ frame * 192.0f, 0, 192, 144 };
  Rectangle dst{ (W - dw) / 2 + 30 + bobX, H - cfg::HudH - dh + 26 + bobY, dw, dh };

  // dim the weapon with the room so it doesn't glow in the freezer
  Zone zn = g.map.zoneAt((int)floorf(p.pos.x), (int)floorf(p.pos.y));
  Color tint = WHITE;
  if (zn == Zone::Freezer) tint = Color{ 150, 185, 235, 255 };
  else if (zn == Zone::Magazijn) tint = Color{ 235, 205, 170, 255 };
  DrawTexturePro(tex, src, dst, Vector2{ 0, 0 }, 0, tint);
}

// ---------------------------------------------------------------------------
// Status bar (SPEC.md §9): panel with four recessed cells + the face.
// Cell x-ranges measured from hud_panel.png (320x48).
// ---------------------------------------------------------------------------
void DrawStatusBar(const Game& g, const Assets& A) {
  const Player& p = g.pl;
  const float sx = (float)W / 320.0f;  // 4.0
  const float sy = (float)cfg::HudH / 48.0f;  // 3.0
  const int top = H - cfg::HudH;
  DrawTexturePro(A.hudPanel, Rectangle{ 0, 0, 320, 48 },
                 Rectangle{ 0, (float)top, (float)W, (float)cfg::HudH },
                 Vector2{ 0, 0 }, 0, WHITE);

  struct Cell { float cx; const char* label; };
  const Cell cells[4] = {
    { 40.5f, "HEALTH" }, { 119.5f, "AMMO" }, { 199.5f, "ARMOUR" }, { 280.0f, "KEY" },
  };
  const char* values[4];
  static char v0[16], v1[16], v2[16];
  snprintf(v0, sizeof(v0), "%d%%", (int)p.hp);
  snprintf(v1, sizeof(v1), "%d", p.ammo);
  snprintf(v2, sizeof(v2), "%d%%", (int)p.armour);
  values[0] = v0; values[1] = v1; values[2] = v2; values[3] = p.hasKeycard ? nullptr : "-";

  for (int i = 0; i < 4; i++) {
    int cx = (int)(cells[i].cx * sx);
    DrawTextCentredX(cells[i].label, cx, top + (int)(15 * sy), 14, Color{ 140, 150, 160, 255 });
    if (values[i]) {
      Color c = RAYWHITE;
      if (i == 0 && p.hp < 50) c = Color{ 255, 120, 90, 255 };
      DrawTextCentredX(values[i], cx, top + (int)(24 * sy), 34, c);
    } else {
      // keycard icon once collected
      Texture2D key = A.pickups[(int)PickupType::Keycard];
      float ks = 1.9f;
      DrawTexturePro(key, Rectangle{ 0, 0, 32, 32 },
                     Rectangle{ cx - 16 * ks, top + 24 * sy - 6, 32 * ks, 32 * ks },
                     Vector2{ 0, 0 }, 0, WHITE);
    }
  }

  // the face, Doom-style, centred over the panel seam
  int face = p.hp <= 0 ? 2 : (p.hp < 50 ? 1 : 0);
  const float fs = 2.1f;
  DrawTexturePro(A.hudFace, Rectangle{ face * 48.0f, 0, 48, 56 },
                 Rectangle{ W / 2 - 24 * fs, H - 56 * fs - 10, 48 * fs, 56 * fs },
                 Vector2{ 0, 0 }, 0, WHITE);
}

// ---------------------------------------------------------------------------
void DrawPlayOverlays(const Game& g) {
  const Player& p = g.pl;
  if (p.hurtFlash > 0)
    DrawRectangle(0, 0, W, H, Fade(Color{ 200, 20, 20, 255 }, fminf(p.hurtFlash, 0.45f)));
  if (p.pickupFlash > 0)
    DrawRectangle(0, 0, W, H, Fade(RAYWHITE, p.pickupFlash * 0.55f));

  DrawCircle(W / 2, H / 2, 2.5f, Fade(RAYWHITE, 0.65f));

  if (g.msgT > 0 && !g.msg.empty()) {
    float a = g.msgT < 0.5f ? g.msgT / 0.5f : 1.0f;
    DrawTextCentredX(g.msg.c_str(), W / 2, H - cfg::HudH - 52, 26,
                     Fade(Color{ 255, 220, 90, 255 }, a));
  }
}

// ---------------------------------------------------------------------------
static const Color kAHBlue{ 0, 173, 239, 255 };

void DrawTitleScreen(const Assets& A, float t) {
  ClearBackground(Color{ 8, 10, 14, 255 });
  DrawRectangle(0, 130, W, 6, kAHBlue);
  DrawRectangle(0, 296, W, 6, kAHBlue);

  DrawTextCentredX("AH: HELL AISLE", W / 2, 170, 84, RAYWHITE);
  DrawTextCentredX("de nachtdienst is nog niet voorbij", W / 2, 258, 24,
                   Color{ 160, 170, 180, 255 });

  const char* lines[] = {
    "WASD lopen   .   muis of pijltjes draaien   .   LMB / CTRL vuren",
    "1 stokbrood   .   2 prijspistool   .   E deuren   .   ESC stoppen",
    "",
    "Vind de bedrijfsleiderspas. Open het magazijn. Neem het laaddok.",
  };
  for (int i = 0; i < 4; i++)
    DrawTextCentredX(lines[i], W / 2, 350 + i * 34, 20, Color{ 150, 160, 170, 255 });

  // the shift roster, walk frame 0
  for (int i = 0; i < 3; i++) {
    float s = 2.6f;
    DrawTexturePro(A.enemy[i], Rectangle{ 0, 0, 64, 64 },
                   Rectangle{ W / 2 - 290 + i * 220.0f, 500, 64 * s, 64 * s },
                   Vector2{ 0, 0 }, 0, WHITE);
  }

  if (fmodf(t, 1.0f) < 0.65f)
    DrawTextCentredX("DRUK OP EEN TOETS", W / 2, 552, 30, kAHBlue);
}

void DrawEndScreen(const Game& g) {
  bool won = g.state == GameState::Won;
  DrawRectangle(0, 0, W, H, Fade(won ? Color{ 0, 20, 10, 255 } : Color{ 30, 0, 0, 255 }, 0.72f));

  DrawTextCentredX(won ? "ONTSNAPT" : "GESLOTEN", W / 2, 200, 96,
                   won ? Color{ 120, 230, 140, 255 } : Color{ 235, 60, 50, 255 });
  DrawTextCentredX(won ? "via het laaddok de nacht in" : "deze vakkenvuller vult niets meer bij",
                   W / 2, 300, 24, Color{ 180, 180, 180, 255 });

  char stats[96];
  int mins = (int)(g.time / 60), secs = (int)g.time % 60;
  snprintf(stats, sizeof(stats), "TIJD %02d:%02d      KILLS %d / %d",
           mins, secs, g.kills, g.totalEnemies);
  DrawTextCentredX(stats, W / 2, 380, 32, RAYWHITE);

  DrawTextCentredX("R = opnieuw      ESC = stoppen", W / 2, 460, 24, kAHBlue);
}
