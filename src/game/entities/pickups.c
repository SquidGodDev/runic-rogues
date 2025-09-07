#define TARGET_EXTENSION 1

#include "globals/helpers.h"
#include "game/entities/entities.h"
#include "game/entities/pickups.h"
#include "game/player/player.h"
#include "game/map/map.h"

#define PICKUP_DISTANCE 80.0f

#define PICKUP_ANIM_FRAME_TIME 0.2f
#define PICKUP_ANIM_FRAMES 8

Pickup pickups[MAX_PICKUP_COUNT];

void initPickups(void) {
    for (int i = 0; i < MAX_PICKUP_COUNT; i++) {
        pickups[i].active = false;
    }
}

void spawnPickup(int mapX, int mapY, PickupType type) {
    float x = mapX * CELL_SIZE + CELL_SIZE / 2;
    float y = mapY * CELL_SIZE + CELL_SIZE / 2;
    for (int i = 0; i < MAX_PICKUP_COUNT; i++) {
        if (!pickups[i].active) {
            pickups[i] = (Pickup) {
                .x = x,
                .y = y,
                .mapX = mapX,
                .mapY = mapY,
                .active = 1,
                .animationFrame = 0,
                .animationTime = PICKUP_ANIM_FRAME_TIME,
                .textureIndex = TEXTURE_CAMPFIRE // CHANGE TO COIN TEXTURE
            };
            return;
        }
    }
}

int otherPickupAtPosition(int mapX, int mapY) {
    int playerMapX = playerX / CELL_SIZE;
    int playerMapY = playerY / CELL_SIZE;
    if (mapX == playerMapX && mapY == playerMapY) {
        return 1;
    }

    for (int i = 0; i < MAX_PICKUP_COUNT; i++) {
        if (pickups[i].active) {
            if (pickups[i].mapX == mapX && pickups[i].mapY == mapY) {
                return 1;
            }
        }
    }
    return 0;
}

void updatePickups(float deltaTime) {
    for (int i = 0; i < MAX_PICKUP_COUNT; i++) {
        Pickup *pickup = &pickups[i];
        if (!pickup->active) {
            continue;
        }

        pickup->animationTime -= deltaTime;
        if (pickup->animationTime <= 0) {
            pickup->animationTime = PICKUP_ANIM_FRAME_TIME;
            pickup->animationFrame++;
            if (pickup->animationFrame >= PICKUP_ANIM_FRAMES) {
                pickup->animationFrame = 0;
            }
        }
        
        if (distanceSquared(pickup->x, pickup->y, playerX, playerY) <= PICKUP_DISTANCE) {
            pickup->active = false; // Pickup collected
        }

        addEntityToRenderQueue(pickup->x, pickup->y, PICKUP, i, pickup->textureIndex, pickup->animationFrame);
    }
}