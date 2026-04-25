#pragma once

#include "stdbool.h"

void startBossCinematic(int bossIndex);
void updateBossCinematic(float deltaTime);
bool isBossCinematicActive(void);
float getBossCinematicProgress(void);
