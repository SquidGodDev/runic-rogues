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

Enemy enemies[MAX_ENEMY_COUNT] = {0};

typedef struct EnemyData {
	char* name;
	int health;
	int attackDistance;
	int attackDamage;
	float attackSpeed;
	float moveSpeed;
	int dodgeChance;
	uint8_t* texture;
	int frameCount;
	float frameTime;
} EnemyData;

EnemyData enemyData[ENEMY_TYPE_COUNT] = {
	{
		.name = "Bat",
		.health = 4,
		.attackDistance = 100,
		.attackDamage = 1,
		.attackSpeed = 1.0f,
		.moveSpeed = 0.2f,
		.dodgeChance = 20,
		.texture = batTexture,
		.frameCount = 6,
		.frameTime = 0.12f
	},
	{
		.name = "Bunny",
		.health = 2,
		.attackDistance = 100,
		.attackDamage = 1,
		.attackSpeed = 1.0f,
		.moveSpeed = 0.2f,
		.dodgeChance = 20,
		.texture = bunnyTexture,
		.frameCount = 1,
		.frameTime = 0.12f
	}
};

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
	int dodgeChance = enemyData[enemies[index].type].dodgeChance;
	int deltaX = clampInt((playerX / CELL_SIZE) - *targetMapX, -1, 1);
	int deltaY = clampInt((playerY / CELL_SIZE) - *targetMapY, -1, 1);

	int dodgeValue = randInt(0, 100);
	if(deltaX == 0)
	{
		if(dodgeValue < dodgeChance / 2)
		{
			deltaX = -1;
		}
		else if(dodgeValue < dodgeChance)
		{
			deltaX = 1;
		}
	}
	else if(deltaY == 0)
	{
		if(dodgeValue < dodgeChance / 2)
		{
			deltaY = -1;
		}
		else if(dodgeValue < dodgeChance)
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
	EnemyData curEnemyData = enemyData[enemy.type];
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
	float moveSpeed = curEnemyData.moveSpeed;
	float deltaX = clampFloat(targetX - enemy.x, -moveSpeed, moveSpeed);
	float deltaY = clampFloat(targetY - enemy.y, -moveSpeed, moveSpeed);

	enemy.y += deltaY;
	enemy.x += deltaX;

	return enemy;
}

void spawnEnemy(EnemyType enemyType) {
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

	EnemyData curEnemyData = enemyData[enemyType];

	enemies[newEnemyIndex] = (struct Enemy) {
		.x = randMapX * CELL_SIZE,
		.y = randMapY * CELL_SIZE,
		.targetMapX = randMapX,
		.targetMapY = randMapY,
		.health = curEnemyData.health,
		.attackTime = 0,
		.hitFlashTime = 0,
		.animationTime = randFloat(0, curEnemyData.frameTime),
		.animationFrame = randInt(0, curEnemyData.frameCount - 1),
		.active = 1,
		.type = enemyType
	};
}

void initEnemies(void) {
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		enemies[i].active = 0;
	}

	for (int i = 0; i < 5; i++) {
		spawnEnemy(randInt(0, ENEMY_TYPE_COUNT - 1));
	}
}

void updateEnemies(float deltaTime) {
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		if (!enemies[i].active) {
			continue;
		}

		Enemy curEnemy = enemies[i];
		EnemyData curEnemyData = enemyData[curEnemy.type];
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
			curEnemy.animationTime = curEnemyData.frameTime;
			curEnemy.animationFrame += 1;
			if (curEnemy.animationFrame >= curEnemyData.frameCount) {
				curEnemy.animationFrame = 0;
			}
		}

		curEnemy.hitFlashTime -= deltaTime;
		if (curEnemy.hitFlashTime <= 0) {
			curEnemy.hitFlashTime = 0;
		}

		if (distanceSquared(curEnemy.x, curEnemy.y, playerX, playerY) <= curEnemyData.attackDistance) {
			curEnemy.attackTime = curEnemyData.attackSpeed;
			damagePlayer(curEnemyData.attackDamage);
		}

		enemies[i] = curEnemy;

		addDrawableToRenderQueue(curEnemy.x, curEnemy.y, ENEMY, i, curEnemyData.texture, curEnemy.animationFrame);
	}
}

void damageEnemy(int index, int damage) {
	enemies[index].hitFlashTime = HIT_FLASH_TIME;
	enemies[index].health -= damage;
	if (enemies[index].health <= 0) {
		enemies[index].active = 0;
		enemyKilled();
		spawnEnemy(randInt(0, ENEMY_TYPE_COUNT - 1));
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