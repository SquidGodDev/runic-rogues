#pragma once

#include <stdbool.h>
#include "game/player/classes/classData.h"

// Per-class difficulty. Each class starts at difficulty 0 and unlocks the next
// level each time a run is won at its current highest unlocked difficulty.
// Difficulty effects are cumulative: level N applies every effect from 1..N.
//   0: Normal
//   1: Heal 50% of missing health (instead of 100%) at the start of each floor
//   2: Non-boss enemies deal 20% more damage and attack 20% more often
//   3: Bosses use their harder property set (boss difficulty index 1)
//   4: Start each run with 20% less max health
#define DIFFICULTY_LEVEL_COUNT 5

// Highest unlocked difficulty per class (0..DIFFICULTY_LEVEL_COUNT-1). Persisted
// via the save system.
int getClassUnlockedDifficulty(ClassType classType);
void setClassUnlockedDifficulty(ClassType classType, int level);
void unlockNextDifficultyForClass(ClassType classType, int wonDifficulty);

// Difficulty last selected in the class-select screen per class, used as the
// default the next time that class is opened. Persisted via the save system.
int getClassLastSelectedDifficulty(ClassType classType);
void setClassLastSelectedDifficulty(ClassType classType, int level);

// Difficulty the current run is being played at. Set when a run starts.
int getRunDifficulty(void);
void setRunDifficulty(int level);

// Run-modifier queries derived from the current run difficulty.
bool difficultyReducesFloorHeal(void);            // level >= 1
float getDifficultyEnemyDamageMultiplier(void);   // level >= 2 -> 1.2, else 1.0
float getDifficultyEnemyCooldownMultiplier(void); // level >= 2 -> 0.8, else 1.0
int getDifficultyBossPropertyIndex(void);         // level >= 3 -> 1, else 0
float getDifficultyMaxHealthMultiplier(void);     // level >= 4 -> 0.8, else 1.0
