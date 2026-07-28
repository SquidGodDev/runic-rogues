#include "game/systems/difficultyManager.h"

static int classUnlockedDifficulty[CLASS_COUNT];
static int runDifficulty;

static int clampLevel(int level) {
    if (level < 0) {
        return 0;
    }
    if (level > DIFFICULTY_LEVEL_COUNT - 1) {
        return DIFFICULTY_LEVEL_COUNT - 1;
    }
    return level;
}

int getClassUnlockedDifficulty(ClassType classType) {
    if (classType < 0 || classType >= CLASS_COUNT) {
        return 0;
    }
    return classUnlockedDifficulty[classType];
}

void setClassUnlockedDifficulty(ClassType classType, int level) {
    if (classType < 0 || classType >= CLASS_COUNT) {
        return;
    }
    classUnlockedDifficulty[classType] = clampLevel(level);
}

void unlockNextDifficultyForClass(ClassType classType, int wonDifficulty) {
    if (classType < 0 || classType >= CLASS_COUNT) {
        return;
    }
    int next = clampLevel(wonDifficulty + 1);
    if (next > classUnlockedDifficulty[classType]) {
        classUnlockedDifficulty[classType] = next;
    }
}

int getRunDifficulty(void) {
    return runDifficulty;
}

void setRunDifficulty(int level) {
    runDifficulty = clampLevel(level);
}

bool difficultyReducesFloorHeal(void) {
    return runDifficulty >= 1;
}

float getDifficultyEnemyDamageMultiplier(void) {
    return runDifficulty >= 2 ? 1.2f : 1.0f;
}

float getDifficultyEnemyCooldownMultiplier(void) {
    return runDifficulty >= 2 ? 0.8f : 1.0f;
}

int getDifficultyBossPropertyIndex(void) {
    return runDifficulty >= 3 ? 1 : 0;
}

float getDifficultyMaxHealthMultiplier(void) {
    return runDifficulty >= 4 ? 0.8f : 1.0f;
}
