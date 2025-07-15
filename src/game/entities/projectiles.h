#pragma once

#include "helpers.h"
#include "game/player/spells.h"

#define MAX_PROJECTILE_COUNT 20

typedef struct Projectile {
	float x;
	float y;
	float deltaX;
	float deltaY;
	float speed;
	SpellProjectileData *spellProjectileData;
	Set *hitEnemies;
	int damage;
	int textureIndex;
	bool isEnemy;
	int hitCount;
	bool active;
} Projectile;

extern Projectile projectiles[MAX_PROJECTILE_COUNT];

void initProjectiles(void);
void fireProjectile(float x, float y, float deltaX, float deltaY, float speed, int damage, int textureIndex, bool isEnemy, SpellProjectileData *spellProjectileData);
void firePlayerProjectile(float x, float y, float deltaX, float deltaY, int damage, SpellProjectileData *spellProjectileData);
void fireEnemyProjectile(float x, float y, float deltaX, float deltaY, float speed, int damage);
void updateProjectiles(float deltaTime);