#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "game/player/projectiles.h"
#include "game/map/map.h"
#include "game/player/player.h"
#include "game/rendering/drawables.h"
#include "game/rendering/textures.h"
#include "game/enemies/enemies.h"

#define MAX_PROJECTILE_COUNT 10

typedef struct Projectile {
	float x;
	float y;
	float direction;
	float speed;
	uint32_t active;
} Projectile;

Projectile projectiles[MAX_PROJECTILE_COUNT] = {0};

void initProjectiles(void) {
    // Empty
}

void fireProjectile(void) {
	float projectileSpeed = 5.0f * TARGET_FPS;
	for (int i = 0; i < MAX_PROJECTILE_COUNT; i++) {
		if (!projectiles[i].active) {
			projectiles[i].x = playerX + cosf(playerAngle) * CELL_SIZE;
			projectiles[i].y = playerY + sinf(playerAngle) * CELL_SIZE;
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
		int drawableMapX = (int)((curProjectile.x - xDirection * CELL_SIZE) / CELL_SIZE);
		int drawableMapY = (int)((curProjectile.y - yDirection * CELL_SIZE) / CELL_SIZE);
		int drawableMapIndex = drawableMapY * MAP_WIDTH + drawableMapX;
		if ((drawableMapIndex >= MAP_WIDTH * MAP_HEIGHT) || (map[drawableMapIndex] != 0)) {
			curProjectile.active = 0;
		} else {
			float collisionRange = 5;
			int collided = checkEnemyCollision(curProjectile.x, curProjectile.y, collisionRange);
			if (collided) {
				curProjectile.active = 0;
				projectiles[i] = curProjectile;
				continue;
			}
		}

		float xVelocity = xDirection * curProjectile.speed;
		float yVelocity = yDirection * curProjectile.speed;
		curProjectile.x += xVelocity * deltaTime;
		curProjectile.y += yVelocity * deltaTime;

		projectiles[i] = curProjectile;

		addDrawableToRenderQueue(curProjectile.x, curProjectile.y, PROJECTILE, projectileTexture);
	}
}