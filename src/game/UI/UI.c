#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "helpers.h"
#include "game/UI/UI.h"
#include "game/player/player.h"
#include "game/rendering/textures.h"
#include "game/map/map.h"
#include "game/enemies/enemies.h"
#include "game/entities/entities.h"
#include "game/entities/projectiles.h"

#define SCREEN_SHAKE_TIME 0.5f // Seconds
#define SCREEN_SHAKE_MAGNITUDE 7

#define MINIMAP_X 170
#define MINIMAP_Y 30

#define MINIMAP_SIZE 13 // Width/height in terms of cells
#define CELL_DRAW_SIZE 4 // Pixel size of each cell
#define HALF_CELL_DRAW_SIZE CELL_DRAW_SIZE / 2
#define ENTITY_DIST_SCALE_FACTOR CELL_DRAW_SIZE / CELL_SIZE
#define MINIMAP_PIXEL_SIZE MINIMAP_SIZE * CELL_DRAW_SIZE
#define ENTITY_CHECK_DISTANCE powf((MINIMAP_SIZE / 2) * CELL_SIZE, 2.0f)
#define MINIMAP_CENTER_X MINIMAP_SIZE / 2 * CELL_DRAW_SIZE - 1
#define MINIMAP_CENTER_Y MINIMAP_SIZE / 2 * CELL_DRAW_SIZE - 1

float screenShakeTime;

void initUI(void) {
    screenShakeTime = 0;
    pd->display->setOffset(0, 0);
    pd->graphics->setBackgroundColor(kColorBlack);
}

void drawPlayerHearts(void) {
    int fullHeartCount = playerHealth / 2;
    int hasHalfHeart = playerHealth % 2;
    int heartCount = (playerMaxHealth + 1) / 2;
    int drawY = 2;
    for (int i = 0; i < heartCount; i++) {
        int drawX = 2 + i * 16;

        if (i < fullHeartCount) {
            pd->graphics->drawBitmap(fullHeartBitmap, drawX, drawY, kBitmapUnflipped);
        } else if (i == fullHeartCount && hasHalfHeart) {
            pd->graphics->drawBitmap(halfHeartBitmap, drawX, drawY, kBitmapUnflipped);
        } else {
            pd->graphics->drawBitmap(emptyHeartBitmap, drawX, drawY, kBitmapUnflipped);
        }
    }
}

void drawEntityToMinimap(float entityX, float entityY, int type, float playerAngleCos, float playerAngleSin) {
    float distSquaredToPlayer = distanceSquared(playerX, playerY, entityX, entityY);
    if (distSquaredToPlayer <= ENTITY_CHECK_DISTANCE) {
        float entityVectorX = (playerX - entityX);
        float entityVectorY = (playerY - entityY);
        // Multiply by rotation matrix to rotate
        float rotatedEntityX = entityVectorX * playerAngleCos - entityVectorY * playerAngleSin;
        float rotatedEntityY = entityVectorX * playerAngleSin + entityVectorY * playerAngleCos;

        float entityMinimapX = MINIMAP_CENTER_X + rotatedEntityX * ENTITY_DIST_SCALE_FACTOR;
        float entityMinimapY = MINIMAP_CENTER_Y + rotatedEntityY * ENTITY_DIST_SCALE_FACTOR;

        if (type == ENEMY) {
            pd->graphics->drawBitmap(minimapEnemyBitmap, entityMinimapX, entityMinimapY, kBitmapUnflipped);
        } else if (type == PROJECTILE) {
            pd->graphics->drawBitmap(minimapProjectileBitmap, entityMinimapX, entityMinimapY, kBitmapUnflipped);
        }
    }
}

void drawMinimap(void) {
    // Angle that matches orientation of walls
    float rotatedPlayerAngle = 2*PI - playerAngle - P2;

    // == Base minimap layer (walls) ==
    LCDBitmap *minimapBitmap = pd->graphics->newBitmap(MINIMAP_PIXEL_SIZE, MINIMAP_PIXEL_SIZE, kColorBlack);

    pd->graphics->pushContext(minimapBitmap);
    int playerMapX = playerX / CELL_SIZE;
    int playerMapY = playerY / CELL_SIZE;
    int playerOffsetX = roundf((playerX - playerMapX * CELL_SIZE) / CELL_DRAW_SIZE) + CELL_DRAW_SIZE / 2;
    int playerOffsetY = roundf((playerY - playerMapY * CELL_SIZE) / CELL_DRAW_SIZE) + CELL_DRAW_SIZE / 2;
    
    // Draw walls
    for (int mapY = 0; mapY < MINIMAP_SIZE + 1; mapY++) {
        int adjustedMapY = playerMapY - (MINIMAP_SIZE + 1) / 2 + mapY;
        if (adjustedMapY < 0 || adjustedMapY >= MAP_HEIGHT) {
            continue;
        }
        for (int mapX = 0; mapX < MINIMAP_SIZE + 1; mapX++) {
            int adjustedMapX = playerMapX - (MINIMAP_SIZE + 1) / 2 + mapX;
            if (adjustedMapX < 0 || adjustedMapX >= MAP_WIDTH) {
                continue;
            }
            int drawX = mapX * CELL_DRAW_SIZE - playerOffsetX;
            int drawY = mapY * CELL_DRAW_SIZE - playerOffsetY;
            if (isBlocked(adjustedMapX, adjustedMapY)) {
                pd->graphics->drawBitmap(minimapWallBitmap, drawX, drawY, kBitmapUnflipped);
            }
        }
    }

    pd->graphics->popContext();

    // Set circle mask
    LCDBitmap *minimapMask = pd->graphics->newBitmap(MINIMAP_PIXEL_SIZE, MINIMAP_PIXEL_SIZE, kColorBlack);
    pd->graphics->pushContext(minimapMask);
    pd->graphics->fillEllipse(1, 1, MINIMAP_PIXEL_SIZE - 2, MINIMAP_PIXEL_SIZE - 2, 0, 0, kColorWhite);
    pd->graphics->popContext();
    pd->graphics->setBitmapMask(minimapBitmap, minimapMask);
    
    // Draw base layer
    pd->graphics->drawRotatedBitmap(minimapBitmap, MINIMAP_X, MINIMAP_Y, radToDeg(rotatedPlayerAngle), 0.5f, 0.5f, 1.0f, 1.0f);
    pd->graphics->freeBitmap(minimapBitmap);
    pd->graphics->freeBitmap(minimapMask);
    // == End of base layer ==

    // == Overlay layer (player + entities) ==
    LCDBitmap *minimapOverlayBitmap = pd->graphics->newBitmap(MINIMAP_PIXEL_SIZE, MINIMAP_PIXEL_SIZE, kColorClear);
    pd->graphics->pushContext(minimapOverlayBitmap);

    // Draw player
    pd->graphics->drawBitmap(minimapPlayerBitmap, MINIMAP_CENTER_X, MINIMAP_CENTER_Y, kBitmapUnflipped);

    // Draw entities
    float playerAngleCos = cosf(rotatedPlayerAngle + PI);
    float playerAngleSin = sinf(rotatedPlayerAngle + PI);
    // Draw enemies
    for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
        Enemy curEnemy = enemies[i];
        if (!curEnemy.active || curEnemy.state == ENEMY_STATE_DEAD) {
            continue;
        }

        drawEntityToMinimap(curEnemy.x, curEnemy.y, ENEMY, playerAngleCos, playerAngleSin);
    }
    // Draw projectiles
    for (int i = 0; i < MAX_PROJECTILE_COUNT; i++) {
        Projectile curProjectile = projectiles[i];
        if (!curProjectile.active) {
            continue;
        }

        drawEntityToMinimap(curProjectile.x, curProjectile.y, PROJECTILE, playerAngleCos, playerAngleSin);
    }

    // Draw border
    pd->graphics->drawBitmap(minimapBorderBitmap, 0, 0, kBitmapUnflipped);

    // Draw overlay layer
    pd->graphics->popContext();
    pd->graphics->drawRotatedBitmap(minimapOverlayBitmap, MINIMAP_X, MINIMAP_Y, 0, 0.5f, 0.5f, 1.0f, 1.0f);
    pd->graphics->freeBitmap(minimapOverlayBitmap);
    // == End of overlay layer ==
}

void drawUI(float deltaTime) {
    if (screenShakeTime > 0) {
        screenShakeTime -= deltaTime;
        if (screenShakeTime <= 0) {
            screenShakeTime = 0;
            pd->display->setOffset(0, 0);
        } else {
            int shakeMagnitude = screenShakeTime * SCREEN_SHAKE_MAGNITUDE;
            int shakeX = randInt(-shakeMagnitude, shakeMagnitude);
            int shakeY = randInt(-shakeMagnitude, shakeMagnitude);
            pd->display->setOffset(shakeX, shakeY);
        }
    }

    drawPlayerHearts();
    drawMinimap();
    drawWeapon(deltaTime);
}

void screenShake(void) {
    screenShakeTime = SCREEN_SHAKE_TIME;
}