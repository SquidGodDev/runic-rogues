#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "game/player/player.h"
#include "game/entities/entities.h"
#include "game/enemies/enemies.h"
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
	float knockbackTime;
	float knockbackAmount;
	int enemySpawnFrame;
	SFX sfx;
} ParticleData;

Particle particles[MAX_PARTICLE_COUNT] = {0};
ParticleData particleData[PARTICLE_TYPE_COUNT] = {
	[PARTICLE_TYPE_PROJECTILE_BREAK] = {
		.frameCount = 16,
		.animationTime = 0.016f,
		.textureIndex = TEXTURE_PARTICLE_PROJECTILE_BREAK,
		.damageFrame = 0,
		.sfx = SFX_NONE
	},
	[PARTICLE_TYPE_PROJECTILE_BREAK_2] = {
		.frameCount = 16,
		.animationTime = 0.016f,
		.textureIndex = TEXTURE_PARTICLE_PROJECTILE_BREAK_2,
		.damageFrame = 0,
		.sfx = SFX_NONE
	},
	[PARTICLE_TYPE_PROJECTILE_BREAK_3] = {
		.frameCount = 17,
		.animationTime = 0.016f,
		.textureIndex = TEXTURE_PARTICLE_PROJECTILE_BREAK_3,
		.damageFrame = 0,
		.sfx = SFX_NONE
	},
	[PARTICLE_TYPE_PROJECTILE_BREAK_4] = {
		.frameCount = 15,
		.animationTime = 0.016f,
		.textureIndex = TEXTURE_PARTICLE_PROJECTILE_BREAK_4,
		.damageFrame = 0,
		.sfx = SFX_NONE
	},
	[PARTICLE_TYPE_PROJECTILE_BREAK_5] = {
		.frameCount = 15,
		.animationTime = 0.016f,
		.textureIndex = TEXTURE_PARTICLE_PROJECTILE_BREAK_5,
		.damageFrame = 0,
		.sfx = SFX_NONE
	},
	[PARTICLE_TYPE_PROJECTILE_SHRINK] = {
		.frameCount = 3,
		.animationTime = 0.05f,
		.textureIndex = TEXTURE_PARTICLE_PROJECTILE_SHRINK,
		.damageFrame = 0,
		.sfx = SFX_NONE
	},
	[PARTICLE_TYPE_WARNING] = {
		.frameCount = 4,
		.animationTime = 0.3f,
		.textureIndex = TEXTURE_PARTICLE_WARNING,
		.damageFrame = 0,
		.damage = 0,
		.damageRadius = 0,
		.sfx = SFX_NONE
	},
	[PARTICLE_TYPE_WARNING_SHORT] = {
		.frameCount = 2,
		.animationTime = 0.3f,
		.textureIndex = TEXTURE_PARTICLE_WARNING,
		.damageFrame = 0,
		.damage = 0,
		.damageRadius = 0,
		.sfx = SFX_NONE
	},
	[PARTICLE_TYPE_WARNING_LONG] = {
		.frameCount = 6,
		.animationTime = 0.3f,
		.textureIndex = TEXTURE_PARTICLE_WARNING,
		.damageFrame = 0,
		.damage = 0,
		.damageRadius = 0,
		.sfx = SFX_NONE
	},
	[PARTICLE_TYPE_BOSS_ROOT_ATTACK] = {
		.frameCount = 10,
		.animationTime = 0.1f,
		.textureIndex = TEXTURE_PARTICLE_ROOT_ATTACK,
		.damageFrame = 4,
		.damage = 10,
		.damageRadius = CELL_SIZE / 2,
		.sfx = SFX_ELDER_BOSS_ROOT
	},
	[PARTICLE_TYPE_PLAYER_ROOT_ATTACK] = {
		.frameCount = 10,
		.animationTime = 0.03f,
		.textureIndex = TEXTURE_PARTICLE_ROOT_ATTACK,
		.damageFrame = 4,
		.damage = 10,
		.damageRadius = CELL_SIZE,
		.sfx = SFX_NONE
	},
	[PARTICLE_TYPE_LIGHTNING_ATTACK] = {
		.frameCount = 6,
		.animationTime = 0.08f,
		.textureIndex = TEXTURE_PARTICLE_LIGHTNING_ATTACK,
		.damageFrame = 0,
		.damage = 0,
		.damageRadius = CELL_SIZE,
		.sfx = SFX_NONE
	},
	[PARTICLE_TYPE_SERPENT_TAIL] = {
		.frameCount = 7,
		.animationTime = 0.06f,
		.textureIndex = TEXTURE_PARTICLE_SERPENT_TAIL,
		.damageFrame = 3,
		.damage = 10,
		.damageRadius = CELL_SIZE / 2,
		.sfx = SFX_NONE
	},
	[PARTICLE_TYPE_BOMB] = {
		.frameCount = 66,
		.animationTime = 0.03f,
		.textureIndex = TEXTURE_PARTICLE_BOMB,
		.damageFrame = 47,
		.damage = 30,
		.damageRadius = CELL_SIZE * 3,
		.knockbackTime = 0.3f,
		.knockbackAmount = 1.5f,
		.sfx = SFX_BOMB
	},
	[PARTICLE_TYPE_ENEMY_BOMB] = {
		.frameCount = 66,
		.animationTime = 0.03f,
		.textureIndex = TEXTURE_PARTICLE_MINI_BOMB,
		.damageFrame = 47,
		.damage = 20,
		.damageRadius = CELL_SIZE,
		.knockbackTime = 0.3f,
		.knockbackAmount = 2.0f,
		.sfx = SFX_BOMB
	},
	[PARTICLE_TYPE_BLACK_HOLE] = {
		.frameCount = 50,
		.animationTime = 0.1f,
		.textureIndex = TEXTURE_PARTICLE_BLACK_HOLE,
		.damageFrame = 0,
		.damage = 0,
		.damageRadius = 0,
		.sfx = SFX_NONE
	},
	[PARTICLE_TYPE_RUBBLE] = {
		.frameCount = 9,
		.animationTime = 0.1f,
		.textureIndex = TEXTURE_PARTICLE_RUBBLE,
		.damageFrame = 2,
		.damage = 10,
		.damageRadius = CELL_SIZE * 0.5f,
		.sfx = SFX_ROCK_GOLEM_RUBBLE
	},
	[PARTICLE_TYPE_HEAL] = {
		.frameCount = 8,
		.animationTime = 0.1f,
		.textureIndex = TEXTURE_PARTICLE_HEAL,
		.damageFrame = 0,
		.damage = 0,
		.damageRadius = 0,
		.sfx = SFX_NONE
	},
	[PARTICLE_TYPE_SPAWN] = {
		.frameCount = 18,
		.animationTime = 0.05f,
		.textureIndex = TEXTURE_PARTICLE_SPAWN,
		.damageFrame = 0,
		.damage = 0,
		.damageRadius = 0,
		.enemySpawnFrame = 11,
		.sfx = SFX_SUMMON
	},
};

void initParticles(void) {
	for (int i = 0; i < MAX_PARTICLE_COUNT; i++) {
		particles[i].active = 0;
	}
}

void clearParticles(void) {
	for (int i = 0; i < MAX_PARTICLE_COUNT; i++) {
		particles[i].active = false;
	}
}

int spawnParticle(ParticleType type, float x, float y, float sourceX, float sourceY, ParticleType spawnOnEndParticleType, bool enemyParticle) {
	int newParticleIndex = -1;
	for (int i = 0; i < MAX_PARTICLE_COUNT; i++) {
		if (!particles[i].active) {
			newParticleIndex = i;
			break;
		}
	}

	if (newParticleIndex == -1) {
		return -1;
	}

	ParticleData data = particleData[type];
	Set damagedEntities;
	setInit(&damagedEntities);
	particles[newParticleIndex] = (struct Particle) {
		.x = x,
		.y = y,
		.sourceX = sourceX,
		.sourceY = sourceY,
		.type = type,
		.animationTime = data.animationTime,
		.frameCount = data.frameCount,
		.animationFrame = 0,
		.active = true,
		.enemyParticle = enemyParticle,
		.appliedDamage = false,
		.spawnTriggered = false,
		.pendingEnemyType = -1,
		.spawnOnEndParticleType = spawnOnEndParticleType
	};
	return newParticleIndex;
}

void spawnEnemyWithSpawnParticle(int enemyType, float x, float y) {
	int idx = spawnParticle(PARTICLE_TYPE_SPAWN, x, y, x, y, PARTICLE_TYPE_NONE, false /*enemyParticle*/);
	if (idx >= 0) {
		particles[idx].pendingEnemyType = enemyType;
	}
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
			if (curParticle->type == PARTICLE_TYPE_WARNING && (curParticle->animationFrame % 2 == 1)) {
				playSoundSpatial(data->sfx, curParticle->x, curParticle->y);
			}
			if (curParticle->animationFrame >= curParticle->frameCount) {
				curParticle->active = 0;
				if (curParticle->spawnOnEndParticleType != PARTICLE_TYPE_NONE && !isAreaCompleted()) {
					spawnParticle(curParticle->spawnOnEndParticleType, curParticle->x, curParticle->y, curParticle->sourceX, curParticle->sourceY, PARTICLE_TYPE_NONE, curParticle->enemyParticle);
				}
				continue;
			}
		}

		if (data->damageFrame > 0 && curParticle->animationFrame >= data->damageFrame - 1 && !curParticle->appliedDamage) {
			if (data->sfx != SFX_NONE) {
				playSoundSpatial(data->sfx, curParticle->x, curParticle->y);
			}
			curParticle->appliedDamage = true;
			if (curParticle->enemyParticle) {
				float distSquared = distanceSquared(curParticle->x, curParticle->y, playerX, playerY);
				if (distSquared < data->damageRadius * data->damageRadius) {
					damagePlayerFromDirection(data->damage, curParticle->sourceX, curParticle->sourceY);
				}
			} else {
				damageEnemiesInArea(curParticle->x, curParticle->y, data->damageRadius, data->damage);
				knockbackEnemiesInArea(curParticle->x, curParticle->y, data->damageRadius, curParticle->sourceX, curParticle->sourceY, data->knockbackTime, data->knockbackAmount, false /*awayFromPlayer*/);
			}
		}

		if (data->enemySpawnFrame > 0 && curParticle->pendingEnemyType >= 0
				&& curParticle->animationFrame >= data->enemySpawnFrame - 1
				&& !curParticle->spawnTriggered) {
			curParticle->spawnTriggered = true;
			if (!isAreaCompleted()) {
				if (data->sfx != SFX_NONE) {
					playSoundSpatial(data->sfx, curParticle->x, curParticle->y);
				}
				spawnEnemy((EnemyType)curParticle->pendingEnemyType, (int)curParticle->x, (int)curParticle->y, true /*isMinion*/);
			}
		}

		addEntityToRenderQueue(curParticle->x, curParticle->y, PARTICLE, i, data->textureIndex, curParticle->animationFrame);
	}
}