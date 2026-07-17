#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/helpers.h"
#include "game/entities/projectiles.h"
#include "game/entities/entities.h"
#include "game/map/map.h"
#include "game/player/player.h"
#include "game/rendering/particles.h"
#include "game/rendering/textures.h"
#include "game/enemies/enemies.h"
#include "game/systems/audioManager.h"
#include "game/systems/runStats.h"

#define FIRE_ANGLE_OFFSET PI / 8
#define FIRE_POSITION_OFFSET CELL_SIZE / 2
#define MAP_CHECK_OFFSET_X cosf(FIRE_ANGLE_OFFSET) * FIRE_POSITION_OFFSET / 4
#define MAP_CHECK_OFFSET_Y sinf(FIRE_ANGLE_OFFSET) * FIRE_POSITION_OFFSET / 4

#define PLAYER_PROJECTILE_COLLISION_RANGE 30
#define ENEMY_PROJECTILE_COLLISION_RANGE 10
#define PARTICLE_OFFSET CELL_SIZE / 2

Projectile projectiles[MAX_PROJECTILE_COUNT] = {0};

void initProjectiles(void) {
    for (int i = 0; i < MAX_PROJECTILE_COUNT; i++) {
		projectiles[i].active = 0;
		projectiles[i].hitEnemies = pd->system->realloc(projectiles[i].hitEnemies, sizeof(Set));
	}
}

void clearProjectiles(void) {
	for (int i = 0; i < MAX_PROJECTILE_COUNT; i++) {
		projectiles[i].active = false;
	}
}

void fireProjectile(float x, float y, float sourceX, float sourceY, float deltaX, float deltaY, float speed, int damage, int textureIndex, bool isEnemy, SpellProjectileData spellProjectileData) {
	for (int i = 0; i < MAX_PROJECTILE_COUNT; i++) {
		if (!projectiles[i].active) {
			Set *hitEnemies = projectiles[i].hitEnemies;
			setClear(hitEnemies);
			if (spellProjectileData.hitParticle == PARTICLE_TYPE_NONE) {
				ParticleType breakParticles[] = {PARTICLE_TYPE_PROJECTILE_BREAK, PARTICLE_TYPE_PROJECTILE_BREAK_2, PARTICLE_TYPE_PROJECTILE_BREAK_3, PARTICLE_TYPE_PROJECTILE_BREAK_4, PARTICLE_TYPE_PROJECTILE_BREAK_5};
				spellProjectileData.hitParticle = breakParticles[randInt(0, 4)];
			}
			projectiles[i] = (Projectile) {
				.x = x,
				.y = y,
				.sourceX = sourceX,
				.sourceY = sourceY,
				.deltaX = deltaX,
				.deltaY = deltaY,
				.speed = speed,
				.acceleration = spellProjectileData.acceleration,
				.damage = damage,
				.spellProjectileData = spellProjectileData,
				.textureIndex = textureIndex,
				.frameCount = spellProjectileData.frameCount,
				.frameTime = spellProjectileData.frameTime,
				.currentFrame = 0,
				.currentFrameTime = 0.0f,
				.isEnemy = isEnemy,
				.pierceCount = spellProjectileData.pierceCount,
				.onHitSpell = spellProjectileData.onHitSpell,
				.hitEnemies = hitEnemies,
				.activeTime = spellProjectileData.activeTime,
				.attackId = isEnemy ? 0 : runStatsGetCurrentAttackId(),
				.active = 1
			};
			break;
		}
	}
}

void firePlayerProjectile(float x, float y, float deltaX, float deltaY, int damage, SpellProjectileData spellProjectileData) {
	fireProjectile(x, y, x, y, deltaX, deltaY, spellProjectileData.speed, damage, spellProjectileData.textureIndex, 0 /*isEnemy*/, spellProjectileData);
}

void fireEnemyProjectile(float x, float y, float sourceX, float sourceY, float deltaX, float deltaY, float speed, int damage, SpellProjectileData spellProjectileData) {
	int textureIndex = spellProjectileData.textureIndex != 0 ? spellProjectileData.textureIndex : TEXTURE_ENEMY_PROJECTILE;
	fireProjectile(x, y, sourceX, sourceY, deltaX, deltaY, speed, damage, textureIndex, 1 /*isEnemy*/, spellProjectileData);
}

void updateProjectiles(float deltaTime) {
	for (int i = 0; i < MAX_PROJECTILE_COUNT; i++) {
		Projectile *curProjectile = &projectiles[i];
		if (!curProjectile->active) {
			continue;
		}

		float deltaX = curProjectile->deltaX;
		float deltaY = curProjectile->deltaY;
		if (curProjectile->activeTime > 0) {
			curProjectile->activeTime -= deltaTime;
			if (curProjectile->activeTime <= 0) {
				curProjectile->active = 0;
				int particleX = curProjectile->x;
				int particleY = curProjectile->y;
				ParticleType timeoutParticle = curProjectile->spellProjectileData.timeoutParticle;
				if (timeoutParticle != PARTICLE_TYPE_NONE) {
					spawnParticle(timeoutParticle, particleX, particleY, curProjectile->sourceX, curProjectile->sourceY, PARTICLE_TYPE_NONE, curProjectile->isEnemy);
				}
				continue;
			}
		}

		int entityMapX = (int)((curProjectile->x + MAP_CHECK_OFFSET_X) / CELL_SIZE);
		int entityMapY = (int)((curProjectile->y + MAP_CHECK_OFFSET_Y) / CELL_SIZE);
		int entityMapIndex = entityMapY * MAP_WIDTH + entityMapX;
		int entityMapCell = (entityMapIndex >= 0 && entityMapIndex < MAP_WIDTH * MAP_HEIGHT) ? map[entityMapIndex] : MAP_WALL;
		if ((entityMapIndex >= MAP_WIDTH * MAP_HEIGHT) || (entityMapCell == MAP_WALL || entityMapCell == MAP_GATE)) {
			playSoundSpatial(SFX_WALL_HIT, curProjectile->x, curProjectile->y);
			curProjectile->active = 0;
			int particleX = curProjectile->x - PARTICLE_OFFSET * deltaX;
			int particleY = curProjectile->y - PARTICLE_OFFSET * deltaY;
			ParticleType hitParticle = curProjectile->spellProjectileData.hitParticle;
			if (hitParticle != PARTICLE_TYPE_BLANK) {
				spawnParticle(hitParticle, particleX, particleY, curProjectile->sourceX, curProjectile->sourceY, PARTICLE_TYPE_NONE, curProjectile->isEnemy);
			}
		} else {
			bool collided = false;
			int hitEnemyIndex = -1;
			if (curProjectile->isEnemy) {
				float distSquared = distanceSquared(curProjectile->x, curProjectile->y, playerX, playerY);
				if (distSquared < ENEMY_PROJECTILE_COLLISION_RANGE) {
					collided = true;
					damagePlayerFromDirection(curProjectile->damage, curProjectile->sourceX, curProjectile->sourceY);
				}
			} else {
				hitEnemyIndex = checkEnemyCollision(
					curProjectile->x,
					curProjectile->y,
					curProjectile->hitEnemies,
					PLAYER_PROJECTILE_COLLISION_RANGE,
					curProjectile->damage,
					curProjectile->spellProjectileData.slowTime,
					curProjectile->spellProjectileData.slowAmount
				);
				collided = hitEnemyIndex != -1;
				if (collided) {
					runStatsRecordHitForAttack(curProjectile->attackId);
				}
			}
			if (collided) {
				if (!curProjectile->isEnemy && curProjectile->spellProjectileData.knockbackTime > 0) {
					SpellProjectileData *projData = &curProjectile->spellProjectileData;
					bool awayFromPlayer = !projData->knockbackFromProjectile;
					if (projData->knockbackRadius > 0) {
						knockbackEnemiesInArea(
							curProjectile->x, curProjectile->y,
							(int)(projData->knockbackRadius * CELL_SIZE),
							curProjectile->x, curProjectile->y,
							projData->knockbackTime, projData->knockbackAmount, awayFromPlayer
						);
					} else {
						knockbackEnemy(
							hitEnemyIndex,
							(int)curProjectile->x, (int)curProjectile->y,
							projData->knockbackTime, projData->knockbackAmount, awayFromPlayer
						);
					}
				}
				int particleX = curProjectile->x - PARTICLE_OFFSET * deltaX;
				int particleY = curProjectile->y - PARTICLE_OFFSET * deltaY;
				if (curProjectile->onHitSpell != SPELL_NONE) {
					castSpellDirect(curProjectile->onHitSpell, particleX, particleY, atan2f(deltaY, deltaX));
				}
				if (curProjectile->pierceCount <= 0) {
					curProjectile->active = 0;
					ParticleType hitParticle = curProjectile->spellProjectileData.hitParticle;
					spawnParticle(hitParticle, particleX, particleY, curProjectile->sourceX, curProjectile->sourceY, PARTICLE_TYPE_NONE, curProjectile->isEnemy);
					continue;
				}
				curProjectile->pierceCount--;
			}
		}

		float xVelocity = deltaX * curProjectile->speed;
		float yVelocity = deltaY * curProjectile->speed;
		curProjectile->x += xVelocity * deltaTime;
		curProjectile->y += yVelocity * deltaTime;

		curProjectile->speed += curProjectile->acceleration * deltaTime;

		if (curProjectile->frameCount > 1) {
			curProjectile->currentFrameTime += deltaTime;
			if (curProjectile->currentFrameTime >= curProjectile->frameTime) {
				curProjectile->currentFrameTime = 0.0f;
				curProjectile->currentFrame++;
				if (curProjectile->currentFrame >= curProjectile->frameCount) {
					if (curProjectile->spellProjectileData.holdLastFrame) {
						curProjectile->currentFrame = curProjectile->frameCount - 1;
					} else {
						curProjectile->currentFrame = 0;
					}
				}
			}
		}

		addEntityToRenderQueue(curProjectile->x, curProjectile->y, PROJECTILE, i, curProjectile->textureIndex, curProjectile->currentFrame);
	}
}