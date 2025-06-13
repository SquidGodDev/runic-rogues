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

void roomCompleted(void) {
    addInteractable(PORTAL_SPAWN_MAP_X, PORTAL_SPAWN_MAP_Y, INTERACTABLE_TYPE_EXIT);
    displayExitArrow(PORTAL_SPAWN_MAP_X * CELL_SIZE + CELL_SIZE / 2, PORTAL_SPAWN_MAP_Y * CELL_SIZE + CELL_SIZE / 2);
}

void nextRoom(void) {
    transitioning = 1;
    currentRoom++;
    if (currentRoom >= MAX_ROOM_COUNT) {
        currentRoom = 0;
        currentArea++;
        if (currentArea >= AREA_COUNT) {
            // TODO: Transition to game win scene
            currentArea = AREA_ECHOING_CAVERNS; // Temporary: loop back to the first area
        }
    }
    pd->lua->pushInt(playerHealth);
    pd->lua->callFunction("NextLevel", 1, NULL);
}

int isTransitioning(void) {
    return transitioning;
}

EnemyType *getAreaSpawnList(void) {
    return enemySpawnList[currentArea];
}