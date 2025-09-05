#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "game/player/player.h"
#include "game/entities/entities.h"
#include "game/rendering/particles.h"
#include "game/rendering/textures.h"
#include "game/systems/progressionManager.h"

typedef struct ParticleData {
	int frameCount;
	float animationTime;
	int textureIndex;
	int damageFrame;
	int damage;
	int damageRadius;
} ParticleData;

Particle particles[MAX_PARTICLE_COUNT] = {0};
ParticleData particleData[PARTICLE_TYPE_COUNT] = {
	[PARTICLE_TYPE_PROJECTILE_BREAK] = {
		.frameCount = 21,
		.animationTime = 0.016f,
		.textureIndex = TEXTURE_PARTICLE_PROJECTILE_BREAK,
		.damageFrame = 0,
	},
	[PARTICLE_TYPE_PROJECTILE_SHRINK] = {
		.frameCount = 3,
		.animationTime = 0.05f,
		.textureIndex = TEXTURE_PARTICLE_PROJECTILE_SHRINK,
		.damageFrame = 0,
	},
	[PARTICLE_TYPE_WARNING] = {
		.frameCount = 4,
		.animationTime = 0.3f,
		.textureIndex = TEXTURE_PARTICLE_WARNING,
		.damageFrame = 0,
		.damage = 0,
		.damageRadius = 0
	},
	[PARTICLE_TYPE_BOSS_ROOT_ATTACK] = {
		.frameCount = 10,
		.animationTime = 0.1f,
		.textureIndex = TEXTURE_PARTICLE_ROOT_ATTACK,
		.damageFrame = 4,
		.damage = 10,
		.damageRadius = CELL_SIZE / 2
	},
	[PARTICLE_TYPE_PLAYER_ROOT_ATTACK] = {
		.frameCount = 10,
		.animationTime = 0.03f,
		.textureIndex = TEXTURE_PARTICLE_ROOT_ATTACK,
		.damageFrame = 4,
		.damage = 10,
		.damageRadius = CELL_SIZE
	},
	[PARTICLE_TYPE_SERPENT_TAIL] = {
		.frameCount = 7,
		.animationTime = 0.06f,
		.textureIndex = TEXTURE_PARTICLE_SERPENT_TAIL,
		.damageFrame = 3,
		.damage = 10,
		.damageRadius = CELL_SIZE / 2
	},
	[PARTICLE_TYPE_BOMB] = {
		.frameCount = 66,
		.animationTime = 0.03f,
		.textureIndex = TEXTURE_PARTICLE_BOMB,
		.damageFrame = 47,
		.damage = 40,
		.damageRadius = CELL_SIZE
	},
};

void initParticles(void) {
	for (int i = 0; i < MAX_PARTICLE_COUNT; i++) {
		particles[i].active = 0;
	}
}

void spawnParticle(ParticleType type, float x, float y, ParticleType spawnOnEndParticleType, bool enemyParticle) {
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
	Set damagedEntities;
	setInit(&damagedEntities);
	particles[newParticleIndex] = (struct Particle) {
		.x = x,
		.y = y,
		.type = type,
		.animationTime = data.animationTime,
		.frameCount = data.frameCount,
		.animationFrame = 0,
		.active = true,
		.enemyParticle = enemyParticle,
		.appliedDamage = false,
		.spawnOnEndParticleType = spawnOnEndParticleType
	};
}

void updateParticles(float deltaTime) {
	for (int i = 0; i < MAX_PARTICLE_COUNT; i++) {
		Particle *curParticle = &particles[i];
		if (!curParticle->active) {
			continue;
		}

		ParticleData *data = &particleData[curParticle->type];

		curParticle->animationTime -= deltaTime;
		if (curParticle->animationTime <= 0) {
			curParticle->animationTime += data->animationTime;
			curParticle->animationFrame += 1;
			if (curParticle->animationFrame >= curParticle->frameCount) {
				curParticle->active = 0;
				if (curParticle->spawnOnEndParticleType != PARTICLE_TYPE_NONE && !isRoomCompleted()) {
					spawnParticle(curParticle->spawnOnEndParticleType, curParticle->x, curParticle->y, PARTICLE_TYPE_NONE, curParticle->enemyParticle);
				}
				continue;
			}
		}

		if (data->damageFrame > 0 && curParticle->animationFrame >= data->damageFrame - 1 && !curParticle->appliedDamage) {
			curParticle->appliedDamage = true;
			if (curParticle->enemyParticle) {
				float distSquared = distanceSquared(curParticle->x, curParticle->y, playerX, playerY);
				if (distSquared < data->damageRadius * data->damageRadius) {
					damagePlayer(data->damage);
				}
			} else {
				damageEnemiesInArea(curParticle->x, curParticle->y, data->damageRadius * data->damageRadius, data->damage);
			}
		}

		addEntityToRenderQueue(curParticle->x, curParticle->y, PARTICLE, i, data->textureIndex, curParticle->animationFrame);
	}
}