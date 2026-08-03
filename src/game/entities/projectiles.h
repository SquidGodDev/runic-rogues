#pragma once

#include "globals/helpers.h"
#include "game/player/spells/spells.h"

#define MAX_PROJECTILE_COUNT 300

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
	float damage;
	int textureIndex;
	int frameCount;
	float frameTime;
	int currentFrame;
	float currentFrameTime;
	bool isEnemy;
	int pierceCount;
	int bounceCount;
	Spell onHitSpell;
	float activeTime;
	int attackId;
	bool active;
} Projectile;

extern Projectile projectiles[MAX_PROJECTILE_COUNT];

void initProjectiles(void);
void clearProjectiles(void);
void fireProjectile(float x, float y, float sourceX, float sourceY, float deltaX, float deltaY, float speed, float damage, int textureIndex, bool isEnemy, SpellProjectileData spellProjectileData);
void firePlayerProjectile(float x, float y, float deltaX, float deltaY, float damage, SpellProjectileData spellProjectileData);
void fireEnemyProjectile(float x, float y, float sourceX, float sourceY, float deltaX, float deltaY, float speed, float damage, SpellProjectileData spellProjectileData);
// Fires count evenly spaced enemy projectiles in a full circle around (x, y), rotated so one of
// them flies along startAngle.
void fireEnemyProjectileRing(float x, float y, int count, float startAngle, float speed, float damage, SpellProjectileData spellProjectileData);
void updateProjectiles(float deltaTime);