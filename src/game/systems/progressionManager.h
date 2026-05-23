#pragma once

#include "pd_api.h"
#include "globals/globals.h"
#include "game/enemies/enemies.h"

#define MAX_ROOM_COUNT 1

typedef enum Area
{
    AREA_TWILIGHT_COURT,
    AREA_LOST_ARCHIVES,
    AREA_ECHOING_CAVERNS,
    AREA_TUTORIAL,
    AREA_TEST,
    AREA_COUNT
} Area;

typedef struct AreaData {
    uint8_t *wallTexture;
    uint8_t *ceilingTexture;
    uint8_t *floorTexture;
    uint8_t *gateTexture;
    uint8_t *skyboxTexture;
    bool useSkybox;
} AreaData;

void initProgressionManager(void);
void resetProgressionManagerRoomState(void);
float getExperiencePercentToNextLevel(void);
void addExperience(float amount);
AreaData getCurrentAreaData(void);
Area getCurrentArea(void);
void setCurrentArea(Area area);
int getCurrentRoom(void);
RoomType getCurrentRoomType(void);
void setAreaCompleted(bool completed);
void checkAndSpawnPendingExit(void);
bool isAreaCompleted(void);
void incrementToNextRoom(void);
void showAreaEnterPopup(void);