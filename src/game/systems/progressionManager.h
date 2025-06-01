#pragma once

#include "pd_api.h"
#include "game/enemies/enemies.h"

#define MAX_ROOM_COUNT 10
#define ENEMY_TYPES_PER_AREA 3

typedef enum Area
{
    AREA_ECHOING_CAVERNS,
    AREA_LOST_ARCHIVES,
    AREA_TEST,
    AREA_COUNT
} Area;

typedef struct AreaData {
    uint8_t *wallTexture;
    uint8_t *ceilingTexture;
    uint8_t *floorTexture;
} AreaData;

void initProgressionManager(void);
AreaData getCurrentAreaData(void);
Area getCurrentArea(void);
int getCurrentRoom(void);
void roomCompleted(void);
void nextRoom(void);
EnemyType *getAreaSpawnList(void);