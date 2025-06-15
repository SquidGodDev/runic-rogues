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

typedef enum RoomType
{
    ROOM_TYPE_ENEMY,
    ROOM_TYPE_ELITE,
    ROOM_TYPE_SHOP,
    ROOM_TYPE_CAMPFIRE,
    ROOM_TYPE_BOSS,
    ROOM_TYPE_COUNT
} RoomType;

void initProgressionManager(int area, int room);
AreaData getCurrentAreaData(void);
Area getCurrentArea(void);
int getCurrentRoom(void);
void roomCompleted(void);
void nextRoom(void);
int isTransitioning(void);
EnemyType *getAreaSpawnList(void);