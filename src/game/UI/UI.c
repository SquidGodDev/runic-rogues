#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "helpers.h"
#include "game/UI/UI.h"
#include "game/player/player.h"
#include "game/rendering/textures.h"

#define SCREEN_SHAKE_TIME 0.5f // Seconds
#define SCREEN_SHAKE_MAGNITUDE 7

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
    drawWeapon(deltaTime);
}

void screenShake(void) {
    screenShakeTime = SCREEN_SHAKE_TIME;
}