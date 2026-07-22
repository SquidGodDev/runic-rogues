#define TARGET_EXTENSION 1

#include "globals/helpers.h"
#include "game/entities/entities.h"
#include "game/entities/pickups.h"
#include "game/player/player.h"
#include "game/player/playerStats.h"
#include "game/map/map.h"
#include "game/rendering/particles.h"
#include "game/systems/progressionManager.h"
#include "game/systems/runeManager.h"
#include "game/systems/audioManager.h"

#define PICKUP_DISTANCE 80.0f
#define HEALTH_POTION_HEAL_AMOUNT 5.0f
#define EXPIRATION_TIME 10.0f
#define EXPIRATION_FLASH_TIME 2.0f
#define EXPIRATION_FLASH_INTERVAL 0.2f

typedef struct PickupData {
    int textureIndex;
    int animationFrames;
    float animationFrameTime;
} PickupData;

PickupData pickupData[PICKUP_TYPE_COUNT] = {
    [PICKUP_TYPE_XP_ORB] = {
        .textureIndex = TEXTURE_XP_ORB,
        .animationFrames = 1,
        .animationFrameTime = 0
    },
    [PICKUP_TYPE_ARROW] = {
        .textureIndex = TEXTURE_ARROW,
        .animationFrames = 8,
        .animationFrameTime = 0.2f
    },
    [PICKUP_TYPE_HEALTH_POTION] = {
        .textureIndex = TEXTURE_HEALTH_POTION,
        .animationFrames = 6,
        .animationFrameTime = 0.12f
    }
};

Pickup pickups[MAX_PICKUP_COUNT];

void initPickups(void) {
    for (int i = 0; i < MAX_PICKUP_COUNT; i++) {
        pickups[i].active = false;
    }
}

int spawnPickup(int worldX, int worldY, PickupType type) {
    for (int i = 0; i < MAX_PICKUP_COUNT; i++) {
        if (!pickups[i].active) {
            PickupData data = pickupData[type];
            pickups[i] = (Pickup) {
                .x = worldX,
                .y = worldY,
                .type = type,
                .active = 1,
                .expirationTime = EXPIRATION_TIME,
                .animationFrame = 0,
                .animationTime = data.animationFrameTime,
                .textureIndex = data.textureIndex
            };
            return i;
        }
    }
    return -1;
}

void updatePickups(float deltaTime) {
    for (int i = 0; i < MAX_PICKUP_COUNT; i++) {
        Pickup *pickup = &pickups[i];
        if (!pickup->active) {
            continue;
        }

        PickupData data = pickupData[pickup->type];
        if (data.animationFrames > 1) {
            pickup->animationTime -= deltaTime;
            if (pickup->animationTime <= 0) {
                pickup->animationTime = data.animationFrameTime;
                pickup->animationFrame++;
                if (pickup->animationFrame >= data.animationFrames) {
                    pickup->animationFrame = 0;
                }
            }
        }
        
        if (distanceSquared(pickup->x, pickup->y, playerX, playerY) <= PICKUP_DISTANCE) {
            if (pickup->type == PICKUP_TYPE_XP_ORB) {
                pickup->active = false;
                addExperience(1.0f);
                signalRuneTrigger(RUNE_TRIGGER_EXPERIENCE, 1.0f);
            } else if (pickup->type == PICKUP_TYPE_ARROW) {
                pickup->active = false;
                playSound(SFX_TUTORIAL_ARROW_PICKUP);
            } else if (pickup->type == PICKUP_TYPE_HEALTH_POTION) {
                if (playerHealth < getPlayerMaxHealth()) {
                    playSound(SFX_HEAL);
                    pickup->active = false;
                    healPlayer(HEALTH_POTION_HEAL_AMOUNT);
                    spawnParticle(PARTICLE_TYPE_HEAL, playerX, playerY, playerX, playerY, PARTICLE_TYPE_NONE, false /*enemyParticle*/);
                }
            }
        }

        if (!pickup->active) {
            continue;
        }

        if (pickup->type != PICKUP_TYPE_ARROW) {
            pickup->expirationTime -= deltaTime;
            if (pickup->expirationTime <= 0) {
                pickup->active = false;
                continue;
            }

            if (pickup->expirationTime <= EXPIRATION_FLASH_TIME) {
                int flashPhase = (int)(pickup->expirationTime / EXPIRATION_FLASH_INTERVAL);
                if (flashPhase % 2 == 0) {
                    continue;
                }
            }
        }

        addEntityToRenderQueue(pickup->x, pickup->y, PICKUP, i, pickup->textureIndex, pickup->animationFrame);
    }
}