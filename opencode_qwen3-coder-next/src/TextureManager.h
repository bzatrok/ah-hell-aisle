#pragma once

#include <raylib.h>

struct TextureManager {
    Texture2D walls[6];
    Texture2D doors[2];
    Texture2D floor;
    Texture2D ceiling;
    
    Texture2D enemySprites[3];
    Texture2D weaponSprites[2];
    Texture2D pickupSprites[5];
    Texture2D projSoupCan;
    
    Texture2D hudFace;
    Texture2D hudPanel;
    
    bool loadAll(const char* assetsPath);
    void unload();
};
