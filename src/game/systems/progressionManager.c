#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "game/systems/progressionManager.h"
#include "game/rendering/textures.h"
#include "game/player/player.h"
#include "game/entities/interactables.h"
#include "game/map/map.h"
#include "game/UI/UI.h"

static int currentRoom = 0;
static int transitioning = 0;
static int roomCompleted = 0;

static Area currentArea = AREA_LOST_ARCHIVES;
static AreaData areaData[AREA_COUNT];
static EnemyType enemySpawnList[AREA_COUNT][ENEMY_TYPES_PER_AREA] = {
    [AREA_ECHOING_CAVERNS] = {ENEMY_TYPE_SLIME, ENEMY_TYPE_SKELETON, ENEMY_TYPE_BAT},
    [AREA_LOST_ARCHIVES] = {ENEMY_TYPE_IMP, ENEMY_TYPE_FLYING_TOME, ENEMY_TYPE_LIVING_ARMOR},
    [AREA_TEST] = {ENEMY_TYPE_IMP}
};

void initProgressionManager(int area, int room) {
    currentArea = area;
    currentRoom = room;
    transitioning = 0;
    roomCompleted = 0;

    areaData[AREA_ECHOING_CAVERNS] = (AreaData){
        .wallTexture = TEXTURES[TEXTURE_ECHOING_CAVERNS_WALL],
        .ceilingTexture = TEXTURES[TEXTURE_ECHOING_CAVERNS_CEILING],
        .floorTexture = TEXTURES[TEXTURE_ECHOING_CAVERNS_FLOOR]
    };
    areaData[AREA_LOST_ARCHIVES] = (AreaData){
        .wallTexture = TEXTURES[TEXTURE_LOST_ARCHIVES_WALL],
        .ceilingTexture = TEXTURES[TEXTURE_LOST_ARCHIVES_CEILING],
        .floorTexture = TEXTURES[TEXTURE_LOST_ARCHIVES_FLOOR]
    };
    areaData[AREA_TEST] = (AreaData){
        .wallTexture = TEXTURES[TEXTURE_TEST_WALL],
        .ceilingTexture = TEXTURES[TEXTURE_TEST_CEILING],
        .floorTexture = TEXTURES[TEXTURE_TEST_FLOOR]
    };
}

AreaData getCurrentAreaData(void) {
    return areaData[currentArea];
}

Area getCurrentArea(void) {
    return currentArea;
}

int getCurrentRoom(void) {
    return currentRoom;
}

void setRoomCompleted(int completed) {
    roomCompleted = completed;
    if (completed) {
        spawnExit();
    }
}

int isRoomCompleted(void) {
    return roomCompleted;
}

void nextRoom(void) {
    transitioning = 1;
    pd->lua->callFunction("NextLevel", 0, NULL);
}

int isTransitioning(void) {
    return transitioning;
}

EnemyType *getAreaSpawnList(void) {
    return enemySpawnList[currentArea];
}