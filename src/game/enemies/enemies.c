#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "helpers.h"
#include "game/map/map.h"
#include "game/enemies/enemies.h"
#include "game/player/player.h"
#include "game/player/playerUI.h"
#include "game/entities/entities.h"
#include "game/entities/projectiles.h"
#include "game/rendering/textures.h"

#define HIT_FLASH_TIME 0.1f // Seconds
#define PROJECTILE_OFFSET 3

typedef enum AttackPattern
{
	ATTACK_PATTERN_PROXIMITY,
	ATTACK_PATTERN_MELEE,
	ATTACK_PATTERN_PROJECTILE
} AttackPattern;

Enemy enemies[MAX_ENEMY_COUNT] = {0};

typedef struct EnemyData {
	char* name;
	int health;
	int attackDistance;
	int attackDamage;
	float attackCooldown;
	float moveSpeed;
	int dodgeChance;
	float abilityData; // Flexible property for enemy-specific behaviors or traits
	uint8_t* texture;
	int idleFrameEnd;
	int attackFrameStart;
	int attackFrameEnd;
	int attackActiveFrame;
	float frameTime;
	int attackPattern;
} EnemyData;

EnemyData enemyData[ENEMY_TYPE_COUNT];
void initEnemyData(void) {
	enemyData[ENEMY_TYPE_BAT] = (EnemyData){
		.name = "Bat",
		.health = 4,
		.attackDistance = 100,
		.attackDamage = 1,
		.attackCooldown = 0,
		.moveSpeed = 0.2f,
		.dodgeChance = 20,
		.texture = batTexture,
		.idleFrameEnd = 6,
		.frameTime = 0.12f,
		.attackPattern = ATTACK_PATTERN_PROXIMITY
	};
	enemyData[ENEMY_TYPE_SLIME] = (EnemyData){
		.name = "Slime",
		.health = 3,
		.attackDistance = 100,
		.attackDamage = 1,
		.attackCooldown = 0,
		.moveSpeed = 0.1f,
		.dodgeChance = 20,
		.abilityData = 4, // Number of baby slimes to spawn
		.texture = slimeTexture,
		.idleFrameEnd = 6,
		.frameTime = 0.14f,
		.attackPattern = ATTACK_PATTERN_PROXIMITY
	};
	enemyData[ENEMY_TYPE_BABY_SLIME] = (EnemyData){
		.name = "Baby Slime",
		.health = 1,
		.attackDistance = 100,
		.attackDamage = 1,
		.attackCooldown = 0,
		.moveSpeed = 0.2f,
		.dodgeChance = 0,
		.texture = babySlimeTexture,
		.idleFrameEnd = 6,
		.frameTime = 0.1f,
		.attackPattern = ATTACK_PATTERN_PROXIMITY
	};
	enemyData[ENEMY_TYPE_SKELETON] = (EnemyData){
		.name = "Skeleton",
		.health = 4,
		.attackDistance = 350,
		.attackDamage = 1,
		.attackCooldown = 0,
		.moveSpeed = 0.2f,
		.dodgeChance = 20,
		.texture = skeletonTexture,
		.idleFrameEnd = 6,
		.attackFrameStart = 7,
		.attackFrameEnd = 14,
		.attackActiveFrame = 12,
		.frameTime = 0.12f,
		.attackPattern = ATTACK_PATTERN_MELEE
	};
	enemyData[ENEMY_TYPE_IMP] = (EnemyData){
		.name = "Imp",
		.health = 3,
		.attackDistance = 2000,
		.attackDamage = 1,
		.attackCooldown = 8.0f,
		.moveSpeed = 0.2f,
		.dodgeChance = 20,
		.abilityData = 1.0f * TARGET_FPS, // Projectile speed
		.texture = impTexture,
		.idleFrameEnd = 6,
		.attackFrameStart = 7,
		.attackFrameEnd = 14,
		.attackActiveFrame = 11,
		.frameTime = 0.14f,
		.attackPattern = ATTACK_PATTERN_PROJECTILE
	};
}

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

void spawnEnemy(EnemyType enemyType, int x, int y) {
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

	EnemyData curEnemyData = enemyData[enemyType];

	enemies[newEnemyIndex] = (struct Enemy) {
		.x = x,
		.y = y,
		.targetMapX = x / CELL_SIZE,
		.targetMapY = y / CELL_SIZE,
		.health = curEnemyData.health,
		.attackCooldown = 0,
		.hitFlashTime = 0,
		.animationTime = randFloat(0, curEnemyData.frameTime),
		.animationFrame = randInt(0, curEnemyData.idleFrameEnd - 1),
		.active = 1,
		.state = ENEMY_STATE_IDLE,
		.type = enemyType
	};
}

void spawnRandomEnemy(void) {
	EnemyType spawnList[] = {
		ENEMY_TYPE_BAT,
		ENEMY_TYPE_SKELETON,
		ENEMY_TYPE_SLIME,
		ENEMY_TYPE_IMP
	};
	int spawnListLength = *(&spawnList + 1) - spawnList;
	EnemyType enemyType = spawnList[randInt(0, spawnListLength - 1)];
	// EnemyType enemyType = ENEMY_TYPE_IMP;

	int randMapX = randInt(1, MAP_WIDTH - 2);
	int randMapY = randInt(1, MAP_HEIGHT - 2);
	int playerMapX = playerX / CELL_SIZE;
	int playerMapY = playerY / CELL_SIZE;

	while (map[randMapY * MAP_WIDTH + randMapX] != 0 || (randMapX == playerMapX && randMapY == playerMapY)) {
		randMapX = randInt(1, MAP_WIDTH - 2);
		randMapY = randInt(1, MAP_HEIGHT - 2);
	}

	spawnEnemy(enemyType, randMapX * CELL_SIZE, randMapY * CELL_SIZE);
}

void initEnemies(void) {
	initEnemyData();

	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		enemies[i].active = 0;
	}

	for (int i = 0; i < 5; i++) {
		spawnRandomEnemy();
	}
}

int playerInAttackDistance(int enemyIndex) {
	Enemy curEnemy = enemies[enemyIndex];
	EnemyData curEnemyData = enemyData[curEnemy.type];
	return distanceSquared(curEnemy.x, curEnemy.y, playerX, playerY) <= curEnemyData.attackDistance;
}

void updateEnemies(float deltaTime) {
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		if (!enemies[i].active) {
			continue;
		}

		Enemy curEnemy = enemies[i];
		EnemyData curEnemyData = enemyData[curEnemy.type];

		curEnemy.animationTime -= deltaTime;
		if (curEnemy.animationTime <= 0) {
			curEnemy.animationTime = curEnemyData.frameTime;
			curEnemy.animationFrame += 1;
			if (curEnemy.state == ENEMY_STATE_IDLE) {
				if (curEnemy.animationFrame >= curEnemyData.idleFrameEnd) {
					curEnemy.animationFrame = 0;
				}
			} else if (curEnemy.state == ENEMY_STATE_ATTACK || curEnemy.state == ENEMY_STATE_ATTACK_END) {
				if (curEnemy.animationFrame >= curEnemyData.attackFrameEnd) {
					curEnemy.animationFrame = 0;
					curEnemy.state = ENEMY_STATE_IDLE;
				}
			}
		}

		curEnemy.hitFlashTime -= deltaTime;
		if (curEnemy.hitFlashTime <= 0) {
			curEnemy.hitFlashTime = 0;
		}

		curEnemy.attackCooldown -= deltaTime;
		if (curEnemy.attackCooldown <= 0) {
			curEnemy.attackCooldown = 0;
		}

		if (curEnemy.state == ENEMY_STATE_IDLE) {
			if (curEnemy.attackCooldown <= 0 && playerInAttackDistance(i)) {
				curEnemy.attackCooldown = curEnemyData.attackCooldown;
				if (curEnemyData.attackPattern == ATTACK_PATTERN_PROXIMITY) {
					damagePlayer(curEnemyData.attackDamage);
				} else if (curEnemyData.attackPattern == ATTACK_PATTERN_MELEE) {
					curEnemy.state = ENEMY_STATE_ATTACK;
					curEnemy.animationFrame = curEnemyData.attackFrameStart - 1;
					curEnemy.animationTime = curEnemyData.frameTime;
				} else if (curEnemyData.attackPattern == ATTACK_PATTERN_PROJECTILE) {
					curEnemy.state = ENEMY_STATE_ATTACK;
					curEnemy.animationFrame = curEnemyData.attackFrameStart - 1;
					curEnemy.animationTime = curEnemyData.frameTime;
				}
			}
			curEnemy = moveEnemy(curEnemy, i);
		} else if (curEnemy.state == ENEMY_STATE_ATTACK) {
			if (curEnemy.animationFrame >= curEnemyData.attackActiveFrame) {
				if (curEnemyData.attackPattern == ATTACK_PATTERN_MELEE) {
					if (playerInAttackDistance(i)) {
						damagePlayer(curEnemyData.attackDamage);
					}
				} else if (curEnemyData.attackPattern == ATTACK_PATTERN_PROJECTILE) {
					float distanceToPlayer = sqrtf(distanceSquared(playerX, playerY, curEnemy.x, curEnemy.y));
					float deltaX = (playerX - curEnemy.x) / distanceToPlayer;
					float deltaY = (playerY - curEnemy.y) / distanceToPlayer;
					float projectileSpeed = curEnemyData.abilityData;
					float damage = curEnemyData.attackDamage;
					float projectileX = curEnemy.x + deltaX * PROJECTILE_OFFSET;
					float projectileY = curEnemy.y + deltaY * PROJECTILE_OFFSET;
					fireProjectile(projectileX, projectileY, deltaX, deltaY, projectileSpeed, damage, enemyProjectileTexture, 1 /*isEnemy*/);
				}
				curEnemy.state = ENEMY_STATE_ATTACK_END;
			}
		}

		enemies[i] = curEnemy;

		addEntityToRenderQueue(curEnemy.x, curEnemy.y, ENEMY, i, curEnemyData.texture, curEnemy.animationFrame);
	}
}

void enemyDeathEffect(int index) {
	Enemy curEnemy = enemies[index];
	EnemyType enemyType = curEnemy.type;
	if (enemyType == ENEMY_TYPE_SLIME) {
		int slimeSpawnCount = enemyData[enemyType].abilityData;
		for (int i = 0; i < slimeSpawnCount; i++) {
			spawnEnemy(ENEMY_TYPE_BABY_SLIME, curEnemy.x, curEnemy.y);
		}
	}
}

void damageEnemy(int index, int damage) {
	enemies[index].hitFlashTime = HIT_FLASH_TIME;
	enemies[index].health -= damage;
	if (enemies[index].health <= 0) {
		enemyDeathEffect(index);

		enemies[index].active = 0;
		// Temp enemy respawn
		if (enemies[index].type != ENEMY_TYPE_BABY_SLIME) {
			spawnRandomEnemy();
		}
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