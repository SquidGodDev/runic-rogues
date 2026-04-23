#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/sceneManager.h"

#include "game/systems/progressionManager.h"
#include "game/systems/shopManager.h"
#include "game/rendering/textures.h"
#include "game/player/player.h"
#include "game/entities/interactables.h"
#include "game/map/map.h"
#include "game/UI/UI.h"

#define LEVEL_UP_EXPERIENCE_BASE 10.0f
#define LEVEL_UP_EXPERIENCE_SCALING 2.0f

static RoomType currentRoomType;
static Area currentArea;
static AreaData areaData[AREA_COUNT];
static float experience;
static int curLevel;

static int currentRoom = 0;
static bool areaCompleted;
static bool bossDefeated;
static bool pendingExitSpawn;

void initProgressionManager(void) {
    currentArea = AREA_ECHOING_CAVERNS;
    currentRoom = 0;
    currentRoomType = ROOM_TYPE_ENEMY;
    experience = 0.0f;
    curLevel = 0;

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
    areaCompleted = false;
    bossDefeated = false;
    pendingExitSpawn = false;
}

float getExperiencePercentToNextLevel(void) {
    float levelUpExp = LEVEL_UP_EXPERIENCE_BASE * (curLevel + LEVEL_UP_EXPERIENCE_SCALING);
    return experience / levelUpExp;
}

void addExperience(float amount) {
    experience += amount;
    float levelUpExp = LEVEL_UP_EXPERIENCE_BASE * (curLevel + LEVEL_UP_EXPERIENCE_SCALING);
    if (experience >= levelUpExp) {
        experience -= levelUpExp;
        curLevel++;
        initShop();
    }
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

void setAreaCompleted(bool completed) {
    areaCompleted = completed;
    if (completed) {
        showPopup(POPUP_AREA_CLEARED);
        pendingExitSpawn = true;
    }
}

void checkAndSpawnPendingExit(void) {
    if (pendingExitSpawn) {
        pendingExitSpawn = false;
        spawnExit();
    }
}

bool isAreaCompleted(void) {
    return areaCompleted;
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

    if (currentRoom == MAX_ROOM_COUNT - 1) {
        currentRoomType = ROOM_TYPE_BOSS;
    } else {
        currentRoomType = ROOM_TYPE_ENEMY;
    }

    switchScene(SCENE_GAME);
}
