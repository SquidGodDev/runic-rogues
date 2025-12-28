#pragma once

#include "pd_api.h"
#include "game/player/spells.h"

#define MAX_PICKUP_COUNT 10

typedef enum PickupType {
    PICKUP_TYPE_COIN,
    PICKUP_TYPE_COUNT
} PickupType;

typedef struct Pickup {
    float x;
    float y;
    int mapX;
    int mapY;
    PickupType type;
    bool active;
	int animationFrame;
    float animationTime;
    int textureIndex;
} Pickup;

extern Pickup pickups[MAX_PICKUP_COUNT];

void initPickups(void);
void spawnPickup(int mapX, int mapY, PickupType type);
void updatePickups(float deltaTime);