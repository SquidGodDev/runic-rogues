#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "helpers.h"
#include "game/player/playerUI.h"
#include "game/player/player.h"
#include "game/rendering/textures.h"

#define HIT_INVINCIBILITY_TIME 0.5f // Seconds
#define SCREEN_SHAKE_TIME 0.5f // Seconds
#define SCREEN_SHAKE_MAGNITUDE 7

int playerMaxHealth;
int playerHealth;
float playerHitInvincibility;
float screenShakeTime;

void initPlayerUI(void) {
    playerMaxHealth = 8;
    playerHealth = playerMaxHealth;
    playerHitInvincibility = 0;
    screenShakeTime = 0;
    pd->display->setOffset(0, 0);
    pd->graphics->setBackgroundColor(kColorBlack);
}

void damagePlayer(int amount) {
    if (playerHitInvincibility > 0) {
        return;
    }

    screenShakeTime = SCREEN_SHAKE_TIME;
    playerHitInvincibility = HIT_INVINCIBILITY_TIME;
    playerHealth -= amount;
    if (playerHealth <= 0) {
        playerHealth = 0;
        pd->graphics->clear(kColorBlack);
        pd->lua->callFunction("GameOver", 0, NULL);
    }
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

void drawPlayerUI(float deltaTime) {
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
    if (playerHitInvincibility > 0) {
        playerHitInvincibility -= deltaTime;
        if (playerHitInvincibility < 0) {
            playerHitInvincibility = 0;
        }
    }

    drawPlayerHearts();
    drawWeapon(deltaTime);
}