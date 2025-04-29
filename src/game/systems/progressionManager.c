#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "game/systems/progressionManager.h"
#include "game/rendering/textures.h"

Floor currentFloor = FLOOR_ECHOING_CAVERNS;
FloorData floorData[FLOOR_COUNT];

void initProgressionManager(void) {
    floorData[FLOOR_ECHOING_CAVERNS] = (FloorData){
        .wallTexture = echoingCavernsWall,
        .ceilingTexture = echoingCavernsCeiling,
        .floorTexture = echoingCavernsFloor
    };
    floorData[FLOOR_LOST_ARCHIVES] = (FloorData){
        .wallTexture = lostArchivesWall,
        .ceilingTexture = lostArchivesCeiling,
        .floorTexture = lostArchivesFloor
    };
    floorData[FLOOR_TEST] = (FloorData){
        .wallTexture = testWall,
        .ceilingTexture = testCeiling,
        .floorTexture = testFloor
    };
}

FloorData getCurrentFloorData(void) {
    return floorData[currentFloor];
}