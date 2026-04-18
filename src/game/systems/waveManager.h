#pragma once

void initWaveManager(void);
bool getBossActive(void);
void setBossActive(bool active);
int getCurrentWave(void);
int getTotalWaves(void);
int getCurrentRoomIndex(void);
void onShopCompleted(void);
void updateWaveManager(float deltaTime);
