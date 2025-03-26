#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "helpers.h"
#include "game/map/map.h"
#include "game/enemies/enemies.h"
#include "game/player/player.h"
#include "game/rendering/drawables.h"
#include "game/rendering/textures.h"

#define HIT_FLASH_TIME 0.1f // Seconds
#define FRAME_TIME 0.12f // Seconds
#define FRAME_COUNT 6

Enemy enemies[MAX_ENEMY_COUNT] = {0};

int checkTargetCell(int index, int *targetMapX, int *targetMapY, int newX, int newY) {
	if (isBlocked(newX, newY)) {
		return 0;
	} else if (isBlocked(*targetMapX, newY)) {
		return 0;
	} else if (isBlocked(newX, *targetMapY)) {
		return 0;
	}

	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		Enemy otherEnemy = enemies[i];
		if (i == index || !otherEnemy.active) {
			continue;
		}

		if (otherEnemy.targetMapX == newX && otherEnemy.targetMapY == newY) {
			return 0;
		}
	}

	*targetMapX = newX;
	*targetMapY = newY;

	return 1;
}

int pickNewTargetCell(int index, int *targetMapX, int *targetMapY) {
	int deltaX = clampInt((playerX / CELL_SIZE) - *targetMapX, -1, 1);
	int deltaY = clampInt((playerY / CELL_SIZE) - *targetMapY, -1, 1);

	int dodgeChance = randInt(0, 100);
	if(deltaX == 0)
	{
		if(dodgeChance < 20)
		{
			deltaX = -1;
		}
		else if(dodgeChance < 40)
		{
			deltaX = 1;
		}
	}
	else if(deltaY == 0)
	{
		if(dodgeChance < 20)
		{
			deltaY = -1;
		}
		else if(dodgeChance < 40)
		{
			deltaY = 1;
		}
	}

	return checkTargetCell(index, targetMapX, targetMapY, *targetMapX + deltaX, *targetMapY + deltaY)
		|| checkTargetCell(index, targetMapX, targetMapY, *targetMapX + deltaX, *targetMapY) 
		|| checkTargetCell(index, targetMapX, targetMapY, *targetMapX, *targetMapY + deltaY) 
		|| checkTargetCell(index, targetMapX, targetMapY, *targetMapX - deltaX, *targetMapY + deltaY)
		|| checkTargetCell(index, targetMapX, targetMapY, *targetMapX + deltaX, *targetMapY - deltaY);
}

void moveEnemy(int enemyIndex) {
	Enemy enemy = enemies[enemyIndex];
	int enemyMapX = enemy.x / CELL_SIZE;
	int enemyMapY = enemy.y / CELL_SIZE;
	int targetMapX = enemy.targetMapX;
	int targetMapY = enemy.targetMapY;
	if(enemyMapX == targetMapX && enemyMapY == targetMapY)
	{
		pickNewTargetCell(enemyIndex, &targetMapX, &targetMapY);
		enemy.targetMapX = targetMapX;
		enemy.targetMapY = targetMapY;
	}

	if (isBlocked(targetMapX, targetMapY)) {
		return;
	}

	float moveSpeed = 0.2f;
	int targetX = (targetMapX * CELL_SIZE) + CELL_SIZE / 2;
	int targetY = (targetMapY * CELL_SIZE) + CELL_SIZE / 2;
	float deltaX = clampFloat(targetX - enemy.x, -moveSpeed, moveSpeed);
	float deltaY = clampFloat(targetY - enemy.y, -moveSpeed, moveSpeed);

	enemy.y += deltaY;
	enemy.x += deltaX;

	enemies[enemyIndex] = enemy;
}

void initEnemies(void) {
	for (int i = 0; i < 5; i++) {
		int targetX = 15;
		int targetY = 5 + i;
		enemies[i] = (struct Enemy) {
			targetX * CELL_SIZE, // x
			targetY * CELL_SIZE, // y
			targetX, // targetMapX
			targetY, // targetMapY
			0, // hitFlashTime
			randFloat(0, FRAME_TIME), // animationTime
			randInt(0, FRAME_COUNT - 1), // animationFrame
			1  // active
		};
	}
}

void updateEnemies(float deltaTime) {
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		if (!enemies[i].active) {
			continue;
		}

		moveEnemy(i);

		Enemy curEnemy = enemies[i];
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