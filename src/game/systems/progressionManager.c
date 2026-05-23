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
    currentArea = AREA_TWILIGHT_COURT;
    #ifdef DEBUG_AREA_OVERRIDE
        currentArea = (Area)DEBUG_AREA_OVERRIDE;
    #endif
    #if DEBUG_START_IN_TEST_AREA
        currentArea = AREA_TEST;
    #endif
    currentRoom = 0;
    currentRoomType = ROOM_TYPE_ENEMY;
    experience = 0.0f;
    curLevel = 0;

    areaData[AREA_TWILIGHT_COURT] = (AreaData){
        .wallTexture = TEXTURES[TEXTURE_TWILIGHT_COURT_WALL],
        .floorTexture = TEXTURES[TEXTURE_TWILIGHT_COURT_FLOOR],
        .gateTexture = TEXTURES[TEXTURE_TWILIGHT_COURT_WALL],
        .skyboxTexture = TEXTURES[TEXTURE_TWILIGHT_COURT_SKYBOX],
        .useSkybox = true
    };
    areaData[AREA_LOST_ARCHIVES] = (AreaData){
        .wallTexture = TEXTURES[TEXTURE_LOST_ARCHIVES_WALL],
        .ceilingTexture = TEXTURES[TEXTURE_LOST_ARCHIVES_CEILING],
        .floorTexture = TEXTURES[TEXTURE_LOST_ARCHIVES_FLOOR],
        .gateTexture = TEXTURES[TEXTURE_LOST_ARCHIVES_GATE]
    };
    areaData[AREA_ECHOING_CAVERNS] = (AreaData){
        .wallTexture = TEXTURES[TEXTURE_ECHOING_CAVERNS_WALL],
        .ceilingTexture = TEXTURES[TEXTURE_ECHOING_CAVERNS_CEILING],
        .floorTexture = TEXTURES[TEXTURE_ECHOING_CAVERNS_FLOOR],
        .gateTexture = TEXTURES[TEXTURE_ECHOING_CAVERNS_GATE],
        .useSkybox = false
    };
    areaData[AREA_TUTORIAL] = (AreaData){
        .wallTexture = TEXTURES[TEXTURE_TUTORIAL_WALL],
        .ceilingTexture = TEXTURES[TEXTURE_TUTORIAL_CEILING],
        .floorTexture = TEXTURES[TEXTURE_TUTORIAL_FLOOR],
        .gateTexture = TEXTURES[TEXTURE_TUTORIAL_WALL],
        .useSkybox = false
    };
    areaData[AREA_TEST] = (AreaData){
        .wallTexture = TEXTURES[TEXTURE_TEST_WALL],
        .ceilingTexture = TEXTURES[TEXTURE_TEST_CEILING],
        .floorTexture = TEXTURES[TEXTURE_TEST_FLOOR],
        .gateTexture = TEXTURES[TEXTURE_TEST_WALL],
        .skyboxTexture = TEXTURES[TEXTURE_TEST_SKYBOX],
        .useSkybox = true
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

void setCurrentArea(Area area) {
    currentArea = area;
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
        showPopup(POPUP_AREA_CLEARED, POPUP_POSITION_CENTER);
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

void showAreaEnterPopup(void) {
    PopupType popup;
    switch (currentArea) {
        case AREA_TWILIGHT_COURT:
            popup = POPUP_AREA_TWILIGHT_COURT;
            break;
        case AREA_LOST_ARCHIVES:
            popup = POPUP_AREA_LOST_ARCHIVES;
            break;
        case AREA_ECHOING_CAVERNS:
            popup = POPUP_AREA_ECHOING_CAVERNS;
            break;
        case AREA_TEST:
        default:
            popup = POPUP_AREA_TEST;
            break;
    }
    showPopup(popup, POPUP_POSITION_CENTER);
}

void incrementToNextRoom(void) {
    currentRoom++;
    if (currentRoom >= MAX_ROOM_COUNT) {
        currentRoom = 0;
        currentArea++;
        if (currentArea >= AREA_COUNT) {
            currentArea = AREA_TWILIGHT_COURT;
        }
    }

    if (currentRoom == MAX_ROOM_COUNT - 1) {
        currentRoomType = ROOM_TYPE_BOSS;
    } else {
        currentRoomType = ROOM_TYPE_ENEMY;
    }

    switchScene(SCENE_GAME);
}
