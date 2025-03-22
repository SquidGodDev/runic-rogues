#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "helpers.h"
#include "game/enemies/enemies.h"
#include "game/rendering/drawables.h"
#include "game/rendering/textures.h"

#define MAX_ENEMY_COUNT 10

typedef struct Enemy {
	float x;
	float y;
	float direction;
	float speed;
	uint32_t active;
} Enemy;

Enemy enemies[MAX_ENEMY_COUNT] = {0};

void initEnemies(void) {
    enemies[0] = (struct Enemy){2 * CELL_SIZE, 2 * CELL_SIZE, 0, 0, 1};
	enemies[1] = (struct Enemy){8 * CELL_SIZE, 6 * CELL_SIZE, 0, 0, 1};
	enemies[2] = (struct Enemy){8 * CELL_SIZE, 7 * CELL_SIZE, 0, 0, 1};
	enemies[3] = (struct Enemy){8 * CELL_SIZE, 8 * CELL_SIZE, 0, 0, 1};
	enemies[4] = (struct Enemy){8 * CELL_SIZE, 9 * CELL_SIZE, 0, 0, 1};
}

void updateEnemies(float deltaTime) {
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		Enemy curEnemy = enemies[i];
		if (!curEnemy.active) {
			continue;
		}

		addDrawableToRenderQueue(curEnemy.x, curEnemy.y, ENEMY, enemyTexture);
	}
}

int checkEnemyCollision(float x, float y, float collisionRange) {
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		Enemy curEnemy = enemies[i];
		if (!curEnemy.active) {
			continue;
		}

		float distSquared = distanceSquared(x, y, curEnemy.x, curEnemy.y);
		if (distSquared < collisionRange) {
			return 1;
		}
	}
	return 0;
}