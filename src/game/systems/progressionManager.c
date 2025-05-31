#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "game/systems/progressionManager.h"
#include "game/rendering/textures.h"
#include "game/entities/interactables.h"
#include "game/map/map.h"
#include "game/UI/UI.h"

static int currentRoom = 0;

static Floor currentFloor = FLOOR_LOST_ARCHIVES;
static FloorData floorData[FLOOR_COUNT];
static EnemyType enemySpawnList[FLOOR_COUNT][ENEMY_TYPES_PER_FLOOR] = {
    [FLOOR_ECHOING_CAVERNS] = {ENEMY_TYPE_SLIME, ENEMY_TYPE_SKELETON, ENEMY_TYPE_BAT},
    [FLOOR_LOST_ARCHIVES] = {ENEMY_TYPE_IMP, ENEMY_TYPE_FLYING_TOME, ENEMY_TYPE_LIVING_ARMOR},
    [FLOOR_TEST] = {ENEMY_TYPE_IMP}
};

void initProgressionManager(void) {
    floorData[FLOOR_ECHOING_CAVERNS] = (FloorData){
        .wallTexture = TEXTURES[TEXTURE_ECHOING_CAVERNS_WALL],
        .ceilingTexture = TEXTURES[TEXTURE_ECHOING_CAVERNS_CEILING],
        .floorTexture = TEXTURES[TEXTURE_ECHOING_CAVERNS_FLOOR]
    };
    floorData[FLOOR_LOST_ARCHIVES] = (FloorData){
        .wallTexture = TEXTURES[TEXTURE_LOST_ARCHIVES_WALL],
        .ceilingTexture = TEXTURES[TEXTURE_LOST_ARCHIVES_CEILING],
        .floorTexture = TEXTURES[TEXTURE_LOST_ARCHIVES_FLOOR]
    };
    floorData[FLOOR_TEST] = (FloorData){
        .wallTexture = TEXTURES[TEXTURE_TEST_WALL],
        .ceilingTexture = TEXTURES[TEXTURE_TEST_CEILING],
        .floorTexture = TEXTURES[TEXTURE_TEST_FLOOR]
    };
}

FloorData getCurrentFloorData(void) {
    return floorData[currentFloor];
}

int getCurrentRoom(void) {
    return currentRoom;
}

void roomCompleted(void) {
    addInteractable(PORTAL_SPAWN_MAP_X, PORTAL_SPAWN_MAP_Y, INTERACTABLE_TYPE_EXIT);
    // TODO: Show arrow pointing to exit interactable
}

void nextRoom(void) {
    currentRoom++;
    if (currentRoom >= MAX_ROOM_COUNT) {
        currentRoom = 0;
        currentFloor++;
        if (currentFloor >= FLOOR_COUNT) {
            // TODO: Transition to game win scene
            currentFloor = FLOOR_ECHOING_CAVERNS; // Loop back to the first floor
        }
    }
    pd->lua->callFunction("NextLevel", 0, NULL);
}

EnemyType *getFloorSpawnList(void) {
    return enemySpawnList[currentFloor];
}