#pragma once

#include "pd_api.h"

#define MAX_PARTICLE_COUNT 8

typedef enum ParticleType {
	PARTICLE_TYPE_NONE,
	PARTICLE_TYPE_PROJECTILE_BREAK,
	PARTICLE_TYPE_WARNING,
	PARTICLE_TYPE_ROOT_ATTACK,
	PARTICLE_TYPE_COUNT
} ParticleType;

typedef struct Particle {
	float x;
	float y;
	ParticleType type;
	float animationTime;
	uint16_t frameCount;
	uint16_t animationFrame;
	uint16_t active;
	uint16_t damagedPlayer;
	ParticleType spawnOnEndParticleType;
} Particle;

extern Particle particles[MAX_PARTICLE_COUNT];

void initParticles(void);
void spawnParticle(ParticleType type, int x, int y, ParticleType spawnOnEndParticleType);
void updateParticles(float deltaTime);