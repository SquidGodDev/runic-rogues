#pragma once

#include "pd_api.h"

enum DrawableType {
    ENEMY,
    PROJECTILE
};

extern enum DRAWABLE_TYPE DrawableType;

void initDrawables(void);
void renderDrawables(float *zBuffer);
void addDrawableToRenderQueue(float x, float y, int type, int index, uint8_t *texture);