#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "helpers.h"
#include "game/UI/UI.h"
#include "game/player/player.h"
#include "game/rendering/textures.h"
#include "game/map/map.h"

#define SCREEN_SHAKE_TIME 0.5f // Seconds
#define SCREEN_SHAKE_MAGNITUDE 7

#define MINIMAP_X 170
#define MINIMAP_Y 30

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

void drawMinimap(void) {
    int minimapSize = 13;
    int cellDrawSize = 4;

    // == Base minimap layer (walls) ==
    int minimapPixelSize = minimapSize * cellDrawSize;
    LCDBitmap *minimapBitmap = pd->graphics->newBitmap(minimapPixelSize, minimapPixelSize, kColorBlack);

    pd->graphics->pushContext(minimapBitmap);
    int playerMapX = playerX / CELL_SIZE;
    int playerMapY = playerY / CELL_SIZE;
    int playerOffsetX = roundf((playerX - playerMapX * CELL_SIZE) / cellDrawSize) + cellDrawSize / 2;
    int playerOffsetY = roundf((playerY - playerMapY * CELL_SIZE) / cellDrawSize) + cellDrawSize / 2;
    
    // Draw walls
    for (int mapY = 0; mapY < minimapSize + 1; mapY++) {
        int adjustedMapY = playerMapY - (minimapSize + 1) / 2 + mapY;
        if (adjustedMapY < 0 || adjustedMapY >= MAP_HEIGHT) {
            continue;
        }
        for (int mapX = 0; mapX < minimapSize + 1; mapX++) {
            int adjustedMapX = playerMapX - (minimapSize + 1) / 2 + mapX;
            if (adjustedMapX < 0 || adjustedMapX >= MAP_WIDTH) {
                continue;
            }
            int drawX = mapX * cellDrawSize - playerOffsetX;
            int drawY = mapY * cellDrawSize - playerOffsetY;
            if (isBlocked(adjustedMapX, adjustedMapY)) {
                pd->graphics->drawBitmap(minimapWallBitmap, drawX, drawY, kBitmapUnflipped);
            }
        }
    }
    pd->graphics->popContext();

    // Set circle mask
    LCDBitmap *minimapMask = pd->graphics->newBitmap(minimapPixelSize, minimapPixelSize, kColorBlack);
    pd->graphics->pushContext(minimapMask);
    pd->graphics->fillEllipse(1, 1, minimapPixelSize - 2, minimapPixelSize - 2, 0, 0, kColorWhite);
    pd->graphics->popContext();
    pd->graphics->setBitmapMask(minimapBitmap, minimapMask);

    // Draw base layer
    pd->graphics->drawRotatedBitmap(minimapBitmap, MINIMAP_X, MINIMAP_Y, radToDeg(2*PI - playerAngle - P2), 0.5f, 0.5f, 1.0f, 1.0f);
    pd->graphics->freeBitmap(minimapBitmap);
    pd->graphics->freeBitmap(minimapMask);
    // == End of base layer ==

    // == Overlay layer (player + entities + border) ==
    LCDBitmap *minimapOverlayBitmap = pd->graphics->newBitmap(minimapPixelSize, minimapPixelSize, kColorClear);
    pd->graphics->pushContext(minimapOverlayBitmap);

    // Draw player
    int playerMinimapX = minimapSize / 2 * cellDrawSize - 1;
    int playerMinimapY = minimapSize / 2 * cellDrawSize - 1;
    pd->graphics->drawBitmap(minimapPlayerBitmap, playerMinimapX, playerMinimapY, kBitmapUnflipped);

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