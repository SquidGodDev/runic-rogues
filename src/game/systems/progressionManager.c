#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/sceneManager.h"

#include "game/systems/progressionManager.h"
#include "game/rendering/textures.h"
#include "game/player/player.h"
#include "game/entities/interactables.h"
#include "game/map/map.h"
#include "game/UI/UI.h"

static int currentRoom = 0;
static bool roomCompleted;
static bool bossDefeated;

static RoomType currentRoomType;
static Area currentArea = AREA_LOST_ARCHIVES;
static AreaData areaData[AREA_COUNT];

void initProgressionManager(void) {
    currentArea = AREA_LOST_ARCHIVES;
    currentRoom = 0;
    currentRoomType = ROOM_TYPE_ENEMY;

    areaData[AREA_ECHOING_CAVERNS] = (AreaData){
        .wallTexture = TEXTURES[TEXTURE_ECHOING_CAVERNS_WALL],
        .ceilingTexture = TEXTURES[TEXTURE_ECHOING_CAVERNS_CEILING],
        .floorTexture = TEXTURES[TEXTURE_ECHOING_CAVERNS_FLOOR],
        .gateTexture = TEXTURES[TEXTURE_ECHOING_CAVERNS_GATE]
    };
    areaData[AREA_LOST_ARCHIVES] = (AreaData){
        .wallTexture = TEXTURES[TEXTURE_LOST_ARCHIVES_WALL],
        .ceilingTexture = TEXTURES[TEXTURE_LOST_ARCHIVES_CEILING],
        .floorTexture = TEXTURES[TEXTURE_LOST_ARCHIVES_FLOOR],
        .gateTexture = TEXTURES[TEXTURE_LOST_ARCHIVES_GATE]
    };
    areaData[AREA_TEST] = (AreaData){
        .wallTexture = TEXTURES[TEXTURE_TEST_WALL],
        .ceilingTexture = TEXTURES[TEXTURE_TEST_CEILING],
        .floorTexture = TEXTURES[TEXTURE_TEST_FLOOR],
        .gateTexture = TEXTURES[TEXTURE_TEST_WALL]
    };
}

void resetProgressionManagerRoomState(void) {
    roomCompleted = false;
    bossDefeated = false;
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

RoomType getCurrentRoomType(void) {
    return currentRoomType;
}

void setBossDefeated(void) {
    bossDefeated = true;
}

bool isBossDefeated(void) {
    return bossDefeated;
}

void setRoomCompleted(bool completed) {
    roomCompleted = completed;
    if (completed) {
        showRoomClearedPopup();
    }
}

bool isRoomCompleted(void) {
    return roomCompleted;
}

void incrementToNextRoom(void) {
    currentRoom++;
    if (currentRoom >= MAX_ROOM_COUNT) {
        currentRoom = 0;
        currentArea++;
        if (currentArea >= AREA_COUNT) {
            currentArea = AREA_ECHOING_CAVERNS;
        }
    }
    switchScene(SCENE_GAME);
}
