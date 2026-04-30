#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "game/systems/waveManager.h"
#include "game/systems/progressionManager.h"
#include "game/enemies/enemies.h"
#include "game/map/map.h"
#include "game/UI/UI.h"
#include "game/systems/shopManager.h"
#include "game/systems/bossCinematic.h"
#include "globals/helpers.h"

#define WAVES_PER_ROOM 5
#define MAX_SPAWN_QUEUE 20
#define MAX_POOL_ENTRIES 8
#define SPECIAL_COST_THRESHOLD 3
#define WAVE_SPAWN_STAGGER 0.0f
#define PRE_WAVE_DELAY 0.0f

typedef struct EnemyPoolEntry {
    EnemyType type;
    int cost;
    int weight;
} EnemyPoolEntry;

typedef struct AreaEnemyPool {
    EnemyPoolEntry entries[MAX_POOL_ENTRIES];
    int entryCount;
    EnemyType bossType;
} AreaEnemyPool;

typedef struct RoomBudget {
    int budget;
    int minSpecials;
} RoomBudget;

typedef enum WaveState {
    WAVE_STATE_PRE_WAVE,
    WAVE_STATE_SPAWNING,
    WAVE_STATE_ACTIVE,
    WAVE_STATE_POPUP_WAIT,
    WAVE_STATE_ROOM_TRANSITION,
    WAVE_STATE_BOSS,
    WAVE_STATE_BOSS_POPUP_WAIT,
    WAVE_STATE_BOSS_DONE
} WaveState;

static AreaEnemyPool areaPools[AREA_COUNT] = {
    [AREA_ECHOING_CAVERNS] = {
        .entries = {
            { .type = ENEMY_TYPE_BABY_SLIME, .cost = 1, .weight = 10 },
            { .type = ENEMY_TYPE_SKELETON,   .cost = 3, .weight = 5 },
            { .type = ENEMY_TYPE_SLIME,      .cost = 3, .weight = 5 },
            { .type = ENEMY_TYPE_SERPENT,    .cost = 3, .weight = 3 },
            { .type = ENEMY_TYPE_BAT,        .cost = 3, .weight = 3 },
        },
        .entryCount = 5,
        .bossType = ENEMY_TYPE_ELDER_BOSS,
    },
    [AREA_LOST_ARCHIVES] = {
        .entries = {
            { .type = ENEMY_TYPE_RAT,          .cost = 1, .weight = 10 },
            { .type = ENEMY_TYPE_LIVING_ARMOR, .cost = 3, .weight = 5 },
            { .type = ENEMY_TYPE_MUMMY,        .cost = 3, .weight = 5 },
            { .type = ENEMY_TYPE_FLYING_TOME,  .cost = 3, .weight = 3 },
            { .type = ENEMY_TYPE_IMP,          .cost = 3, .weight = 3 },
        },
        .entryCount = 5,
        .bossType = ENEMY_TYPE_WRAITH_BOSS,
    },
    [AREA_TEST] = {
        .entries = {
            { .type = ENEMY_TYPE_BABY_SLIME,   .cost = 1, .weight = 10 },
            { .type = ENEMY_TYPE_LIVING_ARMOR, .cost = 3, .weight = 5 },
            { .type = ENEMY_TYPE_WRAITH,       .cost = 3, .weight = 5 },
            { .type = ENEMY_TYPE_FLYING_TOME,  .cost = 3, .weight = 3 },
            { .type = ENEMY_TYPE_BAT,          .cost = 3, .weight = 3 },
        },
        .entryCount = 5,
        .bossType = ENEMY_TYPE_WRAITH_BOSS,
    },
};

static const RoomBudget roomBudgets[AREA_COUNT][WAVES_PER_ROOM] = {
    [AREA_ECHOING_CAVERNS] = {
        { .budget = 5,  .minSpecials = 0 },
        { .budget = 8,  .minSpecials = 1 },
        { .budget = 10, .minSpecials = 2 },
        { .budget = 12, .minSpecials = 3 },
        { .budget = 14, .minSpecials = 4 },
    },
    [AREA_LOST_ARCHIVES] = {
        { .budget = 5,  .minSpecials = 1 },
        { .budget = 8,  .minSpecials = 2 },
        { .budget = 10, .minSpecials = 3 },
        { .budget = 12, .minSpecials = 4 },
        { .budget = 14, .minSpecials = 4 },
    },
    [AREA_TEST] = {
        { .budget = 5,  .minSpecials = 0 },
        { .budget = 8,  .minSpecials = 1 },
        { .budget = 10, .minSpecials = 1 },
        { .budget = 12, .minSpecials = 2 },
        { .budget = 14, .minSpecials = 2 },
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
static EnemyType spawnQueue[MAX_SPAWN_QUEUE];
static int spawnQueueSize;

static int pickWeightedEnemy(const AreaEnemyPool *pool, int affordableCost, bool specialsOnly) {
    int totalWeight = 0;
    for (int i = 0; i < pool->entryCount; i++) {
        const EnemyPoolEntry *e = &pool->entries[i];
        if (e->cost > affordableCost) {
            continue;
        }
        if (specialsOnly && e->cost < SPECIAL_COST_THRESHOLD) {
            continue;
        }
        totalWeight += e->weight;
    }
    if (totalWeight <= 0) {
        return -1;
    }

    int roll = randInt(0, totalWeight - 1);
    for (int i = 0; i < pool->entryCount; i++) {
        const EnemyPoolEntry *e = &pool->entries[i];
        if (e->cost > affordableCost) {
            continue;
        }
        if (specialsOnly && e->cost < SPECIAL_COST_THRESHOLD) {
            continue;
        }
        roll -= e->weight;
        if (roll < 0) {
            return i;
        }
    }
    return -1;
}

static EnemyType getBasicEnemyType(const AreaEnemyPool *pool) {
    for (int i = 0; i < pool->entryCount; i++) {
        if (pool->entries[i].cost < SPECIAL_COST_THRESHOLD) {
            return pool->entries[i].type;
        }
    }
    return pool->entries[0].type;
}

int rollEnemiesFromBudget(EnemyType *outTypes, int maxOut, int budget, int minSpecials) {
    AreaEnemyPool *pool = &areaPools[getCurrentArea()];

    if (minSpecials == 0) {
        EnemyType basic = getBasicEnemyType(pool);
        int count = (budget < maxOut) ? budget : maxOut;
        for (int i = 0; i < count; i++) {
            outTypes[i] = basic;
        }
        return count;
    }

    int count = 0;
    for (int i = 0; i < minSpecials && count < maxOut; i++) {
        int idx = pickWeightedEnemy(pool, budget, true /*specialsOnly*/);
        if (idx < 0) {
            break;
        }
        outTypes[count++] = pool->entries[idx].type;
        budget -= pool->entries[idx].cost;
    }

    while (count < maxOut) {
        int idx = pickWeightedEnemy(pool, budget, false /*specialsOnly*/);
        if (idx < 0) {
            break;
        }
        outTypes[count++] = pool->entries[idx].type;
        budget -= pool->entries[idx].cost;
    }

    return count;
}

static void resolveWaveEnemies(int waveIndex) {
    Area area = getCurrentArea();
    RoomBudget rb = roomBudgets[area][waveIndex];
    spawnQueueSize = rollEnemiesFromBudget(spawnQueue, MAX_SPAWN_QUEUE, rb.budget, rb.minSpecials);
}

static void spawnBoss(void) {
    AreaEnemyPool *pool = &areaPools[getCurrentArea()];
    Point spawnPosition = getRoomWorldCenter(DUNGEON_ROOM_COUNT - 1);
    killAllNonBossEnemies();
    int bossIdx = spawnEnemy(pool->bossType, spawnPosition.x, spawnPosition.y, false /*isMinion*/);
    waveState = WAVE_STATE_BOSS;
    startBossCinematic(bossIdx);
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
        // Place state as if the last wave room was cleared and the shop was closed:
        // player in the last wave room, boss gate open, arrow pointing to the boss room.
        currentWave = WAVES_PER_ROOM;
        currentRoomIndex = WAVE_ROOM_COUNT - 1;
        openRoomGate(WAVE_ROOM_COUNT - 1);
        Point bossCenter = getRoomWorldCenter(DUNGEON_ROOM_COUNT - 1);
        setArrowPosition(bossCenter.x, bossCenter.y);
        arrowTargetRoom = DUNGEON_ROOM_COUNT - 1;
    #endif
}

bool getBossActive(void) {
    return bossActive;
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
    showPopup(POPUP_ROOM_CLEARED, POPUP_POSITION_CENTER);
    currentWave++;
    waveState = WAVE_STATE_POPUP_WAIT;
}

void onShopCompleted(void) {
    if (isAreaCompleted()) {
        return;
    }
    int gateIndex = (currentWave >= WAVES_PER_ROOM) ? WAVE_ROOM_COUNT - 1 : currentWave - 1;
    openRoomGate(gateIndex);
    int targetRoom = (currentWave >= WAVES_PER_ROOM) ? DUNGEON_ROOM_COUNT - 1 : currentWave;
    Point targetCenter = getRoomWorldCenter(targetRoom);
    setArrowPosition(targetCenter.x, targetCenter.y);
    arrowTargetRoom = targetRoom;
}

void onBossDefeated(void) {
    bossActive = false;
    setAreaCompleted(true);
    waveState = WAVE_STATE_BOSS_POPUP_WAIT;
}

void updateWaveManager(float deltaTime) {
    if (bossActive) {
        return;
    }
    if (isAreaCompleted() && waveState != WAVE_STATE_BOSS_POPUP_WAIT) {
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
                #if DEBUG_INSTANT_ROOM_CLEAR
                    killAllNonBossEnemies();
                #endif
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

                currentRoomIndex = targetRoom;
                if (currentWave >= WAVES_PER_ROOM) {
                    spawnBoss();
                } else {
                    waveState = WAVE_STATE_PRE_WAVE;
                    transitionTimer = PRE_WAVE_DELAY;
                }
            }
            break;
        }

        case WAVE_STATE_BOSS:
            break;

        case WAVE_STATE_BOSS_POPUP_WAIT:
            if (!isPopupActive()) {
                initShop();
                waveState = WAVE_STATE_BOSS_DONE;
            }
            break;

        case WAVE_STATE_BOSS_DONE:
            break;
    }
}
