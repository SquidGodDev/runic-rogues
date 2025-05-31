#pragma once

#include "pd_api.h"
#include "game/enemies/enemies.h"

#define MAX_ROOM_COUNT 10
#define ENEMY_TYPES_PER_FLOOR 3

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
int getCurrentRoom(void);
void roomCompleted(void);
void nextRoom(void);
EnemyType *getFloorSpawnList(void);