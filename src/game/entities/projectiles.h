#pragma once

#include "helpers.h"

#define MAX_PROJECTILE_COUNT 20

typedef struct Projectile {
	float x;
	float y;
	float deltaX;
	float deltaY;
	float speed;
	int hitCount;
	float slowTime;
	float slowAmount;
	Set *hitEnemies;
	int damage;
	int textureIndex;
	bool isEnemy;
	bool active;
} Projectile;

extern Projectile projectiles[MAX_PROJECTILE_COUNT];

void initProjectiles(void);
void fireProjectile(float x, float y, float deltaX, float deltaY, float speed, int damage, int textureIndex, bool isEnemy, int hitCount, float slowTime, float slowAmount);
void firePlayerProjectile(float x, float y, float deltaX, float deltaY, float speed, int damage, int hitCount, float slowTime, float slowAmount);
void fireEnemyProjectile(float x, float y, float deltaX, float deltaY, float speed, int damage);
void updateProjectiles(float deltaTime);