#pragma once

#include "pd_api.h"
#include "globals/helpers.h"
#include "stdbool.h"

#define MAX_PARTICLE_COUNT 300

typedef enum ParticleType {
	PARTICLE_TYPE_NONE,
	PARTICLE_TYPE_BLANK,
	PARTICLE_TYPE_PROJECTILE_BREAK,
	PARTICLE_TYPE_PROJECTILE_BREAK_2,
	PARTICLE_TYPE_PROJECTILE_BREAK_3,
	PARTICLE_TYPE_PROJECTILE_BREAK_4,
	PARTICLE_TYPE_PROJECTILE_BREAK_5,
	PARTICLE_TYPE_PROJECTILE_SHRINK,
	PARTICLE_TYPE_WARNING,
	PARTICLE_TYPE_WARNING_SHORT,
	PARTICLE_TYPE_WARNING_MEDIUM,
	PARTICLE_TYPE_WARNING_LONG,
	PARTICLE_TYPE_BOSS_ROOT_ATTACK,
	PARTICLE_TYPE_PLAYER_ROOT_ATTACK,
	PARTICLE_TYPE_LIGHTNING_ATTACK,
	PARTICLE_TYPE_SERPENT_TAIL,
	PARTICLE_TYPE_BOMB,
	PARTICLE_TYPE_ENEMY_BOMB,
	PARTICLE_TYPE_BLACK_HOLE,
	PARTICLE_TYPE_RUBBLE,
	PARTICLE_TYPE_HEAL,
	PARTICLE_TYPE_SPAWN,
	PARTICLE_TYPE_COUNT
} ParticleType;

typedef struct Particle {
	float x;
	float y;
	float sourceX;
	float sourceY;
	ParticleType type;
	float animationTime;
	int frameCount;
	int animationFrame;
	bool active;
	bool enemyParticle;
	bool appliedDamage;
	bool spawnTriggered;
	int pendingEnemyType;
	int burstEnemyType; // Enemy type whose projectile ring fires when the particle damages, -1 for none
	ParticleType spawnOnEndParticleType;
} Particle;

extern Particle particles[MAX_PARTICLE_COUNT];

void initParticles(void);
void clearParticles(void);
int spawnParticle(ParticleType type, float x, float y, float sourceX, float sourceY, ParticleType spawnOnEndParticleType, bool enemyParticle);
void spawnEnemyWithSpawnParticle(int enemyType, float x, float y);
// Spawns an enemy particle that also bursts a ring of burstEnemyType's projectiles when it damages.
// The ring count comes from that enemy's abilityDataTertiary, its speed/visuals from projectileData.
int spawnParticleWithProjectileBurst(ParticleType type, float x, float y, float sourceX, float sourceY, int burstEnemyType);
void updateParticles(float deltaTime);

// Lays a static line of warning particles from (x, y) along the unit direction (dirX, dirY), one
// every `spacing` units out to `length` — a telegraph for a straight-line attack.
void spawnWarningLine(ParticleType type, float x, float y, float dirX, float dirY, float length, float spacing);