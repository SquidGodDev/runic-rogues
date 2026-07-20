#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "game/systems/spawnManager.h"
#include "game/systems/progressionManager.h"
#include "game/enemies/enemies.h"
#include "game/map/map.h"
#include "game/UI/UI.h"
#include "game/systems/shopManager.h"
#include "game/systems/bossCinematic.h"
#include "game/systems/runeManager.h"
#include "game/systems/runStats.h"
#include "globals/sceneManager.h"
#include "globals/helpers.h"

#define MAX_SPAWN_QUEUE 20
#define MAX_POOL_ENTRIES 8
#define SPECIAL_COST_THRESHOLD 3

typedef struct EnemyPoolEntry {
    EnemyType type;
    int cost;
    int weight;
    int maxCount; // 0 = unlimited
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

typedef enum SpawnState {
    SPAWN_STATE_ACTIVE,
    SPAWN_STATE_POPUP_WAIT,
    SPAWN_STATE_ROOM_TRANSITION,
    SPAWN_STATE_BOSS,
    SPAWN_STATE_BOSS_POPUP_WAIT,
    SPAWN_STATE_BOSS_DONE
} SpawnState;

static AreaEnemyPool areaPools[AREA_COUNT] = {
    [AREA_TWILIGHT_COURT] = {
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
    [AREA_ECHOING_CAVERNS] = {
        .entries = {
            { .type = ENEMY_TYPE_BABY_DRAGON,   .cost = 1, .weight = 10 },
            { .type = ENEMY_TYPE_ROCK_GOLEM,    .cost = 3, .weight = 5 },
            { .type = ENEMY_TYPE_BOMBER,        .cost = 3, .weight = 5 },
            { .type = ENEMY_TYPE_BANSHEE,       .cost = 3, .weight = 3, .maxCount = 3 },
            { .type = ENEMY_TYPE_AIR_ELEMENTAL, .cost = 3, .weight = 3 },
        },
        .entryCount = 5,
        .bossType = ENEMY_TYPE_DRAGON_BOSS,
    },
    [AREA_TEST] = {
        .entries = {
            { .type = ENEMY_TYPE_BABY_DRAGON,   .cost = 1, .weight = 10 },
            { .type = ENEMY_TYPE_BOMBER,        .cost = 3, .weight = 5 },
            { .type = ENEMY_TYPE_ROCK_GOLEM,    .cost = 3, .weight = 5 },
            { .type = ENEMY_TYPE_AIR_ELEMENTAL, .cost = 3, .weight = 3 },
            { .type = ENEMY_TYPE_MUMMY,         .cost = 3, .weight = 3 },
        },
        .entryCount = 5,
        .bossType = ENEMY_TYPE_DRAGON_BOSS,
    }
};

static const RoomBudget roomBudgets[AREA_COUNT][NON_BOSS_ROOM_COUNT] = {
    [AREA_TWILIGHT_COURT] = {
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
    [AREA_ECHOING_CAVERNS] = {
        { .budget = 5,  .minSpecials = 0 },
        { .budget = 8,  .minSpecials = 1 },
        { .budget = 10, .minSpecials = 2 },
        { .budget = 12, .minSpecials = 3 },
        { .budget = 14, .minSpecials = 4 },
    },
    [AREA_TEST] = {
        { .budget = 3,  .minSpecials = 0 },
        { .budget = 8,  .minSpecials = 1 },
        { .budget = 10, .minSpecials = 1 },
        { .budget = 12, .minSpecials = 2 },
        { .budget = 14, .minSpecials = 2 },
    },
};

static SpawnState spawnState;
static int nextRoomIndex;
static int currentRoomIndex;
static bool bossActive;
static int arrowTargetRoom;

static bool isEntryEligible(const EnemyPoolEntry *e, int affordableCost, bool specialsOnly, int pickedCount) {
    if (e->cost > affordableCost) {
        return false;
    }
    if (specialsOnly && e->cost < SPECIAL_COST_THRESHOLD) {
        return false;
    }
    if (e->maxCount > 0 && pickedCount >= e->maxCount) {
        return false;
    }
    return true;
}

static int pickWeightedEnemy(const AreaEnemyPool *pool, int affordableCost, bool specialsOnly, const int *pickedCounts) {
    int totalWeight = 0;
    for (int i = 0; i < pool->entryCount; i++) {
        if (isEntryEligible(&pool->entries[i], affordableCost, specialsOnly, pickedCounts[i])) {
            totalWeight += pool->entries[i].weight;
        }
    }
    if (totalWeight <= 0) {
        return -1;
    }

    int roll = randInt(0, totalWeight - 1);
    for (int i = 0; i < pool->entryCount; i++) {
        if (!isEntryEligible(&pool->entries[i], affordableCost, specialsOnly, pickedCounts[i])) {
            continue;
        }
        roll -= pool->entries[i].weight;
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

    int pickedCounts[MAX_POOL_ENTRIES] = {0};
    int count = 0;
    for (int i = 0; i < minSpecials && count < maxOut; i++) {
        int idx = pickWeightedEnemy(pool, budget, true /*specialsOnly*/, pickedCounts);
        if (idx < 0) {
            break;
        }
        outTypes[count++] = pool->entries[idx].type;
        pickedCounts[idx]++;
        budget -= pool->entries[idx].cost;
    }

    while (count < maxOut) {
        int idx = pickWeightedEnemy(pool, budget, false /*specialsOnly*/, pickedCounts);
        if (idx < 0) {
            break;
        }
        outTypes[count++] = pool->entries[idx].type;
        pickedCounts[idx]++;
        budget -= pool->entries[idx].cost;
    }

    return count;
}

static void spawnRoomEnemies(void) {
    RoomBudget roomBudget = roomBudgets[getCurrentArea()][nextRoomIndex];
    EnemyType spawnQueue[MAX_SPAWN_QUEUE];
    int spawnCount = rollEnemiesFromBudget(spawnQueue, MAX_SPAWN_QUEUE, roomBudget.budget, roomBudget.minSpecials);
    for (int i = 0; i < spawnCount; i++) {
        Point spawnPosition = getRoomSpawnPosition(currentRoomIndex, i);
        spawnEnemy(spawnQueue[i], spawnPosition.x, spawnPosition.y, false /*isMinion*/);
    }
    spawnState = SPAWN_STATE_ACTIVE;
    if (DEBUG.instantRoomClear) {
        killAllEnemies();
    }
}

static void spawnBoss(void) {
    AreaEnemyPool *pool = &areaPools[getCurrentArea()];
    Point spawnPosition = getRoomWorldCenter(DUNGEON_ROOM_COUNT - 1);
    killAllNonBossEnemies();
    int bossIdx = spawnEnemy(pool->bossType, spawnPosition.x, spawnPosition.y, false /*isMinion*/);
    spawnState = SPAWN_STATE_BOSS;
    startBossCinematic(bossIdx);
}

void initSpawnManager(void) {
    spawnState = SPAWN_STATE_ROOM_TRANSITION;
    nextRoomIndex = 0;
    currentRoomIndex = 0;
    bossActive = false;
    arrowTargetRoom = -1;
    if (DEBUG.skipToRoom > 0) {
        // Place state as if rooms 0..(N-1) were cleared and the shop was closed:
        // player in room (N-1), gate to room N open, arrow pointing to room N.
        int targetRoom = DEBUG.skipToRoom;
        currentRoomIndex = targetRoom - 1;
        nextRoomIndex = targetRoom;
        openRoomGate(targetRoom - 1);
        Point targetCenter = getRoomWorldCenter(targetRoom);
        setArrowPosition(targetCenter.x, targetCenter.y);
        arrowTargetRoom = targetRoom;
    }
}

bool getBossActive(void) {
    return bossActive;
}

void setBossActive(bool active) {
    bossActive = active;
}

int getCurrentRoomIndex(void) {
    return currentRoomIndex;
}

void setCurrentRoomIndex(int roomIndex) {
    currentRoomIndex = roomIndex;
}

void roomCompleted(void) {
    signalRuneTrigger(RUNE_TRIGGER_ROOM_CLEAR, 1.0f);
    showPopup(POPUP_ROOM_CLEARED, POPUP_POSITION_CENTER);
    nextRoomIndex++;
    spawnState = SPAWN_STATE_POPUP_WAIT;
}

void onShopCompleted(void) {
    if (isAreaCompleted()) {
        return;
    }
    int gateIndex = (nextRoomIndex >= NON_BOSS_ROOM_COUNT) ? NON_BOSS_ROOM_COUNT - 1 : nextRoomIndex - 1;
    openRoomGate(gateIndex);
    int targetRoom = (nextRoomIndex >= NON_BOSS_ROOM_COUNT) ? DUNGEON_ROOM_COUNT - 1 : nextRoomIndex;
    Point targetCenter = getRoomWorldCenter(targetRoom);
    setArrowPosition(targetCenter.x, targetCenter.y);
    arrowTargetRoom = targetRoom;
}

void onBossDefeated(void) {
    bossActive = false;

    if (getCurrentArea() == AREA_ECHOING_CAVERNS) {
        // Final boss: skip the clear popup, shop, and exit portal — go straight to the victory screen
        runStatsCaptureFinal(true);
        switchScene(SCENE_GAME_OVER);
        return;
    }

    setAreaCompleted(true);
    spawnState = SPAWN_STATE_BOSS_POPUP_WAIT;
}

void updateSpawnManager(void) {
    if (bossActive) {
        return;
    }
    if (isAreaCompleted() && spawnState != SPAWN_STATE_BOSS_POPUP_WAIT) {
        return;
    }

    switch (spawnState) {
        case SPAWN_STATE_ACTIVE:
            if (getActiveEnemyCount() == 0) {
                roomCompleted();
            }
            break;

        case SPAWN_STATE_POPUP_WAIT:
            if (!isPopupActive()) {
                initShop();
                spawnState = SPAWN_STATE_ROOM_TRANSITION;
            }
            break;

        case SPAWN_STATE_ROOM_TRANSITION: {
            int targetRoom = (nextRoomIndex >= NON_BOSS_ROOM_COUNT) ? DUNGEON_ROOM_COUNT - 1 : nextRoomIndex;
            if (getPlayerCurrentRoom() == targetRoom) {
                clearArrow();
                arrowTargetRoom = -1;
                if (nextRoomIndex == 0) {
                    closeEntranceGate();
                } else {
                    int gateIndex = (nextRoomIndex >= NON_BOSS_ROOM_COUNT) ? NON_BOSS_ROOM_COUNT - 1 : nextRoomIndex - 1;
                    closeRoomGate(gateIndex);
                }

                currentRoomIndex = targetRoom;
                if (nextRoomIndex >= NON_BOSS_ROOM_COUNT) {
                    spawnBoss();
                } else {
                    spawnRoomEnemies();
                }
            }
            break;
        }

        case SPAWN_STATE_BOSS:
            break;

        case SPAWN_STATE_BOSS_POPUP_WAIT:
            if (!isPopupActive()) {
                if (getCurrentArea() != AREA_ECHOING_CAVERNS) {
                    initShop();
                }
                spawnState = SPAWN_STATE_BOSS_DONE;
            }
            break;

        case SPAWN_STATE_BOSS_DONE:
            break;
    }
}
