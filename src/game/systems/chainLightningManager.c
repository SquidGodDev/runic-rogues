#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/helpers.h"
#include "game/systems/chainLightningManager.h"
#include "game/systems/timerManager.h"
#include "game/enemies/enemies.h"
#include "game/rendering/particles.h"

#define CHAIN_LIGHTNING_MAX_CHAINS  10 // hard upper bound for static array size
#define CHAIN_LIGHTNING_CHAIN_RANGE (CELL_SIZE * 5)
#define CHAIN_LIGHTNING_DELAY       0.12f

static int chainStateDamage; // stored here so timer callbacks can access it
// Static array so pointer-to-element passed to addTimer remains valid across frames
static int chainTargetIndices[CHAIN_LIGHTNING_MAX_CHAINS];

static int findNearestActiveEnemy(float x, float y, float maxDistSq, bool *excludeSet) {
    float nearestDist = maxDistSq;
    int nearestIndex = -1;
    for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
        if (!enemies[i].active) {
            continue;
        }
        if (excludeSet != NULL && excludeSet[i]) {
            continue;
        }
        float d = distanceSquared(x, y, enemies[i].x, enemies[i].y);
        if (d < nearestDist) {
            nearestDist = d;
            nearestIndex = i;
        }
    }
    return nearestIndex;
}

static void chainLinkTimerCallback(int repeatCount, void *data) {
    int targetIndex = *(int *)data;
    if (!enemies[targetIndex].active) {
        return;
    }
    damageEnemy(targetIndex, chainStateDamage);
    spawnParticle(PARTICLE_TYPE_LIGHTNING_ATTACK, enemies[targetIndex].x, enemies[targetIndex].y, enemies[targetIndex].x, enemies[targetIndex].y, PARTICLE_TYPE_NONE, false /*enemyParticle*/);
}

static void chainFromPosition(float searchX, float searchY, int chainCount, bool *hitEnemies) {
    if (chainCount > CHAIN_LIGHTNING_MAX_CHAINS) {
        chainCount = CHAIN_LIGHTNING_MAX_CHAINS;
    }
    for (int link = 0; link < chainCount; link++) {
        float maxDistSq = (float)(CHAIN_LIGHTNING_CHAIN_RANGE * CHAIN_LIGHTNING_CHAIN_RANGE);
        int nearestIndex = findNearestActiveEnemy(searchX, searchY, maxDistSq, hitEnemies);
        if (nearestIndex < 0) {
            break;
        }

        hitEnemies[nearestIndex] = true;
        chainTargetIndices[link] = nearestIndex;
        searchX = enemies[nearestIndex].x;
        searchY = enemies[nearestIndex].y;

        // Stagger: 0.12s, 0.24s, 0.36s from initial hit
        addTimer(CHAIN_LIGHTNING_DELAY * (link + 1), 1, chainLinkTimerCallback, &chainTargetIndices[link]);
    }
}

void startChainLightning(float fromX, float fromY, int damage, int chainCount) {
    spawnParticle(PARTICLE_TYPE_LIGHTNING_ATTACK, fromX, fromY, fromX, fromY, PARTICLE_TYPE_NONE, false /*enemyParticle*/);
    chainStateDamage = damage;
    bool hitEnemies[MAX_ENEMY_COUNT] = {false};

    // Mark the enemy closest to impact as already hit (2-cell threshold)
    float closestDistSq = (float)(CELL_SIZE * CELL_SIZE * 4);
    int justHitIndex = findNearestActiveEnemy(fromX, fromY, closestDistSq, NULL);
    if (justHitIndex >= 0) {
        hitEnemies[justHitIndex] = true;
    }

    chainFromPosition(fromX, fromY, chainCount, hitEnemies);
}

void startChainLightningOnNearestEnemy(float fromX, float fromY, float maxRange, int damage, int chainCount) {
    float maxDistSq = maxRange * maxRange;
    int targetIndex = findNearestActiveEnemy(fromX, fromY, maxDistSq, NULL);
    if (targetIndex < 0) {
        return;
    }

    chainStateDamage = damage;
    bool hitEnemies[MAX_ENEMY_COUNT] = {false};
    hitEnemies[targetIndex] = true;

    damageEnemy(targetIndex, damage);
    spawnParticle(PARTICLE_TYPE_LIGHTNING_ATTACK, enemies[targetIndex].x, enemies[targetIndex].y, enemies[targetIndex].x, enemies[targetIndex].y, PARTICLE_TYPE_NONE, false /*enemyParticle*/);

    chainFromPosition(enemies[targetIndex].x, enemies[targetIndex].y, chainCount, hitEnemies);
}
