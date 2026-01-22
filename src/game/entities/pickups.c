#define TARGET_EXTENSION 1

#include "globals/helpers.h"
#include "game/entities/entities.h"
#include "game/entities/pickups.h"
#include "game/player/player.h"
#include "game/map/map.h"
#include "game/systems/progressionManager.h"

#define PICKUP_DISTANCE 80.0f

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
    }
};

Pickup pickups[MAX_PICKUP_COUNT];

void initPickups(void) {
    for (int i = 0; i < MAX_PICKUP_COUNT; i++) {
        pickups[i].active = false;
    }
}

void spawnPickup(int worldX, int worldY, PickupType type) {
    for (int i = 0; i < MAX_PICKUP_COUNT; i++) {
        if (!pickups[i].active) {
            PickupData data = pickupData[type];
            pickups[i] = (Pickup) {
                .x = worldX,
                .y = worldY,
                .active = 1,
                .animationFrame = 0,
                .animationTime = data.animationFrameTime,
                .textureIndex = data.textureIndex
            };
            return;
        }
    }
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
            pickup->active = false; // Pickup collected
            if (pickup->type == PICKUP_TYPE_XP_ORB) {
                addExperience(1.0f);
            }
        }

        addEntityToRenderQueue(pickup->x, pickup->y, PICKUP, i, pickup->textureIndex, pickup->animationFrame);
    }
}