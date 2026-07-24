#pragma once

void startChainLightning(float fromX, float fromY, float damage, int chainCount);
void startChainLightningOnNearestEnemy(float fromX, float fromY, float maxRange, float damage, int chainCount);
// Arc lightning independently to the nearest `count` enemies within maxRange,
// damaging each. Used by the Lightning Rod rune.
void arcLightningToNearestEnemies(float fromX, float fromY, float maxRange, float damage, int count);
