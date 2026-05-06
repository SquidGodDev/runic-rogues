#pragma once

#include "game/enemies/enemies.h"

void initSpawnManager(void);
bool getBossActive(void);
void setBossActive(bool active);
int getCurrentRoomIndex(void);
void setCurrentRoomIndex(int roomIndex);
void onShopCompleted(void);
void onBossDefeated(void);
void updateSpawnManager(void);

int rollEnemiesFromBudget(EnemyType *outTypes, int maxOut, int budget, int minSpecials);
