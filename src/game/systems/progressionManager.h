#pragma once

#include "pd_api.h"
#include "globals/globals.h"
#include "game/enemies/enemies.h"

#define MAX_ROOM_COUNT 10

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
    uint8_t *gateTexture;
} AreaData;

void initProgressionManager(void);
void resetProgressionManagerRoomState(void);
AreaData getCurrentAreaData(void);
Area getCurrentArea(void);
int getCurrentRoom(void);
RoomType getCurrentRoomType(void);
void setBossDefeated(void);
bool isBossDefeated(void);
void setRoomCompleted(bool completed);
bool isRoomCompleted(void);
void incrementToNextRoom(void);