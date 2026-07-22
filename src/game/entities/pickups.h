#pragma once

#include "pd_api.h"
#include "game/player/spells/spells.h"

#define MAX_PICKUP_COUNT 10

typedef enum PickupType {
    PICKUP_TYPE_XP_ORB,
    PICKUP_TYPE_ARROW,
    PICKUP_TYPE_HEALTH_POTION,
    PICKUP_TYPE_COUNT
} PickupType;

typedef struct Pickup {
    float x;
    float y;
    PickupType type;
    bool active;
    float expirationTime;
	int animationFrame;
    float animationTime;
    int textureIndex;
} Pickup;

extern Pickup pickups[MAX_PICKUP_COUNT];

void initPickups(void);
int spawnPickup(int worldX, int worldY, PickupType type);
void updatePickups(float deltaTime);