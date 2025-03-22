#pragma once

void initEnemies(void);
void updateEnemies(float deltaTime);
int checkEnemyCollision(float x, float y, float collisionRange);