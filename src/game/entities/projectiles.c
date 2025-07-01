#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "helpers.h"
#include "game/entities/projectiles.h"
#include "game/entities/entities.h"
#include "game/map/map.h"
#include "game/player/player.h"
#include "game/rendering/particles.h"
#include "game/enemies/enemies.h"

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
	}
}

void fireProjectile(float x, float y, float deltaX, float deltaY, float speed, int damage, uint8_t *texture, int isEnemy) {
	for (int i = 0; i < MAX_PROJECTILE_COUNT; i++) {
		if (!projectiles[i].active) {
			float projectileX = x, projectileY = y;
			if (isEnemy) {
				projectiles[i].texture = texture;
			} else {
				projectiles[i].texture = texture;
			}
			projectiles[i].x = projectileX;
			projectiles[i].y = projectileY;
			projectiles[i].deltaX = deltaX;
			projectiles[i].deltaY = deltaY;
			projectiles[i].speed = speed;
			projectiles[i].damage = damage;
			projectiles[i].isEnemy = isEnemy;
			projectiles[i].active = 1;
			break;
		}
	}
}

void updateProjectiles(float deltaTime) {
	for (int i = 0; i < MAX_PROJECTILE_COUNT; i++) {
		Projectile curProjectile = projectiles[i];
		if (!curProjectile.active) {
			continue;
		}

		float deltaX = curProjectile.deltaX;
		float deltaY = curProjectile.deltaY;

		int entityMapX = (int)((curProjectile.x + MAP_CHECK_OFFSET_X) / CELL_SIZE);
		int entityMapY = (int)((curProjectile.y + MAP_CHECK_OFFSET_Y) / CELL_SIZE);
		int entityMapIndex = entityMapY * MAP_WIDTH + entityMapX;
		if ((entityMapIndex >= MAP_WIDTH * MAP_HEIGHT) || (map[entityMapIndex] != 0)) {
			curProjectile.active = 0;
			spawnParticle(PARTICLE_TYPE_PROJECTILE_BREAK, curProjectile.x - PARTICLE_OFFSET * deltaX, curProjectile.y - PARTICLE_OFFSET * deltaY, PARTICLE_TYPE_NONE);
		} else {
			int collided = 0;
			if (curProjectile.isEnemy) {
				float distSquared = distanceSquared(curProjectile.x, curProjectile.y, playerX, playerY);
				if (distSquared < ENEMY_COLLISION_RANGE) {
					collided = 1;
					damagePlayer(curProjectile.damage);
				}
			} else {
				collided = checkEnemyCollision(curProjectile.x, curProjectile.y, PLAYER_COLLISION_RANGE, curProjectile.damage);
			}
			if (collided) {
				curProjectile.active = 0;
				projectiles[i] = curProjectile;
				spawnParticle(PARTICLE_TYPE_PROJECTILE_BREAK, curProjectile.x - PARTICLE_OFFSET * deltaX, curProjectile.y - PARTICLE_OFFSET * deltaY, PARTICLE_TYPE_NONE);
				continue;
			}
		}

		float xVelocity = deltaX * curProjectile.speed;
		float yVelocity = deltaY * curProjectile.speed;
		curProjectile.x += xVelocity * deltaTime;
		curProjectile.y += yVelocity * deltaTime;

		projectiles[i] = curProjectile;
		addEntityToRenderQueue(curProjectile.x, curProjectile.y, PROJECTILE, i, curProjectile.texture, 0 /* animationFrame*/);
	}
}