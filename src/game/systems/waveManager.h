#pragma once

void initWaveManager(void);
bool getBossActive(void);
void setBossActive(bool active);
float getLevelTimer(void);
int getCurrentWave(void);
int getTotalWaves(void);
void updateWaveManager(float deltaTime);
