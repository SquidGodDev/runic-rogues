#pragma once

#include "globals/helpers.h"
#include "game/player/spells/spells.h"

#define MAX_PROJECTILE_COUNT 100

typedef struct Projectile {
	float x;
	float y;
	float sourceX;
	float sourceY;
	float deltaX;
	float deltaY;
	float speed;
	float acceleration;
	SpellProjectileData spellProjectileData;
	Set *hitEnemies;
	int damage;
	int textureIndex;
	int frameCount;
	float frameTime;
	int currentFrame;
	float currentFrameTime;
	bool isEnemy;
	int pierceCount;
	Spell onHitSpell;
	float activeTime;
	bool active;
} Projectile;

extern Projectile projectiles[MAX_PROJECTILE_COUNT];

void initProjectiles(void);
void clearProjectiles(void);
void fireProjectile(float x, float y, float sourceX, float sourceY, float deltaX, float deltaY, float speed, int damage, int textureIndex, bool isEnemy, SpellProjectileData spellProjectileData);
void firePlayerProjectile(float x, float y, float deltaX, float deltaY, int damage, SpellProjectileData spellProjectileData);
void fireEnemyProjectile(float x, float y, float sourceX, float sourceY, float deltaX, float deltaY, float speed, int damage);
void updateProjectiles(float deltaTime);