#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "game/systems/waveManager.h"
#include "game/systems/progressionManager.h"
#include "game/enemies/enemies.h"
#include "game/map/map.h"

#define MAX_BASE_ACTIVE_ENEMIES 30
#define BASE_TOTAL_ENEMIES 25
#define WAVE_SPAWN_INTERVAL 0.5f
#define LEVEL_TIME_LIMIT 3.0f * 60.0f // seconds

#define ELITE_ENEMY_TYPE_COUNT 4
typedef struct EnemySpawnData {
    EnemyType basicEnemyType;
    EnemyType eliteEnemyTypes[ELITE_ENEMY_TYPE_COUNT];
    EnemyType bossEnemyType;
    float eliteSpawnStartTime;
    float eliteSpawnBaseChance;
    float eliteSpawnChanceIncrement;
    float eliteSpawnChanceMax;
} EnemySpawnData;

static EnemySpawnData enemySpawnData[AREA_COUNT] = {
    [AREA_ECHOING_CAVERNS] = {
        .basicEnemyType = ENEMY_TYPE_BABY_SLIME,
        .eliteEnemyTypes = {ENEMY_TYPE_SERPENT, ENEMY_TYPE_SLIME, ENEMY_TYPE_SKELETON, ENEMY_TYPE_BAT},
        .bossEnemyType = ENEMY_TYPE_ELDER_BOSS,
        .eliteSpawnStartTime = 10.0f, // seconds
        .eliteSpawnBaseChance = 0.2f,
        .eliteSpawnChanceIncrement = 0.0025f, // per second
        .eliteSpawnChanceMax = 0.5f
    },
    [AREA_LOST_ARCHIVES] = {
        .basicEnemyType = ENEMY_TYPE_RAT,
        .eliteEnemyTypes = {ENEMY_TYPE_FLYING_TOME, ENEMY_TYPE_LIVING_ARMOR, ENEMY_TYPE_MUMMY, ENEMY_TYPE_BAT},
        .bossEnemyType = ENEMY_TYPE_WRAITH_BOSS,
        .eliteSpawnStartTime = 10.0f, // seconds
        .eliteSpawnBaseChance = 0.2f,
        .eliteSpawnChanceIncrement = 0.0025f, // per second
        .eliteSpawnChanceMax = 0.5f
    },
    [AREA_TEST] = {
        .basicEnemyType = ENEMY_TYPE_BABY_SLIME,
        .eliteEnemyTypes = {ENEMY_TYPE_FLYING_TOME, ENEMY_TYPE_LIVING_ARMOR, ENEMY_TYPE_WRAITH, ENEMY_TYPE_BAT},
        .bossEnemyType = ENEMY_TYPE_WRAITH_BOSS,
        .eliteSpawnStartTime = 10.0f, // seconds
        .eliteSpawnBaseChance = 0.2f,
        .eliteSpawnChanceIncrement = 0.0025f, // per second
        .eliteSpawnChanceMax = 0.5f
    }
};

float spawnTimer;
float levelTimer;
float eliteSpawnChance;
int currentEliteIndex;
bool bossActive;

void initWaveManager(void) {
    spawnTimer = 0.0f;
    levelTimer = LEVEL_TIME_LIMIT;
    eliteSpawnChance = 0;
    currentEliteIndex = 0;
    bossActive = false;
}

bool getBossActive(void) {
    return bossActive;
}

void setBossActive(bool active) {
    bossActive = active;
}

float getLevelTimer(void) {
    return levelTimer;
}

void spawnBoss(void) {
    EnemySpawnData curSpawnData = enemySpawnData[getCurrentArea()];
    Point spawnPosition = getRandomEnemySpawnPosition();
    killAllNonBossEnemies();
    spawnEnemy(curSpawnData.bossEnemyType, spawnPosition.x, spawnPosition.y, false /*isMinion*/);
}

void updateWaveManager(float deltaTime) {
    if (bossActive || isRoomCompleted()) {
        return;
    }
    EnemySpawnData curSpawnData = enemySpawnData[getCurrentArea()];
    spawnTimer += deltaTime;
    eliteSpawnChance += curSpawnData.eliteSpawnChanceIncrement * deltaTime;
    if (eliteSpawnChance > curSpawnData.eliteSpawnChanceMax) {
        eliteSpawnChance = curSpawnData.eliteSpawnChanceMax;
    }
    if (levelTimer > 0) {
        levelTimer -= deltaTime;
        if (levelTimer <= 0) {
            spawnBoss();
            return;
        }
    }
    if (spawnTimer >= WAVE_SPAWN_INTERVAL ) {
        if (getActiveEnemyCount() < MAX_BASE_ACTIVE_ENEMIES) {
            spawnTimer = 0.0f;
            Point spawnPosition = getRandomEnemySpawnPosition();
            EnemyType enemyType = curSpawnData.basicEnemyType;
            if (eliteSpawnChance > 0 && randFloat(0, 1.0f) < curSpawnData.eliteSpawnBaseChance + eliteSpawnChance) {
                enemyType = curSpawnData.eliteEnemyTypes[currentEliteIndex];
                currentEliteIndex = (currentEliteIndex + 1) % ELITE_ENEMY_TYPE_COUNT;
            }
            spawnEnemy(enemyType, spawnPosition.x, spawnPosition.y, false /*isMinion*/);
        } else {
            spawnTimer = WAVE_SPAWN_INTERVAL;
        }
    }
}