#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "helpers.h"
#include "game/map/map.h"
#include "game/enemies/enemies.h"
#include "game/player/player.h"
#include "game/player/playerUI.h"
#include "game/rendering/drawables.h"
#include "game/rendering/textures.h"

#define HIT_FLASH_TIME 0.1f // Seconds
#define ATTACK_SPEED 1.0f // Seconds
#define FRAME_TIME 0.12f // Seconds
#define FRAME_COUNT 6
#define ATTACK_DISTANCE 100
#define ATTACK_DAMAGE 1
#define MOVE_SPEED 0.2f
#define HEALTH 4
#define DODGE_CHANCE 20

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
		if(dodgeChance < DODGE_CHANCE / 2)
		{
			deltaX = -1;
		}
		else if(dodgeChance < DODGE_CHANCE)
		{
			deltaX = 1;
		}
	}
	else if(deltaY == 0)
	{
		if(dodgeChance < DODGE_CHANCE / 2)
		{
			deltaY = -1;
		}
		else if(dodgeChance < DODGE_CHANCE)
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

Enemy moveEnemy(Enemy enemy, int enemyIndex) {
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
		return enemy;
	}

	int targetX = (targetMapX * CELL_SIZE) + CELL_SIZE / 2;
	int targetY = (targetMapY * CELL_SIZE) + CELL_SIZE / 2;
	float deltaX = clampFloat(targetX - enemy.x, -MOVE_SPEED, MOVE_SPEED);
	float deltaY = clampFloat(targetY - enemy.y, -MOVE_SPEED, MOVE_SPEED);

	enemy.y += deltaY;
	enemy.x += deltaX;

	return enemy;
}

void spawnEnemy(void) {
	int newEnemyIndex = -1;
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		if (!enemies[i].active) {
			newEnemyIndex = i;
			break;
		}
	}

	if (newEnemyIndex == -1) {
		return;
	}

	int randMapX = randInt(1, MAP_WIDTH - 2);
	int randMapY = randInt(1, MAP_HEIGHT - 2);
	int playerMapX = playerX / CELL_SIZE;
	int playerMapY = playerY / CELL_SIZE;

	while (map[randMapY * MAP_WIDTH + randMapX] != 0 || (randMapX == playerMapX && randMapY == playerMapY)) {
		randMapX = randInt(1, MAP_WIDTH - 2);
		randMapY = randInt(1, MAP_HEIGHT - 2);
	}

	enemies[newEnemyIndex] = (struct Enemy) {
		randMapX * CELL_SIZE, // x
		randMapY * CELL_SIZE, // y
		randMapX, // targetMapX
		randMapY, // targetMapY
		HEALTH, // health
		0, // attackTime
		0, // hitFlashTime
		randFloat(0, FRAME_TIME), // animationTime
		randInt(0, FRAME_COUNT - 1), // animationFrame
		1  // active
	};
}

void initEnemies(void) {
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		enemies[i].active = 0;
	}

	for (int i = 0; i < 5; i++) {
		spawnEnemy();
	}
}

void updateEnemies(float deltaTime) {
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		if (!enemies[i].active) {
			continue;
		}

		Enemy curEnemy = enemies[i];
		if (curEnemy.attackTime > 0) {
			curEnemy.attackTime -= deltaTime;
			if (curEnemy.attackTime <= 0) {
				curEnemy.attackTime = 0;
			}
		} else {
			curEnemy = moveEnemy(curEnemy, i);
		}
		
		curEnemy.animationTime -= deltaTime;
		if (curEnemy.animationTime <= 0) {
			curEnemy.animationTime = FRAME_TIME;
			curEnemy.animationFrame += 1;
			if (curEnemy.animationFrame >= FRAME_COUNT) {
				curEnemy.animationFrame = 0;
			}
		}

		curEnemy.hitFlashTime -= deltaTime;
		if (curEnemy.hitFlashTime <= 0) {
			curEnemy.hitFlashTime = 0;
		}

		if (distanceSquared(curEnemy.x, curEnemy.y, playerX, playerY) <= ATTACK_DISTANCE) {
			curEnemy.attackTime = ATTACK_SPEED;
			damagePlayer(ATTACK_DAMAGE);
		}

		enemies[i] = curEnemy;

		addDrawableToRenderQueue(curEnemy.x, curEnemy.y, ENEMY, i, enemyTexture);
	}
}

void damageEnemy(int index, int damage) {
	enemies[index].hitFlashTime = HIT_FLASH_TIME;
	enemies[index].health -= damage;
	if (enemies[index].health <= 0) {
		enemies[index].active = 0;
		enemyKilled();
		spawnEnemy();
	}
}

int checkEnemyCollision(float x, float y, float collisionRange, int damage) {
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		Enemy curEnemy = enemies[i];
		if (!curEnemy.active) {
			continue;
		}

		float distSquared = distanceSquared(x, y, curEnemy.x, curEnemy.y);
		if (distSquared < collisionRange) {
			damageEnemy(i, damage);
			return 1;
		}
	}
	return 0;
}