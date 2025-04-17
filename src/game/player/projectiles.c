#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "game/player/projectiles.h"
#include "game/map/map.h"
#include "game/player/player.h"
#include "game/rendering/drawables.h"
#include "game/rendering/particles.h"
#include "game/rendering/textures.h"
#include "game/enemies/enemies.h"

#define MAX_PROJECTILE_COUNT 10
// Spawn the projectile slightly to the right to match hand
#define FIRE_ANGLE_OFFSET PI / 8
#define FIRE_POSITION_OFFSET CELL_SIZE / 2
#define MAP_CHECK_OFFSET_X cosf(FIRE_ANGLE_OFFSET) * FIRE_POSITION_OFFSET / 4
#define MAP_CHECK_OFFSET_Y sinf(FIRE_ANGLE_OFFSET) * FIRE_POSITION_OFFSET / 4

#define PROJECTILE_DAMAGE 1
#define COLLISION_RANGE 30
#define PARTICLE_OFFSET CELL_SIZE / 2

typedef struct Projectile {
	float x;
	float y;
	float direction;
	float speed;
	uint32_t active;
} Projectile;

Projectile projectiles[MAX_PROJECTILE_COUNT] = {0};

void initProjectiles(void) {
    for (int i = 0; i < MAX_PROJECTILE_COUNT; i++) {
		projectiles[i].active = 0;
	}
}

void fireProjectile(void) {
	float projectileSpeed = 7.0f * TARGET_FPS;
	for (int i = 0; i < MAX_PROJECTILE_COUNT; i++) {
		if (!projectiles[i].active) {
			projectiles[i].x = playerX + cosf(playerAngle + FIRE_ANGLE_OFFSET) * FIRE_POSITION_OFFSET;
			projectiles[i].y = playerY + sinf(playerAngle + FIRE_ANGLE_OFFSET) * FIRE_POSITION_OFFSET;
			projectiles[i].direction = playerAngle;
			projectiles[i].speed = projectileSpeed;
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

		float xDirection = cosf(curProjectile.direction);
		float yDirection = sinf(curProjectile.direction);
		int drawableMapX = (int)((curProjectile.x + MAP_CHECK_OFFSET_X) / CELL_SIZE);
		int drawableMapY = (int)((curProjectile.y + MAP_CHECK_OFFSET_Y) / CELL_SIZE);
		int drawableMapIndex = drawableMapY * MAP_WIDTH + drawableMapX;
		if ((drawableMapIndex >= MAP_WIDTH * MAP_HEIGHT) || (map[drawableMapIndex] != 0)) {
			curProjectile.active = 0;
			spawnParticle(curProjectile.x - PARTICLE_OFFSET * xDirection, curProjectile.y - PARTICLE_OFFSET * yDirection);
		} else {
			int collided = checkEnemyCollision(curProjectile.x, curProjectile.y, COLLISION_RANGE, PROJECTILE_DAMAGE);
			if (collided) {
				curProjectile.active = 0;
				projectiles[i] = curProjectile;
				spawnParticle(curProjectile.x - PARTICLE_OFFSET * xDirection, curProjectile.y - PARTICLE_OFFSET * yDirection);
				continue;
			}
		}

		float xVelocity = xDirection * curProjectile.speed;
		float yVelocity = yDirection * curProjectile.speed;
		curProjectile.x += xVelocity * deltaTime;
		curProjectile.y += yVelocity * deltaTime;

		projectiles[i] = curProjectile;

		addDrawableToRenderQueue(curProjectile.x, curProjectile.y, PROJECTILE, i, projectileTexture, 0 /* animationFrame*/);
	}
}