#pragma once

void initProjectiles(void);
void fireProjectile(float x, float y, float deltaX, float deltaY, float speed, int damage, uint8_t *texture, int isEnemy);
void updateProjectiles(float deltaTime);