#pragma once

#include "pd_api.h"
#include "helpers.h"
#include "stdbool.h"

#define MAX_PARTICLE_COUNT 8

typedef enum ParticleType {
	PARTICLE_TYPE_NONE,
	PARTICLE_TYPE_PROJECTILE_BREAK,
	PARTICLE_TYPE_WARNING,
	PARTICLE_TYPE_BOSS_ROOT_ATTACK,
	PARTICLE_TYPE_PLAYER_ROOT_ATTACK,
	PARTICLE_TYPE_BOMB,
	PARTICLE_TYPE_COUNT
} ParticleType;

typedef struct Particle {
	float x;
	float y;
	ParticleType type;
	float animationTime;
	int frameCount;
	int animationFrame;
	bool active;
	bool enemyParticle;
	bool appliedDamage;
	ParticleType spawnOnEndParticleType;
} Particle;

extern Particle particles[MAX_PARTICLE_COUNT];

void initParticles(void);
void spawnParticle(ParticleType type, int x, int y, ParticleType spawnOnEndParticleType, bool enemyParticle);
void updateParticles(float deltaTime);