#pragma once

#include "pd_api.h"
#include "game/player/spells.h"

#define MAX_PICKUP_COUNT 10

typedef struct Pickup {
    float x;
    float y;
    int active;
	int animationFrame;
    float animationTime;
    uint8_t *texture;
    Spell spell;
} Pickup;

void initPickups(void);
void spawnRandomPickup(void);
void spawnPickup(float x, float y, Spell spell);
void updatePickups(float deltaTime);