#include <raylib.h>
#include <raymath.h>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <cstring>
#include <cstdlib>

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768

extern float deltaTimeGlobal;

static float zBuffer[SCREEN_WIDTH];

#include "Game.h"
#include "Level.h"

void Game::init() {
    state = GameState::TITLE;
    
    player.pos = {5.5f, 2.0f, 5.5f};
    player.dir = {1.0f, 0.0f, 0.0f};
    player.rightDir = {0.0f, 0.0f, 1.0f};
    player.rotZ = 60.0f * (float)PI / 180.0f;
    
    player.moveSpeed = 3.0f;
    player.turnSpeed = 2.5f;
    
    player.health = 100;
    player.maxHealth = 100;
    player.armor = 0;
    player.maxArmor = 100;
    player.ammo = 20;
    player.weaponType = WeaponType::STOKBROOD;
    player.hasKeycard = false;
    
    weapon.offset = {0.0f, 0.0f, 0.0f};
    weapon.firing = false;
    weapon.fireTime = 0.0f;
    weapon.animTimer = 0.0f;
    
    initLevel();
    
    char cwd[PATH_MAX + 1];
    if (getcwd(cwd, sizeof(cwd)) == nullptr) {
        cwd[0] = '\0';
    }
    
    char assetsPath[256];
    
    const char* base = "/Users/bzatrok/play/random_stuff/assets";
    strcpy(assetsPath, base);
    
    texMan.loadAll(assetsPath);
}

void Game::update(float deltaTime) {
    deltaTimeGlobal = deltaTime;
    
    if (state == GameState::TITLE) {
        if (IsKeyDown(KEY_SPACE)) {
            reset();
        }
    } else if (state == GameState::PLAYING) {
        handleInput(deltaTime);
        
        weapon.update(deltaTime, player.rotZ, player.pos.x, player.pos.z);
        
        for (auto& enemy : enemies) {
            enemy.update(deltaTime, player.pos.x, player.pos.z, &player);
        }
        
        for (auto& pickup : pickups) {
            pickup.update(player);
        }
        
        if (doors.size() > 0 && !doors[0].closed && Vector3Distance(player.pos, {10.5f, 2.0f, 19.5f}) < 2.0f) {
            doors[0].open();
        }
    } else if (state == GameState::DEATH || state == GameState::VICTORY) {
        if (IsKeyDown(KEY_R)) {
            reset();
        }
    }
}

void Game::draw() {
    switch (state) {
        case GameState::TITLE:
            drawTitle();
            break;
        case GameState::PLAYING:
            drawGameWorld();
            break;
        case GameState::DEATH:
            drawDeathScreen();
            break;
        case GameState::VICTORY:
            drawVictoryScreen();
            break;
    }
}

void Game::handleInput(float dt) {
    if (IsKeyDown(KEY_LEFT)) {
        player.rotZ -= player.turnSpeed * dt;
    }
    if (IsKeyDown(KEY_RIGHT)) {
        player.rotZ += player.turnSpeed * dt;
    }
    
    Vector3 moveDir = {0.0f, 0.0f, 0.0f};
    if (IsKeyDown(KEY_W)) {
        moveDir.x += std::cos(player.rotZ);
        moveDir.z += std::sin(player.rotZ);
    }
    if (IsKeyDown(KEY_S)) {
        moveDir.x -= std::cos(player.rotZ);
        moveDir.z -= std::sin(player.rotZ);
    }
    if (IsKeyDown(KEY_A)) {
        moveDir.x -= std::sin(player.rotZ);
        moveDir.z += std::cos(player.rotZ);
    }
    if (IsKeyDown(KEY_D)) {
        moveDir.x += std::sin(player.rotZ);
        moveDir.z -= std::cos(player.rotZ);
    }
    
    float len = Vector3Length(moveDir);
    if (len > 0.0f) {
        moveDir = Vector3Scale(moveDir, dt * player.moveSpeed / len);
        
        Vector3 newPos = Vector3Add(player.pos, moveDir);
        int tileX = (int)newPos.x;
        int tileZ = (int)newPos.z;
        
        if (!isSolidWall(tileX, tileZ)) {
            player.pos = newPos;
        }
    }
    
    if (IsKeyDown(KEY_SPACE)) {
        weapon.fire(&player);
    }
}

void Game::reset() {
    state = GameState::PLAYING;
    
    player.pos = {5.5f, 2.0f, 5.5f};
    player.dir = {1.0f, 0.0f, 0.0f};
    player.rightDir = {0.0f, 0.0f, 1.0f};
    player.rotZ = 60.0f * (float)PI / 180.0f;
    
    player.health = 100;
    player.maxHealth = 100;
    player.armor = 0;
    player.maxArmor = 100;
    player.ammo = 20;
    player.weaponType = WeaponType::STOKBROOD;
    player.hasKeycard = false;
    
    weapon.firing = false;
    weapon.fireTime = 0.0f;
    weapon.animTimer = 0.0f;
    
    initLevel();
}

RayResult Game::castRay(float ox, float oy, float dirX, float dirY) {
    RayResult result = {false, 0.0f, 0, 0, 0};
    
    float dist = 0.0f;
    float stepSize = 0.1f;
    
    while (dist < 20.0f) {
        dist += stepSize;
        int tileX = (int)(ox + dirX * dist);
        int tileZ = (int)(oy + dirY * dist);
        
        if (tileZ < 0 || tileZ >= MAP_HEIGHT || tileX < 0 || tileX >= MAP_WIDTH) {
            result.hit = true;
            result.distance = dist;
            break;
        }
        
        if (isSolidWall(tileX, tileZ)) {
            result.hit = true;
            result.distance = dist;
            result.wallX = tileX;
            result.wallZ = tileZ;
            
            float exactDistX = (tileX + (dirX > 0 ? 1 : 0)) - ox;
            float exactDistY = (tileZ + (dirY > 0 ? 1 : 0)) - oy;
            result.side = (std::abs(exactDistX / dirX) < std::abs(exactDistY / dirY)) ? 0 : 1;
            break;
        }
    }
    
    return result;
}

void Game::drawTitle() {
    float titleWidth = MeasureText("AH: HELL Aisle", 80);
    DrawText("AH: Hell Aisle", (SCREEN_WIDTH - titleWidth) / 2, SCREEN_HEIGHT / 3, 80, BLACK);
    
    float subWidth = MeasureText("Press SPACE to start", 30);
    DrawText("Press SPACE to start", (SCREEN_WIDTH - subWidth) / 2, SCREEN_HEIGHT * 2 / 3, 30, DARKGRAY);
}

void Game::drawGameWorld() {
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        float rayAngle = player.rotZ + (x - SCREEN_WIDTH / 2) * 0.5f * (float)PI / 180.0f;
        float dirX = std::cos(rayAngle);
        float dirY = std::sin(rayAngle);
        
        RayResult result = castRay(player.pos.x, player.pos.z, dirX, dirY);
        zBuffer[x] = result.distance;
        
        if (result.hit) {
            float distance = result.distance;
            float correctedDist = distance * std::cos((x - SCREEN_WIDTH / 2) * 0.5f * (float)PI / 180.0f);
            
            int wallHeight = (int)(SCREEN_HEIGHT / correctedDist);
            int wallTop = -wallHeight / 2 + SCREEN_HEIGHT / 2;
            
            unsigned short tileType = levelMap[result.wallZ * MAP_WIDTH + result.wallX];
            Color wallColor = WHITE;
            
            if (result.side == 1) {
                wallColor = Fade(wallColor, 0.7f);
            }
            
            switch (tileType) {
                case TILE_WALL_SHELF_FULL:
                    DrawRectangle(x, wallTop, 1, wallHeight, LIGHTGRAY);
                    break;
                case TILE_WALL_FREEZER:
                    DrawRectangle(x, wallTop, 1, wallHeight, GRAY);
                    break;
                case TILE_WALL_CHECKOUT:
                    DrawRectangle(x, wallTop, 1, wallHeight, GREEN);
                    break;
                case TILE_WALL_MAGAZIJN:
                    DrawRectangle(x, wallTop, 1, wallHeight, RED);
                    break;
                default:
                    DrawRectangle(x, wallTop, 1, wallHeight, wallColor);
                    break;
            }
        }
    }
    
    DrawRectangle(0, SCREEN_HEIGHT / 2, SCREEN_WIDTH, SCREEN_HEIGHT / 2, DARKGREEN);
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT / 2, GRAY);
    
    float weaponX = SCREEN_WIDTH / 2 + std::cos(weapon.animTimer * 10) * 20;
    float weaponY = SCREEN_HEIGHT - 150 + std::sin(weapon.animTimer * 10) * 20;
    
    if (player.weaponType == WeaponType::STOKBROOD) {
        DrawTexturePro(texMan.weaponSprites[0], 
            {0, 0, (float)texMan.weaponSprites[0].width, (float)texMan.weaponSprites[0].height},
            {(float)(weaponX - 64), (float)weaponY}, {0, 0}, 128.0f, WHITE);
    } else {
        DrawTexturePro(texMan.weaponSprites[1], 
            {0, 0, (float)texMan.weaponSprites[1].width, (float)texMan.weaponSprites[1].height},
            {(float)(weaponX - 64), (float)weaponY}, {0, 0}, 128.0f, WHITE);
    }
    
    DrawTexture(texMan.hudFace, 32, SCREEN_HEIGHT - 96, WHITE);
    DrawText(TextFormat("HP: %d", player.health), 120, SCREEN_HEIGHT - 80, 20, BLACK);
    DrawText(TextFormat("AMMO: %d", player.ammo), 120, SCREEN_HEIGHT - 50, 20, BLACK);
    
    for (const auto& enemy : enemies) {
        if (!enemy.alive) continue;
        
        Color spriteColor = WHITE;
        switch (enemy.type) {
            case EnemyType::WINKELWAGEN:
                DrawTexture(texMan.enemySprites[0], 
                    (int)(SCREEN_WIDTH / 2 + (enemy.pos.x - player.pos.x) * 10), 
                    SCREEN_HEIGHT / 2, spriteColor);
                break;
            case EnemyType::VAKKENVULLER:
                DrawTexture(texMan.enemySprites[1], 
                    (int)(SCREEN_WIDTH / 2 + (enemy.pos.x - player.pos.x) * 10), 
                    SCREEN_HEIGHT / 2, spriteColor);
                break;
            case EnemyType::ZELFSCANNER:
                DrawTexture(texMan.enemySprites[2], 
                    (int)(SCREEN_WIDTH / 2 + (enemy.pos.x - player.pos.x) * 10), 
                    SCREEN_HEIGHT / 2, spriteColor);
                break;
        }
    }
}

void Game::drawDeathScreen() {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.7f));
    float width = MeasureText("YOU DIED", 100);
    DrawText("YOU DIED", (SCREEN_WIDTH - width) / 2, SCREEN_HEIGHT / 3, 100, WHITE);
    
    float subWidth = MeasureText("Press R to restart", 40);
    DrawText("Press R to restart", (SCREEN_WIDTH - subWidth) / 2, SCREEN_HEIGHT * 2 / 3, 40, LIGHTGRAY);
}

void Game::drawVictoryScreen() {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, DARKGREEN);
    float width = MeasureText("ESCAPED!", 100);
    DrawText("ESCAPED!", (SCREEN_WIDTH - width) / 2, SCREEN_HEIGHT / 3, 100, WHITE);
    
    float subWidth = MeasureText("Press R to play again", 40);
    DrawText("Press R to play again", (SCREEN_WIDTH - subWidth) / 2, SCREEN_HEIGHT * 2 / 3, 40, LIGHTGRAY);
}
