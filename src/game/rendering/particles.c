#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "helpers.h"
#include "game/player/player.h"
#include "game/entities/entities.h"
#include "game/rendering/particles.h"
#include "game/rendering/textures.h"

typedef struct ParticleData {
	int frameCount;
	float animationTime;
	uint8_t *texture;
	int damageFrame;
	int damage;
	int damageRadius;
} ParticleData;

ParticleData particleData[PARTICLE_TYPE_COUNT] = {0};
Particle particles[MAX_PARTICLE_COUNT] = {0};

void initParticles(void) {
	for (int i = 0; i < MAX_PARTICLE_COUNT; i++) {
		particles[i].active = 0;
	}

	particleData[PARTICLE_TYPE_PROJECTILE_BREAK] = (ParticleData) {
		.frameCount = 21,
		.animationTime = 0.016f,
		.texture = TEXTURES[TEXTURE_PROJECTILE_BREAK],
		.damageFrame = 0,
		.damage = 0,
		.damageRadius = 0
	};

	particleData[PARTICLE_TYPE_WARNING] = (ParticleData) {
		.frameCount = 4,
		.animationTime = 0.4f,
		.texture = TEXTURES[TEXTURE_WARNING],
		.damageFrame = 0,
		.damage = 0,
		.damageRadius = 0
	};

	particleData[PARTICLE_TYPE_ROOT_ATTACK] = (ParticleData) {
		.frameCount = 10,
		.animationTime = 0.1f,
		.texture = TEXTURES[TEXTURE_ROOT_ATTACK],
		.damageFrame = 4,
		.damage = 1,
		.damageRadius = CELL_SIZE
	};
}

void spawnParticle(ParticleType type, int x, int y, ParticleType spawnOnEndParticleType) {
	int newParticleIndex = -1;
	for (int i = 0; i < MAX_PARTICLE_COUNT; i++) {
		if (!particles[i].active) {
			newParticleIndex = i;
			break;
		}
	}

	if (newParticleIndex == -1) {
		return;
	}

	ParticleData data = particleData[type];
	particles[newParticleIndex] = (struct Particle) {
		.x = x,
		.y = y,
		.type = type,
		.animationTime = data.animationTime,
		.frameCount = data.frameCount,
		.animationFrame = 0,
		.active = 1,
		.damagedPlayer = 0,
		.spawnOnEndParticleType = spawnOnEndParticleType
	};
}

void updateParticles(float deltaTime) {
	for (int i = 0; i < MAX_PARTICLE_COUNT; i++) {
		if (!particles[i].active) {
			continue;
		}

		Particle curParticle = particles[i];
		ParticleData data = particleData[curParticle.type];
		
		curParticle.animationTime -= deltaTime;
		if (curParticle.animationTime <= 0) {
			curParticle.animationTime += data.animationTime;
			curParticle.animationFrame += 1;
			if (curParticle.animationFrame >= curParticle.frameCount) {
				curParticle.active = 0;
				particles[i] = curParticle;
				if (curParticle.spawnOnEndParticleType != PARTICLE_TYPE_NONE) {
					spawnParticle(curParticle.spawnOnEndParticleType, curParticle.x, curParticle.y, PARTICLE_TYPE_NONE);
				}
				continue;
			}
		}

		if (data.damageFrame > 0 && curParticle.animationFrame == data.damageFrame - 1 && !curParticle.damagedPlayer) {
			float distSquared = distanceSquared(curParticle.x, curParticle.y, playerX, playerY);
			if (distSquared < data.damageRadius * data.damageRadius) {
				curParticle.damagedPlayer = 1;
				damagePlayer(data.damage);
			}
		}

		particles[i] = curParticle;

		addEntityToRenderQueue(curParticle.x, curParticle.y, PARTICLE, i, data.texture, curParticle.animationFrame);
	}
}