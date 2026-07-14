#include "TextureManager.h"
#include <cstring>
#include <cstdio>

bool TextureManager::loadAll(const char* assetsPath) {
    char path[256];
    
    sprintf(path, "%s/wall_shelf_full.png", assetsPath);
    walls[0] = LoadTexture(path);
    sprintf(path, "%s/wall_shelf_empty.png", assetsPath);
    walls[1] = LoadTexture(path);
    sprintf(path, "%s/wall_freezer.png", assetsPath);
    walls[2] = LoadTexture(path);
    sprintf(path, "%s/wall_plain.png", assetsPath);
    walls[3] = LoadTexture(path);
    sprintf(path, "%s/wall_checkout.png", assetsPath);
    walls[4] = LoadTexture(path);
    sprintf(path, "%s/wall_magazijn.png", assetsPath);
    walls[5] = LoadTexture(path);
    
    sprintf(path, "%s/door_keycard.png", assetsPath);
    doors[0] = LoadTexture(path);
    sprintf(path, "%s/door_exit.png", assetsPath);
    doors[1] = LoadTexture(path);
    
    sprintf(path, "%s/floor.png", assetsPath);
    floor = LoadTexture(path);
    sprintf(path, "%s/ceiling.png", assetsPath);
    ceiling = LoadTexture(path);
    
    sprintf(path, "%s/enemy_winkelwagen.png", assetsPath);
    enemySprites[0] = LoadTexture(path);
    sprintf(path, "%s/enemy_vakkenvuller.png", assetsPath);
    enemySprites[1] = LoadTexture(path);
    sprintf(path, "%s/enemy_zelfscanner.png", assetsPath);
    enemySprites[2] = LoadTexture(path);
    
    sprintf(path, "%s/weapon_stokbrood.png", assetsPath);
    weaponSprites[0] = LoadTexture(path);
    sprintf(path, "%s/weapon_prijspistool.png", assetsPath);
    weaponSprites[1] = LoadTexture(path);
    
    sprintf(path, "%s/pickup_appelflap.png", assetsPath);
    pickupSprites[0] = LoadTexture(path);
    sprintf(path, "%s/pickup_rookworst.png", assetsPath);
    pickupSprites[1] = LoadTexture(path);
    sprintf(path, "%s/pickup_labels.png", assetsPath);
    pickupSprites[2] = LoadTexture(path);
    sprintf(path, "%s/pickup_bonuskaart.png", assetsPath);
    pickupSprites[3] = LoadTexture(path);
    sprintf(path, "%s/pickup_keycard.png", assetsPath);
    pickupSprites[4] = LoadTexture(path);
    
    sprintf(path, "%s/proj_soepblik.png", assetsPath);
    projSoupCan = LoadTexture(path);
    
    sprintf(path, "%s/hud_face.png", assetsPath);
    hudFace = LoadTexture(path);
    sprintf(path, "%s/hud_panel.png", assetsPath);
    hudPanel = LoadTexture(path);
    
    return true;
}

void TextureManager::unload() {
    for (int i = 0; i < 6; i++) UnloadTexture(walls[i]);
    for (int i = 0; i < 2; i++) UnloadTexture(doors[i]);
    UnloadTexture(floor);
    UnloadTexture(ceiling);
    for (int i = 0; i < 3; i++) UnloadTexture(enemySprites[i]);
    for (int i = 0; i < 2; i++) UnloadTexture(weaponSprites[i]);
    for (int i = 0; i < 5; i++) UnloadTexture(pickupSprites[i]);
    UnloadTexture(projSoupCan);
    UnloadTexture(hudFace);
    UnloadTexture(hudPanel);
}
