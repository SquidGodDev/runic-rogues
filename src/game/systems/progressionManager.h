#pragma once

#include "pd_api.h"

typedef enum Floor
{
    FLOOR_ECHOING_CAVERNS,
    FLOOR_LOST_ARCHIVES,
    FLOOR_TEST,
    FLOOR_COUNT
} Floor;

typedef struct FloorData {
    uint8_t *wallTexture;
    uint8_t *ceilingTexture;
    uint8_t *floorTexture;
} FloorData;

void initProgressionManager(void);
FloorData getCurrentFloorData(void);