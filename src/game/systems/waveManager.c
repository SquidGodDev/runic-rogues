#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "game/systems/waveManager.h"
#include "game/systems/progressionManager.h"
#include "game/enemies/enemies.h"
#include "game/map/map.h"

#define ELITE_ENEMY_TYPE_COUNT 4

typedef struct EnemySpawnData {
    EnemyType basicEnemyType;
    EnemyType eliteEnemyTypes[ELITE_ENEMY_TYPE_COUNT];
} EnemySpawnData;

static EnemySpawnData enemySpawnData[AREA_COUNT] = {
    [AREA_ECHOING_CAVERNS] = {
        .basicEnemyType = ENEMY_TYPE_BABY_SLIME,
        .eliteEnemyTypes = {ENEMY_TYPE_SERPENT, ENEMY_TYPE_SLIME, ENEMY_TYPE_SKELETON, ENEMY_TYPE_BAT}
    },
    [AREA_LOST_ARCHIVES] = {
        .basicEnemyType = ENEMY_TYPE_RAT,
        .eliteEnemyTypes = {ENEMY_TYPE_FLYING_TOME, ENEMY_TYPE_LIVING_ARMOR, ENEMY_TYPE_MUMMY, ENEMY_TYPE_BAT}
    },
    [AREA_TEST] = {
        .basicEnemyType = ENEMY_TYPE_BABY_SLIME,
        .eliteEnemyTypes = {ENEMY_TYPE_FLYING_TOME, ENEMY_TYPE_LIVING_ARMOR, ENEMY_TYPE_WRAITH, ENEMY_TYPE_BAT}
    }
};

#define MAX_BASE_ACTIVE_ENEMIES 20
#define BASE_TOTAL_ENEMIES 10
#define TOTAL_ENEMY_INCREMENT 5
#define WAVE_SPAWN_INTERVAL 1.0f
#define ELITE_BASE_SPAWN_CHANCE 0
#define ELITE_SPAWN_CHANCE_INCREMENT 0.1f

int enemySpawnCount;
int spawnedEnemies;
float spawnTimer;
int eliteSpawnCount;
int eliteSpawnInterval;
int eliteTypeCount;
int currentEliteIndex;

void initWaveManager(void) {
    spawnedEnemies = 0;
    spawnTimer = 0.0f;
    int currentRoom = getCurrentRoom();
    enemySpawnCount = BASE_TOTAL_ENEMIES + (currentRoom * TOTAL_ENEMY_INCREMENT);
    eliteSpawnCount = enemySpawnCount * (ELITE_BASE_SPAWN_CHANCE + (currentRoom * ELITE_SPAWN_CHANCE_INCREMENT));
    eliteSpawnInterval = enemySpawnCount / (eliteSpawnCount > 0 ? eliteSpawnCount : 1);
    eliteTypeCount = currentRoom > ELITE_ENEMY_TYPE_COUNT ? ELITE_ENEMY_TYPE_COUNT : currentRoom;
    currentEliteIndex = 0;
}

bool allEnemiesSpawned(void) {
    return spawnedEnemies >= enemySpawnCount;
}

float getWavePercentage(void) {
    return (float)getEnemiesDefeated() / enemySpawnCount;
}

void updateWaveManager(float deltaTime) {
    spawnTimer += deltaTime;
    if (spawnTimer >= WAVE_SPAWN_INTERVAL) {
        if (getActiveEnemyCount() < MAX_BASE_ACTIVE_ENEMIES && spawnedEnemies < enemySpawnCount) {
            spawnTimer = 0.0f;
            Point spawnPosition = getRandomEnemySpawnPosition();
            EnemyType enemyType = enemySpawnData[getCurrentArea()].basicEnemyType;
            if (eliteSpawnCount > 0 && spawnedEnemies % eliteSpawnInterval == 0) {
                enemyType = enemySpawnData[getCurrentArea()].eliteEnemyTypes[currentEliteIndex];
                currentEliteIndex = (currentEliteIndex + 1) % eliteTypeCount;
            }
            spawnEnemy(enemyType, spawnPosition.x, spawnPosition.y, false /*isMinion*/);
            spawnedEnemies++;
        } else {
            spawnTimer = WAVE_SPAWN_INTERVAL;
        }
    }
}