#pragma once

#define MAX_PARTICLE_COUNT 3

typedef struct Particle {
	float x;
	float y;
	float animationTime;
	uint16_t frameCount;
	uint16_t animationFrame;
	uint32_t active;
} Particle;

extern Particle particles[MAX_PARTICLE_COUNT];

void initParticles(void);
void spawnParticle(int x, int y);
void updateParticles(float deltaTime);