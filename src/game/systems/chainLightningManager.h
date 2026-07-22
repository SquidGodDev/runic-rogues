#pragma once

void startChainLightning(float fromX, float fromY, int damage, int chainCount);
void startChainLightningOnNearestEnemy(float fromX, float fromY, float maxRange, int damage, int chainCount);
// Arc lightning independently to the nearest `count` enemies within maxRange,
// damaging each. Used by the Lightning Rod rune.
void arcLightningToNearestEnemies(float fromX, float fromY, float maxRange, int damage, int count);
