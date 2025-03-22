#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "helpers.h"
#include "game/player/player.h"
#include "game/enemies/enemies.h"
#include "game/rendering/drawables.h"
#include "game/rendering/textures.h"

#define MAX_ENEMY_COUNT 10
#define HIT_FLASH_TIME 0.2f // Seconds

typedef struct Enemy {
	float x;
	float y;
	float direction;
	float speed;
	float hitFlash;
	uint32_t active;
} Enemy;

Enemy enemies[MAX_ENEMY_COUNT] = {0};

void initEnemies(void) {
    enemies[0] = (struct Enemy){2 * CELL_SIZE, 2 * CELL_SIZE, 0, 0, 0, 1};
	enemies[1] = (struct Enemy){8 * CELL_SIZE, 6 * CELL_SIZE, 0, 0, 0, 1};
	enemies[2] = (struct Enemy){8 * CELL_SIZE, 7 * CELL_SIZE, 0, 0, 0, 1};
	enemies[3] = (struct Enemy){8 * CELL_SIZE, 8 * CELL_SIZE, 0, 0, 0, 1};
	enemies[4] = (struct Enemy){8 * CELL_SIZE, 9 * CELL_SIZE, 0, 0, 0, 1};
}

void updateEnemies(float deltaTime) {
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		Enemy curEnemy = enemies[i];
		if (!curEnemy.active) {
			continue;
		}

		float hitFlashTime = curEnemy.hitFlash;
		int isHitFlash = hitFlashTime > 0;
		if (hitFlashTime > 0) {
			hitFlashTime -= deltaTime;
		}
		if (hitFlashTime <= 0) {
			hitFlashTime = 0;
		}
		enemies[i].hitFlash = hitFlashTime;

		addDrawableToRenderQueue(curEnemy.x, curEnemy.y, ENEMY, enemyTexture, isHitFlash);
	}
}

int checkEnemyCollision(float x, float y, float collisionRange) {
	// float distToPlayer = distanceSquared(x, y, playerX, playerY);
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		Enemy curEnemy = enemies[i];
		if (!curEnemy.active) {
			continue;
		}

		// float enemyDistToPlayer = distanceSquared(curEnemy.x, curEnemy.y, playerX, playerY);
		float distSquared = distanceSquared(x, y, curEnemy.x, curEnemy.y);
		if (distSquared < collisionRange) {
			enemies[i].hitFlash = HIT_FLASH_TIME;
			return 1;
		}
	}
	return 0;
}