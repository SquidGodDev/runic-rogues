#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "helpers.h"
#include "game/map/map.h"
#include "game/enemies/enemies.h"
#include "game/player/player.h"
#include "game/entities/entities.h"
#include "game/entities/projectiles.h"
#include "game/rendering/textures.h"
#include "game/systems/progressionManager.h"
#include "game/systems/upgradeManager.h"

#define HIT_FLASH_TIME 0.1f // Seconds
#define PROJECTILE_OFFSET 3

#define ENEMY_SPAWN_BORDER_SIZE 2 // Border size for enemy spawn area
#define ENEMY_SPAWN_MIN_Y ENEMY_SPAWN_BORDER_SIZE
#define ENEMY_SPAWN_MAX_Y (MAP_HEIGHT - ENEMY_SPAWN_BORDER_SIZE)
#define ENEMY_SPAWN_MIN_X (MAP_WIDTH / 2) // Give space for player to spawn
#define ENEMY_SPAWN_MAX_X (MAP_WIDTH - ENEMY_SPAWN_BORDER_SIZE)

// DEBUG
#define SKIP_ENEMY_UPDATE 0

typedef enum AttackPattern
{
	ATTACK_PATTERN_PROXIMITY,
	ATTACK_PATTERN_MELEE,
	ATTACK_PATTERN_PROJECTILE,
	ATTACK_PATTERN_RUSH
} AttackPattern;

Enemy enemies[MAX_ENEMY_COUNT] = {0};

typedef struct EnemyData {
	char* name;
	int health;
	int attackDetectionRange;
	int attackDistance;
	int attackDamage;
	float attackCooldown;
	float abilityCooldown;
	float moveSpeed;
	int dodgeChance;
	float abilityDataPrimary; // Flexible property for enemy-specific behaviors or traits
	float abilityDataSecondary;
	uint8_t* texture;
	int idleFrameEnd; // Frames are 1-indexed to match Aseprite
	int attackFrameStart;
	int attackFrameEnd;
	int attackActiveFrame;
	int deathFrameStart;
	int deathFrameEnd;
	float frameTime;
	int attackPattern;
} EnemyData;

EnemyData enemyData[ENEMY_TYPE_COUNT];
void initEnemyData(void) {
	enemyData[ENEMY_TYPE_BAT] = (EnemyData){
		.name = "Bat",
		.health = 4,
		.attackDetectionRange = 100,
		.attackDistance = 100,
		.attackDamage = 1,
		.attackCooldown = 0,
		.abilityCooldown = 0,
		.moveSpeed = 0.2f,
		.dodgeChance = 20,
		.texture = TEXTURES[TEXTURE_BAT],
		.idleFrameEnd = 6,
		.deathFrameStart = 7,
		.deathFrameEnd = 10,
		.frameTime = 0.12f,
		.attackPattern = ATTACK_PATTERN_PROXIMITY
	};
	enemyData[ENEMY_TYPE_SLIME] = (EnemyData){
		.name = "Slime",
		.health = 3,
		.attackDetectionRange = 100,
		.attackDistance = 100,
		.attackDamage = 1,
		.attackCooldown = 0,
		.abilityCooldown = 0,
		.moveSpeed = 0.1f,
		.dodgeChance = 20,
		.abilityDataPrimary = 4, // Number of baby slimes to spawn
		.texture = TEXTURES[TEXTURE_SLIME],
		.idleFrameEnd = 6,
		.deathFrameStart = 7,
		.deathFrameEnd = 10,
		.frameTime = 0.14f,
		.attackPattern = ATTACK_PATTERN_PROXIMITY
	};
	enemyData[ENEMY_TYPE_BABY_SLIME] = (EnemyData){
		.name = "Baby Slime",
		.health = 1,
		.attackDetectionRange = 100,
		.attackDistance = 100,
		.attackDamage = 1,
		.attackCooldown = 0,
		.abilityCooldown = 0,
		.moveSpeed = 0.4f,
		.dodgeChance = 0,
		.texture = TEXTURES[TEXTURE_BABY_SLIME],
		.idleFrameEnd = 6,
		.deathFrameStart = 7,
		.deathFrameEnd = 10,
		.frameTime = 0.1f,
		.attackPattern = ATTACK_PATTERN_PROXIMITY
	};
	enemyData[ENEMY_TYPE_SKELETON] = (EnemyData){
		.name = "Skeleton",
		.health = 4,
		.attackDetectionRange = 350,
		.attackDistance = 350,
		.attackDamage = 1,
		.attackCooldown = 0,
		.abilityCooldown = 0,
		.moveSpeed = 0.2f,
		.dodgeChance = 20,
		.texture = TEXTURES[TEXTURE_SKELETON],
		.idleFrameEnd = 6,
		.attackFrameStart = 7,
		.attackFrameEnd = 14,
		.attackActiveFrame = 12,
		.deathFrameStart = 16,
		.deathFrameEnd = 19,
		.frameTime = 0.12f,
		.attackPattern = ATTACK_PATTERN_MELEE
	};
	enemyData[ENEMY_TYPE_IMP] = (EnemyData){
		.name = "Imp",
		.health = 3,
		.attackDetectionRange = 2000,
		.attackDamage = 1,
		.attackCooldown = 8.0f,
		.abilityCooldown = 0,
		.moveSpeed = 0.2f,
		.dodgeChance = 20,
		.abilityDataPrimary = 1.0f * TARGET_FPS, // Projectile speed
		.texture = TEXTURES[TEXTURE_IMP],
		.idleFrameEnd = 6,
		.attackFrameStart = 7,
		.attackFrameEnd = 14,
		.attackActiveFrame = 11,
		.deathFrameStart = 15,
		.deathFrameEnd = 18,
		.frameTime = 0.14f,
		.attackPattern = ATTACK_PATTERN_PROJECTILE
	};
	enemyData[ENEMY_TYPE_FLYING_TOME] = (EnemyData){
		.name = "Flying Tome",
		.health = 3,
		.attackDetectionRange = 2500,
		.attackDistance = 100,
		.attackDamage = 1,
		.attackCooldown = 0.8f,
		.abilityCooldown = 5.0f,
		.moveSpeed = 0.2f,
		.dodgeChance = 20,
		.abilityDataPrimary = 0.7f, // Rush move speed
		.texture = TEXTURES[TEXTURE_FLYING_TIME],
		.idleFrameEnd = 6,
		.attackFrameStart = 7,
		.attackFrameEnd = 11,
		.deathFrameStart = 12,
		.deathFrameEnd = 15,
		.frameTime = 0.1f,
		.attackPattern = ATTACK_PATTERN_RUSH
	};
	enemyData[ENEMY_TYPE_LIVING_ARMOR] = (EnemyData){
		.name = "Living Armor",
		.health = 6,
		.attackDetectionRange = 350,
		.attackDistance = 350,
		.attackDamage = 2,
		.attackCooldown = 0,
		.abilityCooldown = 0,
		.moveSpeed = 0.1f,
		.dodgeChance = 10,
		.texture = TEXTURES[TEXTURE_LIVING_ARMOR],
		.idleFrameEnd = 6,
		.attackFrameStart = 7,
		.attackFrameEnd = 16,
		.attackActiveFrame = 12,
		.deathFrameStart = 17,
		.deathFrameEnd = 20,
		.frameTime = 0.15f,
		.attackPattern = ATTACK_PATTERN_MELEE
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

// Adapted from James Howard's Catacombs of the Damned: https://github.com/jhhoward/Arduboy3D
int pickNewTargetCell(int index, int *targetMapX, int *targetMapY, int targetPlayerOverride) {
	int dodgeChance = enemyData[enemies[index].type].dodgeChance;
	int deltaX = clampInt((playerX / CELL_SIZE) - *targetMapX, -1, 1);
	int deltaY = clampInt((playerY / CELL_SIZE) - *targetMapY, -1, 1);

	if (!targetPlayerOverride) {
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
	}

	return checkTargetCell(index, targetMapX, targetMapY, *targetMapX + deltaX, *targetMapY + deltaY)
		|| checkTargetCell(index, targetMapX, targetMapY, *targetMapX + deltaX, *targetMapY) 
		|| checkTargetCell(index, targetMapX, targetMapY, *targetMapX, *targetMapY + deltaY) 
		|| checkTargetCell(index, targetMapX, targetMapY, *targetMapX - deltaX, *targetMapY + deltaY)
		|| checkTargetCell(index, targetMapX, targetMapY, *targetMapX + deltaX, *targetMapY - deltaY);
}

Enemy moveEnemy(Enemy enemy, int enemyIndex, float moveSpeed, int targetPlayerOverride) {
	int enemyMapX = enemy.x / CELL_SIZE;
	int enemyMapY = enemy.y / CELL_SIZE;
	int targetMapX = enemy.targetMapX;
	int targetMapY = enemy.targetMapY;

	if(targetPlayerOverride || (enemyMapX == targetMapX && enemyMapY == targetMapY))
	{
		pickNewTargetCell(enemyIndex, &targetMapX, &targetMapY, targetPlayerOverride);
		enemy.targetMapX = targetMapX;
		enemy.targetMapY = targetMapY;
	}

	if (isBlocked(targetMapX, targetMapY)) {
		return enemy;
	}

	int targetX = (targetMapX * CELL_SIZE) + CELL_SIZE / 2;
	int targetY = (targetMapY * CELL_SIZE) + CELL_SIZE / 2;

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
		.abilityCooldown = 0,
		.hitFlashTime = 0,
		.animationTime = randFloat(0, curEnemyData.frameTime),
		.animationFrame = randInt(0, curEnemyData.idleFrameEnd - 1),
		.active = 1,
		.state = ENEMY_STATE_IDLE,
		.type = enemyType
	};
}

void spawnRandomEnemy(void) {
	EnemyType *spawnList = getAreaSpawnList();
	EnemyType enemyType = spawnList[randInt(0, ENEMY_TYPES_PER_AREA - 1)];

	int randMapX = randInt(ENEMY_SPAWN_MIN_X, ENEMY_SPAWN_MAX_X);
	int randMapY = randInt(ENEMY_SPAWN_MIN_Y, ENEMY_SPAWN_MAX_Y);

	while (map[randMapY * MAP_WIDTH + randMapX] != 0) {
		randMapX = randInt(ENEMY_SPAWN_MIN_X, ENEMY_SPAWN_MAX_X);
		randMapY = randInt(ENEMY_SPAWN_MIN_Y, ENEMY_SPAWN_MAX_Y);
	}

	spawnEnemy(enemyType, randMapX * CELL_SIZE + CELL_SIZE / 2, randMapY * CELL_SIZE + CELL_SIZE / 2);
}

void initEnemies(void) {
	initEnemyData();

	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		enemies[i].active = 0;
	}
}

void spawnEnemies(RoomType roomType) {
	if (roomType == ROOM_TYPE_ENEMY || roomType == ROOM_TYPE_ELITE) {
		int enemiesSpawned = getCurrentRoom() + 2;
		for (int i = 0; i < enemiesSpawned; i++) {
			spawnRandomEnemy();
		}
	}
}

int playerInAttackDetectionRange(int enemyIndex) {
	Enemy curEnemy = enemies[enemyIndex];
	EnemyData curEnemyData = enemyData[curEnemy.type];
	return distanceSquared(curEnemy.x, curEnemy.y, playerX, playerY) <= curEnemyData.attackDetectionRange;
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

		if (SKIP_ENEMY_UPDATE) {
			addEntityToRenderQueue(curEnemy.x, curEnemy.y, ENEMY, i, curEnemyData.texture, curEnemy.animationFrame);
			continue;
		}

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
			} else if (curEnemy.state == ENEMY_STATE_DEAD) {
				if (curEnemy.animationFrame >= curEnemyData.deathFrameEnd) {
					enemies[i].active = 0;
					continue;
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

		curEnemy.abilityCooldown -= deltaTime;
		if (curEnemy.abilityCooldown <= 0) {
			curEnemy.abilityCooldown = 0;
		}

		if (curEnemy.state == ENEMY_STATE_IDLE) {
			AttackPattern curAttackPattern = curEnemyData.attackPattern;
			if (curEnemy.abilityCooldown <= 0) {
				if (curAttackPattern == ATTACK_PATTERN_RUSH && isClearLine(curEnemy.x, curEnemy.y, playerX, playerY)) {
					curEnemy.abilityCooldown = curEnemyData.abilityCooldown;
					curEnemy.state = ENEMY_STATE_ATTACK;
					curEnemy.animationFrame = curEnemyData.attackFrameStart - 1;
					curEnemy.animationTime = curEnemyData.frameTime;
				}
			}

			if (curEnemy.attackCooldown <= 0 && playerInAttackDetectionRange(i)) {
				curEnemy.attackCooldown = curEnemyData.attackCooldown;
				if (curAttackPattern == ATTACK_PATTERN_MELEE || curAttackPattern == ATTACK_PATTERN_PROJECTILE) {
					curEnemy.state = ENEMY_STATE_ATTACK;
					curEnemy.animationFrame = curEnemyData.attackFrameStart - 1;
					curEnemy.animationTime = curEnemyData.frameTime;
				} else if (curAttackPattern == ATTACK_PATTERN_PROXIMITY || curAttackPattern == ATTACK_PATTERN_RUSH) {
					if (playerInAttackDistance(i)) {
						damagePlayer(curEnemyData.attackDamage);
					}
				}
			}

			curEnemy = moveEnemy(curEnemy, i, curEnemyData.moveSpeed, 0 /*targetPlayerOverride*/);
		} else if (curEnemy.state == ENEMY_STATE_ATTACK) {
			AttackPattern curAttackPattern = curEnemyData.attackPattern;
			if (curAttackPattern == ATTACK_PATTERN_RUSH) {
				curEnemy = moveEnemy(curEnemy, i, curEnemyData.abilityDataPrimary, 1 /*targetPlayerOverride*/);
				if (playerInAttackDistance(i)) {
					damagePlayer(curEnemyData.attackDamage);
				}
			} else if (curEnemy.animationFrame >= curEnemyData.attackActiveFrame) {
				if (curAttackPattern == ATTACK_PATTERN_MELEE) {
					if (playerInAttackDistance(i)) {
						damagePlayer(curEnemyData.attackDamage);
					}
				} else if (curAttackPattern == ATTACK_PATTERN_PROJECTILE) {
					float distanceToPlayer = sqrtf(distanceSquared(playerX, playerY, curEnemy.x, curEnemy.y));
					float deltaX = (playerX - curEnemy.x) / distanceToPlayer;
					float deltaY = (playerY - curEnemy.y) / distanceToPlayer;
					float projectileSpeed = curEnemyData.abilityDataPrimary;
					float damage = curEnemyData.attackDamage;
					float projectileX = curEnemy.x + deltaX * PROJECTILE_OFFSET;
					float projectileY = curEnemy.y + deltaY * PROJECTILE_OFFSET;
					fireProjectile(projectileX, projectileY, deltaX, deltaY, projectileSpeed, damage, TEXTURES[TEXTURE_ENEMY_PROJECTILE], 1 /*isEnemy*/);
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
		int slimeSpawnCount = enemyData[enemyType].abilityDataPrimary;
		for (int i = 0; i < slimeSpawnCount; i++) {
			spawnEnemy(ENEMY_TYPE_BABY_SLIME, curEnemy.x, curEnemy.y);
		}
	}
}

int damageEnemy(int index, int damage) {
	Enemy curEnemy = enemies[index];
	if (curEnemy.state == ENEMY_STATE_DEAD) {
		return 0;
	}

	EnemyData curEnemyData = enemyData[curEnemy.type];
	curEnemy.hitFlashTime = HIT_FLASH_TIME;
	curEnemy.health -= damage;

	if (curEnemy.health <= 0) {
		enemyDeathEffect(index);

		// Check for vampirism upgrade
		if (randInt(0, 100) < getUpgradeValue(UPGRADE_TYPE_VAMPIRISM) * 100) {
			healPlayer(1);
		}

		curEnemy.state = ENEMY_STATE_DEAD;
		curEnemy.animationFrame = curEnemyData.deathFrameStart - 1;
		curEnemy.animationTime = curEnemyData.frameTime;
		curEnemy.hitFlashTime = 0;

		int allEnemiesDead = 1;
		for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
			if (i == index) {
				continue;
			}

			if (enemies[i].active && enemies[i].state != ENEMY_STATE_DEAD) {
				allEnemiesDead = 0;
				break;
			}
		}

		if (allEnemiesDead) {
			roomCompleted();
		}
	}
	
	enemies[index] = curEnemy;
	return 1;
}

int checkEnemyCollision(float x, float y, float collisionRange, int damage) {
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		Enemy curEnemy = enemies[i];
		if (!curEnemy.active) {
			continue;
		}

		float distSquared = distanceSquared(x, y, curEnemy.x, curEnemy.y);
		if (distSquared < collisionRange) {
			return damageEnemy(i, damage);
		}
	}
	return 0;
}