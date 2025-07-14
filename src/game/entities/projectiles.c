#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "helpers.h"
#include "game/entities/projectiles.h"
#include "game/entities/entities.h"
#include "game/map/map.h"
#include "game/player/player.h"
#include "game/rendering/particles.h"
#include "game/rendering/textures.h"
#include "game/enemies/enemies.h"
#include "game/systems/audioManager.h"

#define FIRE_ANGLE_OFFSET PI / 8
#define FIRE_POSITION_OFFSET CELL_SIZE / 2
#define MAP_CHECK_OFFSET_X cosf(FIRE_ANGLE_OFFSET) * FIRE_POSITION_OFFSET / 4
#define MAP_CHECK_OFFSET_Y sinf(FIRE_ANGLE_OFFSET) * FIRE_POSITION_OFFSET / 4

#define PLAYER_COLLISION_RANGE 30
#define ENEMY_COLLISION_RANGE 15
#define PARTICLE_OFFSET CELL_SIZE / 2

Projectile projectiles[MAX_PROJECTILE_COUNT] = {0};

void initProjectiles(void) {
    for (int i = 0; i < MAX_PROJECTILE_COUNT; i++) {
		projectiles[i].active = 0;
		projectiles[i].hitEnemies = pd->system->realloc(projectiles[i].hitEnemies, sizeof(Set));
	}
}

void fireProjectile(float x, float y, float deltaX, float deltaY, float speed, int damage, int textureIndex, bool isEnemy, int hitCount, float slowTime, float slowAmount) {
	for (int i = 0; i < MAX_PROJECTILE_COUNT; i++) {
		if (!projectiles[i].active) {
			Set *hitEnemies = projectiles[i].hitEnemies;
			setClear(hitEnemies);
			projectiles[i] = (Projectile) {
				.x = x,
				.y = y,
				.deltaX = deltaX,
				.deltaY = deltaY,
				.speed = speed,
				.damage = damage,
				.textureIndex = textureIndex,
				.isEnemy = isEnemy,
				.hitCount = hitCount,
				.hitEnemies = hitEnemies,
				.slowTime = slowTime,
				.slowAmount = slowAmount,
				.active = 1
			};
			break;
		}
	}
}

void firePlayerProjectile(float x, float y, float deltaX, float deltaY, float speed, int damage, int hitCount, float slowTime, float slowAmount) {
	fireProjectile(x, y, deltaX, deltaY, speed, damage, TEXTURE_PROJECTILE, 0 /*isEnemy*/, hitCount, slowTime, slowAmount);
}

void fireEnemyProjectile(float x, float y, float deltaX, float deltaY, float speed, int damage) {
	fireProjectile(x, y, deltaX, deltaY, speed, damage, TEXTURE_ENEMY_PROJECTILE, 1 /*isEnemy*/, 1 /*hitCount*/, 0 /* slowTime */, 0 /* slowAmount */);
}

void updateProjectiles(float deltaTime) {
	for (int i = 0; i < MAX_PROJECTILE_COUNT; i++) {
		Projectile *curProjectile = &projectiles[i];
		if (!curProjectile->active) {
			continue;
		}

		float deltaX = curProjectile->deltaX;
		float deltaY = curProjectile->deltaY;

		int entityMapX = (int)((curProjectile->x + MAP_CHECK_OFFSET_X) / CELL_SIZE);
		int entityMapY = (int)((curProjectile->y + MAP_CHECK_OFFSET_Y) / CELL_SIZE);
		int entityMapIndex = entityMapY * MAP_WIDTH + entityMapX;
		if ((entityMapIndex >= MAP_WIDTH * MAP_HEIGHT) || (map[entityMapIndex] != 0)) {
			playSound(SFX_WALL_HIT);
			curProjectile->active = 0;
			int particleX = curProjectile->x - PARTICLE_OFFSET * deltaX;
			int particleY = curProjectile->y - PARTICLE_OFFSET * deltaY;
			spawnParticle(PARTICLE_TYPE_PROJECTILE_BREAK, particleX, particleY, PARTICLE_TYPE_NONE, curProjectile->isEnemy);
		} else {
			bool collided = false;
			if (curProjectile->isEnemy) {
				float distSquared = distanceSquared(curProjectile->x, curProjectile->y, playerX, playerY);
				if (distSquared < ENEMY_COLLISION_RANGE) {
					collided = true;
					damagePlayer(curProjectile->damage);
				}
			} else {
				collided = checkEnemyCollision(curProjectile->x, curProjectile->y, curProjectile->hitEnemies, PLAYER_COLLISION_RANGE, curProjectile->damage, curProjectile->slowTime, curProjectile->slowAmount);
			}
			if (collided) {
				curProjectile->hitCount--;
				if (curProjectile->hitCount <= 0) {
					curProjectile->active = 0;
					int particleX = curProjectile->x - PARTICLE_OFFSET * deltaX;
					int particleY = curProjectile->y - PARTICLE_OFFSET * deltaY;
					spawnParticle(PARTICLE_TYPE_PROJECTILE_BREAK, particleX, particleY, PARTICLE_TYPE_NONE, curProjectile->isEnemy);
					continue;
				}
			}
		}

		float xVelocity = deltaX * curProjectile->speed;
		float yVelocity = deltaY * curProjectile->speed;
		curProjectile->x += xVelocity * deltaTime;
		curProjectile->y += yVelocity * deltaTime;

		addEntityToRenderQueue(curProjectile->x, curProjectile->y, PROJECTILE, i, curProjectile->textureIndex, 0 /* animationFrame*/);
	}
}