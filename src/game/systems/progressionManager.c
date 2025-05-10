#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "game/systems/progressionManager.h"
#include "game/rendering/textures.h"

Floor currentFloor = FLOOR_LOST_ARCHIVES;
FloorData floorData[FLOOR_COUNT];

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