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
#include "game/entities/interactables.h"
#include "game/settings/settings.h"

#define MAX_SCREEN_SHAKE_TIME 0.8f // Seconds
#define MAX_SCREEN_SHAKE_MAGNITUDE 10

#define INTERACTABLE_PROMPT_LABEL_X 100
#define INTERACTABLE_PROMPT_LABEL_Y 60
#define INTERACTABLE_BUTTON_PROMPT_X 100
#define INTERACTABLE_BUTTON_PROMPT_Y 80

#define MINIMAP_X 170
#define MINIMAP_Y 30

#define MINIMAP_SIZE 13 // Width/height in terms of cells
#define CELL_DRAW_SIZE 4 // Pixel size of each cell
#define HALF_CELL_DRAW_SIZE CELL_DRAW_SIZE / 2
#define ENTITY_DIST_SCALE_FACTOR CELL_DRAW_SIZE / CELL_SIZE
#define MINIMAP_PIXEL_SIZE MINIMAP_SIZE * CELL_DRAW_SIZE
#define MINIMAP_HALF_PIXEL_SIZE MINIMAP_PIXEL_SIZE / 2
#define ENTITY_CHECK_DISTANCE powf((MINIMAP_SIZE / 2 + 1) * CELL_SIZE, 2.0f)
#define MINIMAP_CENTER_X MINIMAP_SIZE / 2 * CELL_DRAW_SIZE - 1
#define MINIMAP_CENTER_Y MINIMAP_SIZE / 2 * CELL_DRAW_SIZE - 1

static float screenShakeTime;
static int shakeMagnitude;

void initUI(void) {
    screenShakeTime = 0;
    shakeMagnitude = 0;
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
            pd->graphics->drawBitmap(BITMAPS[BITMAP_FULL_HEART], drawX, drawY, kBitmapUnflipped);
        } else if (i == fullHeartCount && hasHalfHeart) {
            pd->graphics->drawBitmap(BITMAPS[BITMAP_HALF_HEART], drawX, drawY, kBitmapUnflipped);
        } else {
            pd->graphics->drawBitmap(BITMAPS[BITMAP_EMPTY_HEART], drawX, drawY, kBitmapUnflipped);
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
            pd->graphics->drawBitmap(BITMAPS[BITMAP_MINIMAP_ENEMY], entityMinimapX, entityMinimapY, kBitmapUnflipped);
        } else if (type == PROJECTILE) {
            pd->graphics->drawBitmap(BITMAPS[BITMAP_MINIMAP_PROJECTILE], entityMinimapX, entityMinimapY, kBitmapUnflipped);
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
    LCDBitmap *minimapWallBitmap = BITMAPS[BITMAP_MINIMAP_WALL];
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
    LCDBitmap *baseLayerMask = pd->graphics->newBitmap(MINIMAP_PIXEL_SIZE, MINIMAP_PIXEL_SIZE, kColorWhite);
    pd->graphics->pushContext(baseLayerMask);
    pd->graphics->drawBitmap(BITMAPS[BITMAP_MINIMAP_MASK], 0, 0, kBitmapUnflipped);
    pd->graphics->popContext();
    pd->graphics->setBitmapMask(minimapBitmap, baseLayerMask);
    
    // Draw base layer
    pd->graphics->drawRotatedBitmap(minimapBitmap, MINIMAP_X, MINIMAP_Y, radToDeg(rotatedPlayerAngle), 0.5f, 0.5f, 1.0f, 1.0f);
    pd->graphics->freeBitmap(minimapBitmap);
    pd->graphics->freeBitmap(baseLayerMask);
    // == End of base layer ==

    // == Overlay layer (player + entities) ==
    LCDBitmap *minimapOverlayBitmap = pd->graphics->newBitmap(MINIMAP_PIXEL_SIZE, MINIMAP_PIXEL_SIZE, kColorClear);
    pd->graphics->pushContext(minimapOverlayBitmap);

    // Draw player
    pd->graphics->drawBitmap(BITMAPS[BITMAP_MINIMAP_PLAYER], MINIMAP_CENTER_X, MINIMAP_CENTER_Y, kBitmapUnflipped);

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

    // Set circle mask
    LCDBitmap *overlayLayerMask = pd->graphics->getBitmapMask(minimapOverlayBitmap);
    pd->graphics->pushContext(overlayLayerMask);
    pd->graphics->drawBitmap(BITMAPS[BITMAP_MINIMAP_MASK], 0, 0, kBitmapUnflipped);
    pd->graphics->popContext();

    // Draw border
    pd->graphics->drawBitmap(BITMAPS[BITMAP_MINIMAP_BORDER], 0, 0, kBitmapUnflipped);

    // Draw overlay layer
    pd->graphics->popContext();
    pd->graphics->drawBitmap(minimapOverlayBitmap, MINIMAP_X - MINIMAP_HALF_PIXEL_SIZE, MINIMAP_Y - MINIMAP_HALF_PIXEL_SIZE, kBitmapUnflipped);
    pd->graphics->freeBitmap(minimapOverlayBitmap);
    // == End of overlay layer ==
}

void drawInteractablePrompt(void) {
    if (interactableIndex < 0 || interactableIndex >= MAX_INTERACTABLE_COUNT) {
        return;
    }

    Interactable curInteractable = INTERACTABLES[interactableIndex];
    if (!curInteractable.active) {
        return;
    }

    switch (curInteractable.type) {
        case INTERACTABLE_TYPE_EXIT:
            if (SETTINGS.language == LANGUAGE_JP) {
                drawCenteredBitmap(BITMAPS[BITMAP_PROMPT_EXIT_JP], INTERACTABLE_PROMPT_LABEL_X, INTERACTABLE_PROMPT_LABEL_Y);
            } else {
                drawCenteredBitmap(BITMAPS[BITMAP_PROMPT_EXIT], INTERACTABLE_PROMPT_LABEL_X, INTERACTABLE_PROMPT_LABEL_Y);
            }
            drawCenteredBitmap(BITMAPS[BITMAP_PROMPT_A_BUTTON], INTERACTABLE_BUTTON_PROMPT_X, INTERACTABLE_BUTTON_PROMPT_Y);
            break;
        default:
            break;
    }
}

void drawUI(float deltaTime) {
    if (screenShakeTime > 0) {
        screenShakeTime -= deltaTime;
        if (screenShakeTime <= 0) {
            screenShakeTime = 0;
            pd->display->setOffset(0, 0);
        } else {
            int curShakeMagnitude = screenShakeTime * shakeMagnitude;
            int shakeX = randInt(-curShakeMagnitude, curShakeMagnitude);
            int shakeY = randInt(-curShakeMagnitude, curShakeMagnitude);
            pd->display->setOffset(shakeX, shakeY);
        }
    }

    drawPlayerHearts();
    drawMinimap();
    drawWeapon(deltaTime);
    drawInteractablePrompt();
}

void screenShake(ScreenShakeStrength strength) {
    switch (strength) {
        case SCREEN_SHAKE_LIGHT:
            screenShakeTime = MAX_SCREEN_SHAKE_TIME * 0.5f;
            shakeMagnitude = MAX_SCREEN_SHAKE_MAGNITUDE * 0.5f;
            break;
        case SCREEN_SHAKE_MEDIUM:
            screenShakeTime = MAX_SCREEN_SHAKE_TIME * 0.75f;
            shakeMagnitude = MAX_SCREEN_SHAKE_MAGNITUDE * 0.75f;
            break;
        case SCREEN_SHAKE_STRONG:
            screenShakeTime = MAX_SCREEN_SHAKE_TIME;
            shakeMagnitude = MAX_SCREEN_SHAKE_MAGNITUDE;
            break;
    }
}