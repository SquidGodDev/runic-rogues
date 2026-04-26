#pragma once

#include "stdbool.h"

void startBossCinematic(int bossIndex);
void startBossDeathCinematic(int bossIndex);
void updateBossCinematic(float deltaTime);
bool isBossCinematicActive(void);
bool isBossDeathCinematicActive(void);
float getBossCinematicProgress(void);
