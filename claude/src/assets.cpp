#include "assets.h"

#include <string>

#include "enemy.h"
#include "pickup.h"
#include "player.h"

Assets gAssets;

namespace {

std::string gDir;

Texture2D Grab(const char* name) {
    const Texture2D tex = LoadTexture((gDir + name).c_str());
    if (tex.id == 0) {
        TraceLog(LOG_FATAL, "could not load %s%s", gDir.c_str(), name);
    }
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);   // it is pixel art; keep the pixels
    return tex;
}

}  // namespace

void AssetsLoad() {
    // The grader runs ./build/ah_hell_aisle from the folder root, so ../assets/ is the
    // path that matters; the others are courtesy for anyone launching from elsewhere.
    for (const char* candidate : {"../assets/", "assets/", "../../assets/"}) {
        if (DirectoryExists(candidate)) {
            gDir = candidate;
            break;
        }
    }
    if (gDir.empty()) {
        TraceLog(LOG_FATAL, "assets/ not found - run ./build/ah_hell_aisle from claude/");
    }

    gAssets.wall[(int)Tile::Plain] = Grab("wall_plain.png");
    gAssets.wall[(int)Tile::Checkout] = Grab("wall_checkout.png");
    gAssets.wall[(int)Tile::ShelfFull] = Grab("wall_shelf_full.png");
    gAssets.wall[(int)Tile::ShelfEmpty] = Grab("wall_shelf_empty.png");
    gAssets.wall[(int)Tile::Freezer] = Grab("wall_freezer.png");
    gAssets.wall[(int)Tile::Magazijn] = Grab("wall_magazijn.png");
    gAssets.wall[(int)Tile::DoorKeycard] = Grab("door_keycard.png");
    gAssets.wall[(int)Tile::DoorExit] = Grab("door_exit.png");

    gAssets.floorTex = Grab("floor.png");
    gAssets.ceilingTex = Grab("ceiling.png");

    gAssets.enemy[(int)EnemyKind::Winkelwagen] = Grab("enemy_winkelwagen.png");
    gAssets.enemy[(int)EnemyKind::Vakkenvuller] = Grab("enemy_vakkenvuller.png");
    gAssets.enemy[(int)EnemyKind::Zelfscanner] = Grab("enemy_zelfscanner.png");

    gAssets.pickup[(int)PickupKind::Appelflap] = Grab("pickup_appelflap.png");
    gAssets.pickup[(int)PickupKind::Rookworst] = Grab("pickup_rookworst.png");
    gAssets.pickup[(int)PickupKind::Labels] = Grab("pickup_labels.png");
    gAssets.pickup[(int)PickupKind::Bonuskaart] = Grab("pickup_bonuskaart.png");
    gAssets.pickup[(int)PickupKind::Keycard] = Grab("pickup_keycard.png");
    gAssets.pickup[(int)PickupKind::Flessen] = Grab("pickup_flessen.png");
    gAssets.pickup[(int)PickupKind::Vuurwerk] = Grab("pickup_vuurwerk.png");
    gAssets.pickup[(int)PickupKind::WeaponScatter] = Grab("pickup_statiegeldkanon.png");
    gAssets.pickup[(int)PickupKind::WeaponRocket] = Grab("pickup_vuurwerkpijl.png");

    gAssets.weapon[(int)WeaponId::Stokbrood] = Grab("weapon_stokbrood.png");
    gAssets.weapon[(int)WeaponId::Prijspistool] = Grab("weapon_prijspistool.png");
    gAssets.weapon[(int)WeaponId::Statiegeldkanon] = Grab("weapon_statiegeldkanon.png");
    gAssets.weapon[(int)WeaponId::Vuurwerkpijl] = Grab("weapon_vuurwerkpijl.png");

    gAssets.soupCan = Grab("proj_soepblik.png");
    gAssets.rocket = Grab("proj_vuurwerkpijl.png");
    gAssets.hudPanel = Grab("hud_panel.png");
    gAssets.hudFace = Grab("hud_face.png");
}

void AssetsUnload() {
    for (int i = 1; i < kTileKindCount; i++) UnloadTexture(gAssets.wall[i]);
    UnloadTexture(gAssets.floorTex);
    UnloadTexture(gAssets.ceilingTex);
    for (Texture2D& t : gAssets.enemy) UnloadTexture(t);
    for (Texture2D& t : gAssets.pickup) UnloadTexture(t);
    for (Texture2D& t : gAssets.weapon) UnloadTexture(t);
    UnloadTexture(gAssets.soupCan);
    UnloadTexture(gAssets.rocket);
    UnloadTexture(gAssets.hudPanel);
    UnloadTexture(gAssets.hudFace);
}
