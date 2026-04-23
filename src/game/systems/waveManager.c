#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "game/systems/waveManager.h"
#include "game/systems/progressionManager.h"
#include "game/enemies/enemies.h"
#include "game/map/map.h"
#include "game/UI/UI.h"
#include "game/systems/shopManager.h"

#define WAVES_PER_ROOM 5
#define MAX_WAVE_SLOTS 12
#define ELITE_ENEMY_TYPE_COUNT 4
#define WAVE_SPAWN_STAGGER 0.0f
#define PRE_WAVE_DELAY 0.0f

typedef enum EnemyRole {
    ROLE_BASIC,
    ROLE_ELITE,
    ROLE_TANK,
    ROLE_SPECIAL
} EnemyRole;

typedef struct WaveTemplate {
    int slotCount;
    EnemyRole slots[MAX_WAVE_SLOTS];
} WaveTemplate;

typedef struct AreaEnemyPool {
    EnemyType basicType;
    EnemyType eliteTypes[ELITE_ENEMY_TYPE_COUNT];
    EnemyType tankTypes[2];
    int tankTypeCount;
    EnemyType specialTypes[2];
    int specialTypeCount;
    EnemyType bossType;
} AreaEnemyPool;

typedef enum WaveState {
    WAVE_STATE_PRE_WAVE,
    WAVE_STATE_SPAWNING,
    WAVE_STATE_ACTIVE,
    WAVE_STATE_POPUP_WAIT,
    WAVE_STATE_ROOM_TRANSITION,
    WAVE_STATE_BOSS
} WaveState;

static WaveTemplate waveTemplates[WAVES_PER_ROOM] = {
    { .slotCount = 5, .slots = {ROLE_BASIC, ROLE_BASIC, ROLE_BASIC, ROLE_BASIC, ROLE_BASIC} },
    { .slotCount = 5, .slots = {ROLE_BASIC, ROLE_BASIC, ROLE_BASIC, ROLE_ELITE, ROLE_ELITE} },
    { .slotCount = 5, .slots = {ROLE_BASIC, ROLE_BASIC, ROLE_ELITE, ROLE_ELITE, ROLE_SPECIAL} },
    { .slotCount = 7, .slots = {ROLE_BASIC, ROLE_BASIC, ROLE_ELITE, ROLE_ELITE, ROLE_TANK, ROLE_SPECIAL, ROLE_SPECIAL} },
    { .slotCount = 6, .slots = {ROLE_ELITE, ROLE_ELITE, ROLE_ELITE, ROLE_TANK, ROLE_SPECIAL, ROLE_SPECIAL} },
};

static AreaEnemyPool areaPools[AREA_COUNT] = {
    [AREA_ECHOING_CAVERNS] = {
        .basicType = ENEMY_TYPE_BABY_SLIME,
        .eliteTypes = {ENEMY_TYPE_SERPENT, ENEMY_TYPE_SLIME, ENEMY_TYPE_SKELETON, ENEMY_TYPE_BAT},
        .tankTypes = {ENEMY_TYPE_SLIME, ENEMY_TYPE_SKELETON},
        .tankTypeCount = 2,
        .specialTypes = {ENEMY_TYPE_BAT, ENEMY_TYPE_SERPENT},
        .specialTypeCount = 2,
        .bossType = ENEMY_TYPE_ELDER_BOSS,
    },
    [AREA_LOST_ARCHIVES] = {
        .basicType = ENEMY_TYPE_RAT,
        .eliteTypes = {ENEMY_TYPE_FLYING_TOME, ENEMY_TYPE_LIVING_ARMOR, ENEMY_TYPE_MUMMY, ENEMY_TYPE_IMP},
        .tankTypes = {ENEMY_TYPE_LIVING_ARMOR, ENEMY_TYPE_MUMMY},
        .tankTypeCount = 2,
        .specialTypes = {ENEMY_TYPE_FLYING_TOME, ENEMY_TYPE_IMP},
        .specialTypeCount = 2,
        .bossType = ENEMY_TYPE_WRAITH_BOSS,
    },
    [AREA_TEST] = {
        .basicType = ENEMY_TYPE_BABY_SLIME,
        .eliteTypes = {ENEMY_TYPE_FLYING_TOME, ENEMY_TYPE_LIVING_ARMOR, ENEMY_TYPE_WRAITH, ENEMY_TYPE_BAT},
        .tankTypes = {ENEMY_TYPE_LIVING_ARMOR, ENEMY_TYPE_WRAITH},
        .tankTypeCount = 2,
        .specialTypes = {ENEMY_TYPE_FLYING_TOME, ENEMY_TYPE_BAT},
        .specialTypeCount = 2,
        .bossType = ENEMY_TYPE_WRAITH_BOSS,
    },
};

static WaveState waveState;
static int currentWave;
static int currentRoomIndex;
static float staggerTimer;
static int spawnIndex;
static float transitionTimer;
static bool bossActive;
static int arrowTargetRoom;
static EnemyType spawnQueue[MAX_WAVE_SLOTS];
static int spawnQueueSize;

static void resolveWaveEnemies(int waveIndex) {
    AreaEnemyPool *pool = &areaPools[getCurrentArea()];
    WaveTemplate *tmpl = &waveTemplates[waveIndex];
    spawnQueueSize = tmpl->slotCount;

    for (int i = 0; i < tmpl->slotCount; i++) {
        switch (tmpl->slots[i]) {
            case ROLE_BASIC:
                spawnQueue[i] = pool->basicType;
                break;
            case ROLE_ELITE:
                spawnQueue[i] = pool->eliteTypes[randInt(0, ELITE_ENEMY_TYPE_COUNT - 1)];
                break;
            case ROLE_TANK:
                spawnQueue[i] = pool->tankTypes[randInt(0, pool->tankTypeCount - 1)];
                break;
            case ROLE_SPECIAL:
                spawnQueue[i] = pool->specialTypes[randInt(0, pool->specialTypeCount - 1)];
                break;
        }
    }
}

static void spawnBoss(void) {
    AreaEnemyPool *pool = &areaPools[getCurrentArea()];
    Point spawnPosition = getRoomWorldCenter(DUNGEON_ROOM_COUNT - 1);
    killAllNonBossEnemies();
    spawnEnemy(pool->bossType, spawnPosition.x, spawnPosition.y, false /*isMinion*/);
    waveState = WAVE_STATE_BOSS;
}

static void beginWaveSpawning(void) {
    resolveWaveEnemies(currentWave);
    waveState = WAVE_STATE_SPAWNING;
    spawnIndex = 0;
    staggerTimer = 0.0f;
}

void initWaveManager(void) {
    waveState = WAVE_STATE_ROOM_TRANSITION;
    currentWave = 0;
    currentRoomIndex = 0;
    staggerTimer = 0.0f;
    spawnIndex = 0;
    transitionTimer = PRE_WAVE_DELAY;
    bossActive = false;
    arrowTargetRoom = -1;
    spawnQueueSize = 0;
    #if DEBUG_SKIP_TO_BOSS
        spawnBoss();
    #endif
}

bool getBossActive(void) {
    return bossActive;
}

EnemyType getRandomBasicEnemyType(void) {
    return areaPools[getCurrentArea()].basicType;
}

EnemyType getRandomEliteEnemyType(void) {
    AreaEnemyPool *pool = &areaPools[getCurrentArea()];
    return pool->eliteTypes[randInt(0, ELITE_ENEMY_TYPE_COUNT - 1)];
}

EnemyType getRandomTankEnemyType(void) {
    AreaEnemyPool *pool = &areaPools[getCurrentArea()];
    return pool->tankTypes[randInt(0, pool->tankTypeCount - 1)];
}

EnemyType getRandomSpecialEnemyType(void) {
    AreaEnemyPool *pool = &areaPools[getCurrentArea()];
    return pool->specialTypes[randInt(0, pool->specialTypeCount - 1)];
}

void setBossActive(bool active) {
    bossActive = active;
}

int getCurrentWave(void) {
    return currentWave;
}

int getTotalWaves(void) {
    return WAVES_PER_ROOM;
}

int getCurrentRoomIndex(void) {
    return currentRoomIndex;
}

void roomCompleted(void) {
    showPopup(POPUP_ROOM_CLEARED);
    currentWave++;
    waveState = WAVE_STATE_POPUP_WAIT;
}

void onShopCompleted(void) {
    int gateIndex = (currentWave >= WAVES_PER_ROOM) ? WAVE_ROOM_COUNT - 1 : currentWave - 1;
    openRoomGate(gateIndex);
    int targetRoom = (currentWave >= WAVES_PER_ROOM) ? DUNGEON_ROOM_COUNT - 1 : currentWave;
    Point targetCenter = getRoomWorldCenter(targetRoom);
    setArrowPosition(targetCenter.x, targetCenter.y);
    arrowTargetRoom = targetRoom;
}

void updateWaveManager(float deltaTime) {
    if (bossActive || isAreaCompleted()) {
        return;
    }

    switch (waveState) {
        case WAVE_STATE_PRE_WAVE:
            transitionTimer -= deltaTime;
            if (transitionTimer <= 0) {
                beginWaveSpawning();
            }
            break;

        case WAVE_STATE_SPAWNING:
            staggerTimer -= deltaTime;
            if (staggerTimer <= 0 && spawnIndex < spawnQueueSize) {
                Point spawnPosition = getRoomSpawnPosition(currentRoomIndex, spawnIndex);
                spawnEnemy(spawnQueue[spawnIndex], spawnPosition.x, spawnPosition.y, false /*isMinion*/);
                spawnIndex++;
                staggerTimer = WAVE_SPAWN_STAGGER;
            }
            if (spawnIndex >= spawnQueueSize) {
                waveState = WAVE_STATE_ACTIVE;
            }
            break;

        case WAVE_STATE_ACTIVE:
            if (getActiveEnemyCount() == 0) {
                roomCompleted();
            }
            break;

        case WAVE_STATE_POPUP_WAIT:
            if (!isPopupActive()) {
                initShop();
                waveState = WAVE_STATE_ROOM_TRANSITION;
            }
            break;

        case WAVE_STATE_ROOM_TRANSITION: {
            int targetRoom = (currentWave >= WAVES_PER_ROOM) ? DUNGEON_ROOM_COUNT - 1 : currentWave;
            if (getPlayerCurrentRoom() == targetRoom) {
                clearArrow();
                arrowTargetRoom = -1;
                if (currentWave == 0) {
                    closeEntranceGate();
                } else {
                    int gateIndex = (currentWave >= WAVES_PER_ROOM) ? WAVE_ROOM_COUNT - 1 : currentWave - 1;
                    closeRoomGate(gateIndex);
                }
                if (currentWave >= WAVES_PER_ROOM) {
                    spawnBoss();
                } else {
                    currentRoomIndex = targetRoom;
                    waveState = WAVE_STATE_PRE_WAVE;
                    transitionTimer = PRE_WAVE_DELAY;
                }
            }
            break;
        }

        case WAVE_STATE_BOSS:
            break;
    }
}
