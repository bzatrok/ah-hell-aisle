#include "hud.h"

#include <cmath>

#include "assets.h"
#include "config.h"
#include "raylib.h"
#include "raymath.h"
#include "world.h"

namespace {

const Color kAhBlue = {0, 160, 227, 255};
const Color kBlood = {190, 32, 26, 255};
const Color kInk = {224, 228, 232, 255};

// Where the four recessed cells of hud_panel.png land once it is stretched across the
// bottom of the screen. The panel art is 320 wide; these are its cell centres, scaled.
const float kCellX[4] = {172.0f, 476.0f, 804.0f, 1108.0f};
const char* const kCellLabel[4] = {"HEALTH", "AMMO", "ARMOUR", "KEY"};

void TextCentred(const char* text, float cx, float y, int size, Color col) {
    DrawText(text, (int)(cx - MeasureText(text, size) * 0.5f), (int)y, size, col);
}

void StatLine(const World& w, float y) {
    const int mins = (int)w.elapsed / 60;
    const int secs = (int)w.elapsed % 60;
    TextCentred(TextFormat("TIJD  %d:%02d", mins, secs), kScreenW * 0.5f, y, 28, kInk);
    TextCentred(TextFormat("OPGERUIMD  %d / %d", w.kills, w.totalEnemies),
                kScreenW * 0.5f, y + 38.0f, 28, kInk);
}

void Dim(unsigned char alpha) {
    DrawRectangle(0, 0, kScreenW, kScreenH, Color{0, 0, 0, alpha});
}

}  // namespace

void HudDraw(const World& w) {
    const Player& p = w.player;
    const float top = kScreenH - kHudH;

    // Damage flash and the freezer's chill, over the world but under the furniture.
    if (p.hurtFlash > 0.0f) {
        DrawRectangle(0, 0, kScreenW, kScreenH,
                      Color{170, 20, 15, (unsigned char)(p.hurtFlash * 72.0f)});
    }

    // Crosshair sits on the projection centre, which is where the labels actually go.
    DrawRectangle(kScreenW / 2 - 1, kScreenH / 2 - 6, 2, 12, Color{255, 255, 255, 70});
    DrawRectangle(kScreenW / 2 - 6, kScreenH / 2 - 1, 12, 2, Color{255, 255, 255, 70});

    DrawTexturePro(gAssets.hudPanel, {0, 0, 320, 48}, {0, top, (float)kScreenW, kHudH},
                   {0, 0}, 0.0f, WHITE);

    for (int i = 0; i < 4; i++) {
        TextCentred(kCellLabel[i], kCellX[i], top + 10.0f, 14, Color{120, 130, 140, 255});
    }

    const Color healthCol = (p.health < 35) ? kBlood : kInk;
    TextCentred(TextFormat("%d%%", p.health), kCellX[0], top + 32.0f, 36, healthCol);

    const Color ammoCol = (p.weapon == WeaponId::Prijspistool) ? kInk : Color{120, 130, 140, 255};
    TextCentred(TextFormat("%d", p.ammo), kCellX[1], top + 32.0f, 36, ammoCol);

    TextCentred(TextFormat("%d%%", p.armour), kCellX[2], top + 32.0f, 36,
                p.armour > 0 ? kAhBlue : Color{120, 130, 140, 255});

    // The pass shows as a ghost until you are actually carrying it.
    const Rectangle keySrc = {0, 0, 32, 32};
    const Rectangle keyDst = {kCellX[3] - 26.0f, top + 34.0f, 52.0f, 52.0f};
    DrawTexturePro(gAssets.pickup[(int)PickupKind::Keycard], keySrc, keyDst, {0, 0}, 0.0f,
                   p.hasKeycard ? WHITE : Color{255, 255, 255, 28});

    const int faceFrame = p.dead() ? 2 : (p.health < 50 ? 1 : 0);
    DrawTexturePro(gAssets.hudFace, {faceFrame * 48.0f, 0, 48, 56},
                   {kScreenW * 0.5f - 37.0f, top + 4.0f, 74.0f, 87.0f}, {0, 0}, 0.0f, WHITE);

    DrawText(p.weapon == WeaponId::Stokbrood ? "1  STOKBROOD" : "2  PRIJSPISTOOL", 18,
             (int)top - 26, 18, Color{200, 205, 210, 160});

    float y = 18.0f;
    for (const HudMessage& m : w.messages) {
        const unsigned char alpha = (unsigned char)(Clamp(m.life / 1.2f, 0.0f, 1.0f) * 235.0f);
        TextCentred(m.text.c_str(), kScreenW * 0.5f, y, 20, Color{235, 238, 242, alpha});
        y += 26.0f;
    }
}

void ScreenTitle() {
    ClearBackground(Color{8, 10, 14, 255});

    DrawRectangle(0, 250, kScreenW, 8, kAhBlue);
    TextCentred("AH: HELL AISLE", kScreenW * 0.5f, 150.0f, 86, kInk);
    TextCentred("een Albert Heijn, na sluitingstijd", kScreenW * 0.5f, 275.0f, 24, kAhBlue);

    TextCentred("02:14. Er kwam iets omhoog uit het putje onder de visafdeling.",
                kScreenW * 0.5f, 340.0f, 22, Color{170, 175, 180, 255});
    TextCentred("Vind de pas van de bedrijfsleider. Haal het magazijn. Kom eruit.",
                kScreenW * 0.5f, 372.0f, 22, Color{170, 175, 180, 255});

    TextCentred("WASD lopen    MUIS / PIJLTJES draaien    KLIK of CTRL slaan en schieten",
                kScreenW * 0.5f, 470.0f, 20, Color{120, 126, 132, 255});
    TextCentred("1 stokbrood    2 prijspistool    E deur    ESC stoppen",
                kScreenW * 0.5f, 500.0f, 20, Color{120, 126, 132, 255});

    const float pulse = 0.55f + 0.45f * sinf((float)GetTime() * 3.4f);
    TextCentred("DRUK OP EEN TOETS", kScreenW * 0.5f, 600.0f, 30,
                Color{255, 255, 255, (unsigned char)(pulse * 255.0f)});
}

void ScreenDead(const World& w) {
    Dim(180);
    DrawRectangle(0, 250, kScreenW, 6, kBlood);
    TextCentred("GESLOTEN", kScreenW * 0.5f, 150.0f, 96, kBlood);
    TextCentred("de nachtploeg is compleet", kScreenW * 0.5f, 275.0f, 24,
                Color{150, 60, 55, 255});

    StatLine(w, 380.0f);
    TextCentred("R  OPNIEUW          ESC  STOPPEN", kScreenW * 0.5f, 500.0f, 26,
                Color{200, 205, 210, 220});
}

void ScreenEscaped(const World& w) {
    Dim(170);
    DrawRectangle(0, 250, kScreenW, 6, kAhBlue);
    TextCentred("LAADPERRON", kScreenW * 0.5f, 150.0f, 90, kInk);
    TextCentred("je staat op de parkeerplaats. het is nog steeds donker.",
                kScreenW * 0.5f, 275.0f, 24, kAhBlue);

    StatLine(w, 380.0f);
    TextCentred("R  OPNIEUW          ESC  STOPPEN", kScreenW * 0.5f, 500.0f, 26,
                Color{200, 205, 210, 220});
}
