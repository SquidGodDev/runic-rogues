#pragma once

#include "game/enemies/enemies.h"

void initWaveManager(void);
bool getBossActive(void);
void setBossActive(bool active);
int getCurrentWave(void);
int getTotalWaves(void);
int getCurrentRoomIndex(void);
void onShopCompleted(void);
void onBossDefeated(void);
void updateWaveManager(float deltaTime);

EnemyType getRandomBasicEnemyType(void);
EnemyType getRandomEliteEnemyType(void);
EnemyType getRandomTankEnemyType(void);
EnemyType getRandomSpecialEnemyType(void);
