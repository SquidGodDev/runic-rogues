#pragma once

#include "pd_api.h"

enum EntityType {
    ENEMY,
    PROJECTILE,
    PARTICLE,
    INTERACTABLE,
};

extern enum ENTITY_TYPE EntityType;

void initEntities(void);
void renderEntities(int *zBuffer);
void addEntityToRenderQueue(float x, float y, int type, int index, uint8_t *texture, int animationFrame);