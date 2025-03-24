#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "helpers.h"
#include "game/enemies/enemies.h"
#include "game/player/player.h"
#include "game/rendering/drawables.h"
#include "game/rendering/textures.h"

#define HIT_FLASH_TIME 0.1f // Seconds
#define FRAME_TIME 0.12f // Seconds
#define FRAME_COUNT 6

Enemy enemies[MAX_ENEMY_COUNT] = {0};

void initEnemies(void) {
	for (int i = 0; i < 5; i++) {
		enemies[i] = (struct Enemy) {
			8 * CELL_SIZE, // x
			(5 + i) * CELL_SIZE, // y
			0, // direction
			0, // speed
			0, // hitFlashTime
			randFloat(0, FRAME_TIME), // animationTime
			randInt(0, FRAME_COUNT - 1), // animationFrame
			1  // active
		};
	}
}

void updateEnemies(float deltaTime) {
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		Enemy curEnemy = enemies[i];
		if (!curEnemy.active) {
			continue;
		}

		curEnemy.animationTime -= deltaTime;
		if (curEnemy.animationTime <= 0) {
			curEnemy.animationTime += FRAME_TIME;
			curEnemy.animationFrame += 1;
			if (curEnemy.animationFrame >= FRAME_COUNT) {
				curEnemy.animationFrame = 0;
			}
		}

		curEnemy.hitFlashTime -= deltaTime;
		if (curEnemy.hitFlashTime <= 0) {
			curEnemy.hitFlashTime = 0;
		}

		enemies[i] = curEnemy;

		addDrawableToRenderQueue(curEnemy.x, curEnemy.y, ENEMY, i, enemyTexture);
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
			enemies[i].hitFlashTime = HIT_FLASH_TIME;
			return 1;
		}
	}
	return 0;
}