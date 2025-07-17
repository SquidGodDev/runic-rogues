#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "game/map/map.h"
#include "game/enemies/enemies.h"
#include "game/player/player.h"
#include "game/entities/entities.h"
#include "game/entities/projectiles.h"
#include "game/rendering/textures.h"
#include "game/rendering/particles.h"
#include "game/systems/audioManager.h"
#include "game/systems/progressionManager.h"
#include "game/systems/upgradeManager.h"

#define SPAWN_TEST_ENEMY 0

#define HIT_FLASH_TIME 0.1f // Seconds
#define PROJECTILE_OFFSET 3

#define ENEMY_SPAWN_BORDER_SIZE 2 // Border size for enemy spawn area
#define ENEMY_SPAWN_MIN_Y ENEMY_SPAWN_BORDER_SIZE
#define ENEMY_SPAWN_MAX_Y (MAP_HEIGHT - ENEMY_SPAWN_BORDER_SIZE)
#define ENEMY_SPAWN_MIN_X (MAP_WIDTH / 2) // Give space for player to spawn
#define ENEMY_SPAWN_MAX_X (MAP_WIDTH - ENEMY_SPAWN_BORDER_SIZE)

#define MIN_DIST_TO_PLAYER 5
#define ENEMY_SEPARATION_RADIUS 20
#define ENEMY_SEPARATION_FORCE 0.1f
#define WALL_CHECK_DISTANCE 4
#define WALL_AVOID_FORCE 0.1f

#define ELDER_BOSS_IDLE_TIME 4.0f
#define ELDER_BOSS_PREP_TIME 2.0f
#define ELDER_BOSS_ATTACK2_COUNT 5
#define ELDER_BOSS_ATTACK2_SPACING CELL_SIZE

#define WRAITH_BOSS_IDLE_TIME 2.0f
#define WRAITH_BOSS_PREP_TIME 1.0f
#define WRAITH_BOSS_ATTACK1_TIME 2.5f
#define WRAITH_BOSS_ATTACK2_TIME 2.0f
#define WRAITH_BOSS_ATTACK3_TIME 5.0f
#define WRAITH_BOSS_RUSH_COUNT 3
#define WRAITH_BOSS_PROJECTILE_COOLDOWN 0.5f

// DEBUG
#define SKIP_ENEMY_UPDATE 0

typedef enum AttackPattern
{
	ATTACK_PATTERN_PROXIMITY,
	ATTACK_PATTERN_MELEE,
	ATTACK_PATTERN_PROJECTILE,
	ATTACK_PATTERN_RUSH,
	ATTACK_PATTERN_ELDER_BOSS,
	ATTACK_PATTERN_WRAITH_BOSS
} AttackPattern;

typedef enum BossState {
	BOSS_STATE_IDLE,
	BOSS_STATE_PREPARING,
	BOSS_STATE_ATTACK1,
	BOSS_STATE_ATTACK2,
	BOSS_STATE_ATTACK3
} BossState;

typedef struct Boss {
	BossState bossState;
	BossState nextAttackState;
	int stateCount;
	float stateTimer;
	float stateData;
} Boss;

Enemy enemies[MAX_ENEMY_COUNT] = {0};
static int bossIndex;
static Boss boss = {0};

typedef struct EnemyData {
	char* name;
	int health;
	int attackDetectionRange;
	int attackDistance;
	int attackDamage;
	float attackCooldown;
	float moveSpeed;
	float abilityDataPrimary; // Flexible property for enemy-specific behaviors or traits
	float abilityDataSecondary; // Flexible property for enemy-specific behaviors or traits
	int textureIndex;
	int idleFrameEnd; // Frames are 1-indexed to match Aseprite
	int prepFrameStart;
	int prepFrameEnd;
	int attackFrameStart;
	int attackFrameEnd;
	int attackActiveFrame;
	int deathFrameStart;
	int deathFrameEnd;
	float frameTime;
	int attackPattern;
} EnemyData;

EnemyData enemyData[ENEMY_TYPE_COUNT] = {
	[ENEMY_TYPE_TEST] = (EnemyData){
		.name = "Test",
		.health = 4,
		.attackDetectionRange = 100,
		.attackDistance = 100,
		.attackDamage = 1,
		.attackCooldown = 1.0f,
		.moveSpeed = 0.15f,
		.textureIndex = TEXTURE_WRAITH_BOSS,
		.idleFrameEnd = 6,
		.deathFrameStart = 1,
		.deathFrameEnd = 1,
		.frameTime = 0.15f,
		.attackPattern = ATTACK_PATTERN_PROXIMITY
	},
	[ENEMY_TYPE_BAT] = (EnemyData){
		.name = "Bat",
		.health = 5,
		.attackDetectionRange = 3000,
		.attackDistance = 100,
		.attackDamage = 1,
		.attackCooldown = 4.0f,
		.moveSpeed = 0.3f,
		.abilityDataPrimary = 0.7f, // Rush move speed
		.abilityDataSecondary = 4.0f, // Rush duration
		.textureIndex = TEXTURE_BAT,
		.idleFrameEnd = 6,
		.attackFrameStart = 1,
		.attackFrameEnd = 6,
		.deathFrameStart = 7,
		.deathFrameEnd = 10,
		.frameTime = 0.12f,
		.attackPattern = ATTACK_PATTERN_RUSH
	},
	[ENEMY_TYPE_SLIME] = (EnemyData){
		.name = "Slime",
		.health = 5,
		.attackDetectionRange = 100,
		.attackDistance = 100,
		.attackDamage = 1,
		.attackCooldown = 0,
		.moveSpeed = 0.1f,
		.abilityDataPrimary = 4, // Number of baby slimes to spawn
		.textureIndex = TEXTURE_SLIME,
		.idleFrameEnd = 6,
		.deathFrameStart = 7,
		.deathFrameEnd = 10,
		.frameTime = 0.14f,
		.attackPattern = ATTACK_PATTERN_PROXIMITY
	},
	[ENEMY_TYPE_BABY_SLIME] = (EnemyData){
		.name = "Baby Slime",
		.health = 1,
		.attackDetectionRange = 100,
		.attackDistance = 100,
		.attackDamage = 1,
		.attackCooldown = 0,
		.moveSpeed = 0.5f,
		.textureIndex = TEXTURE_BABY_SLIME,
		.idleFrameEnd = 6,
		.deathFrameStart = 7,
		.deathFrameEnd = 10,
		.frameTime = 0.1f,
		.attackPattern = ATTACK_PATTERN_PROXIMITY
	},
	[ENEMY_TYPE_SKELETON] = (EnemyData){
		.name = "Skeleton",
		.health = 7,
		.attackDetectionRange = 350,
		.attackDistance = 350,
		.attackDamage = 1,
		.attackCooldown = 0,
		.moveSpeed = 0.2f,
		.textureIndex = TEXTURE_SKELETON,
		.idleFrameEnd = 6,
		.attackFrameStart = 7,
		.attackFrameEnd = 14,
		.attackActiveFrame = 12,
		.deathFrameStart = 16,
		.deathFrameEnd = 19,
		.frameTime = 0.12f,
		.attackPattern = ATTACK_PATTERN_MELEE
	},
	[ENEMY_TYPE_ELDER_BOSS] = (EnemyData){
		.name = "Elder Boss",
		.health = 65,
		.attackDetectionRange = 350,
		.attackDistance = 350,
		.attackDamage = 2,
		.attackCooldown = 1.5f,
		.moveSpeed = 0.2f,
		.textureIndex = TEXTURE_ELDER_BOSS,
		.idleFrameEnd = 14,
		.prepFrameStart = 15,
		.prepFrameEnd = 17,
		.attackFrameStart = 18,
		.attackFrameEnd = 26,
		.attackActiveFrame = 22,
		.deathFrameStart = 27,
		.deathFrameEnd = 30,
		.frameTime = 0.15f,
		.attackPattern = ATTACK_PATTERN_ELDER_BOSS
	},
	[ENEMY_TYPE_IMP] = (EnemyData){
		.name = "Imp",
		.health = 8,
		.attackDetectionRange = 2000,
		.attackDamage = 1,
		.attackCooldown = 8.0f,
		.moveSpeed = 0.4f,
		.abilityDataPrimary = 50.0f, // Projectile speed
		.abilityDataSecondary = 1.0f, // Random movement cooldown
		.textureIndex = TEXTURE_IMP,
		.idleFrameEnd = 6,
		.attackFrameStart = 7,
		.attackFrameEnd = 14,
		.attackActiveFrame = 11,
		.deathFrameStart = 15,
		.deathFrameEnd = 18,
		.frameTime = 0.14f,
		.attackPattern = ATTACK_PATTERN_PROJECTILE
	},
	[ENEMY_TYPE_FLYING_TOME] = (EnemyData){
		.name = "Flying Tome",
		.health = 10,
		.attackDetectionRange = 3000,
		.attackDistance = 100,
		.attackDamage = 1,
		.attackCooldown = 5.0f,
		.moveSpeed = 0.2f,
		.abilityDataPrimary = 1.2f, // Rush move speed
		.abilityDataSecondary = 3.0f, // Rush duration
		.textureIndex = TEXTURE_FLYING_TOME,
		.idleFrameEnd = 6,
		.attackFrameStart = 7,
		.attackFrameEnd = 11,
		.deathFrameStart = 12,
		.deathFrameEnd = 15,
		.frameTime = 0.1f,
		.attackPattern = ATTACK_PATTERN_RUSH
	},
	[ENEMY_TYPE_LIVING_ARMOR] = (EnemyData){
		.name = "Living Armor",
		.health = 20,
		.attackDetectionRange = 500,
		.attackDistance = 600,
		.attackDamage = 2,
		.attackCooldown = 0,
		.moveSpeed = 0.4f,
		.textureIndex = TEXTURE_LIVING_ARMOR,
		.idleFrameEnd = 6,
		.attackFrameStart = 7,
		.attackFrameEnd = 16,
		.attackActiveFrame = 12,
		.deathFrameStart = 17,
		.deathFrameEnd = 20,
		.frameTime = 0.1f,
		.attackPattern = ATTACK_PATTERN_MELEE
	},
	[ENEMY_TYPE_WRAITH_BOSS] = (EnemyData){
		.name = "Wraith Boss",
		.health = 90,
		.attackDetectionRange = 100,
		.attackDistance = 100,
		.attackDamage = 2,
		.attackCooldown = 1.5f,
		.moveSpeed = 0.25f,
		.abilityDataPrimary = 1.2f, // Rush move speed
		.abilityDataSecondary = 70.0f, // Projectile speed
		.textureIndex = TEXTURE_WRAITH_BOSS,
		.idleFrameEnd = 6,
		.prepFrameStart = 7,
		.prepFrameEnd = 10,
		.deathFrameStart = 11,
		.deathFrameEnd = 14,
		.frameTime = 0.15f,
		.attackPattern = ATTACK_PATTERN_WRAITH_BOSS
	},
	[ENEMY_TYPE_WRAITH] = (EnemyData){
		.name = "Wraith",
		.health = 4,
		.attackDetectionRange = 100,
		.attackDistance = 100,
		.attackDamage = 1,
		.attackCooldown = 1.0f,
		.moveSpeed = 0.6f,
		.textureIndex = TEXTURE_WRAITH,
		.idleFrameEnd = 4,
		.deathFrameStart = 5,
		.deathFrameEnd = 8,
		.frameTime = 0.15f,
		.attackPattern = ATTACK_PATTERN_PROXIMITY
	}
};

void moveEnemy(Enemy *enemy, int enemyIndex, float targetX, float targetY, float moveSpeed) {
	float dx = (targetX - enemy->x);
	float dy = (targetY - enemy->y);
	float distToTarget = sqrtf(dx*dx + dy*dy);

	if (distToTarget < MIN_DIST_TO_PLAYER) {
		return;	
	}

	dx /= distToTarget;
	dy /= distToTarget;

	// Repulsion from other enemies
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		if (i == enemyIndex || !enemies[i].active) continue;
		Enemy other = enemies[i];

		float otherX = enemy->x - other.x;
		float otherY = enemy->y - other.y;
		float distSquared = otherX*otherX + otherY*otherY;

		if (distSquared < ENEMY_SEPARATION_RADIUS * ENEMY_SEPARATION_RADIUS && distSquared > 0.01f) {
			float dist = sqrtf(distSquared);
			otherX /= dist;
			otherY /= dist;
			float force = ENEMY_SEPARATION_FORCE * (1.0f - dist / ENEMY_SEPARATION_RADIUS);
			dx += otherX * force;
			dy += otherY * force;
		}
	}

	// Wall avoidance
	int frontX = (int)((enemy->x + dx * WALL_CHECK_DISTANCE) / CELL_SIZE);
	int frontY = (int)((enemy->y + dy * WALL_CHECK_DISTANCE) / CELL_SIZE);
	if (isBlocked(frontX, frontY)) {
		dx = -dx * WALL_AVOID_FORCE;
		dy = -dy * WALL_AVOID_FORCE;
	}

	// Apply slow
	if (enemy->slowTime > 0) {
		moveSpeed *= 1.0f - enemy->slowAmount;
	}

	// Normalize final direction vector
	float magnitude = sqrtf(dx*dx + dy*dy);
	if (magnitude > 0.01f) {
		dx = (dx / magnitude) * moveSpeed;
		dy = (dy / magnitude) * moveSpeed;
	}

	// Apply movement
	enemy->x += dx;
	enemy->y += dy;
}

int spawnEnemy(EnemyType enemyType, int x, int y) {
	int newEnemyIndex = -1;
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		if (!enemies[i].active) {
			newEnemyIndex = i;
			break;
		}
	}

	if (newEnemyIndex == -1) {
		return -1;
	}

	if (enemyType == ENEMY_TYPE_WRAITH_BOSS) {
		boss.bossState = BOSS_STATE_IDLE;
		boss.nextAttackState = BOSS_STATE_ATTACK1;
		boss.stateCount = 0;
		boss.stateTimer = WRAITH_BOSS_IDLE_TIME;
		boss.stateData = 0;
	} else if (enemyType == ENEMY_TYPE_ELDER_BOSS) {
		boss.stateTimer = ELDER_BOSS_IDLE_TIME;
	}

	EnemyData curEnemyData = enemyData[enemyType];

	enemies[newEnemyIndex] = (struct Enemy) {
		.x = x,
		.y = y,
		.targetX = x,
		.targetY = y,
		.health = curEnemyData.health,
		.invincibilityTime = 0.1f,
		.attackCooldown = 0,
		.hitFlashTime = 0,
		.animationTime = randFloat(0, curEnemyData.frameTime),
		.animationFrame = randInt(0, curEnemyData.idleFrameEnd - 1),
		.active = 1,
		.state = ENEMY_STATE_IDLE,
		.type = enemyType,
		.slowTime = 0,
		.slowAmount = 0
	};

	return newEnemyIndex;
}

void spawnRandomEnemy(int spawnX, int spawnY) {
	EnemyType *spawnList = getAreaSpawnList();
	EnemyType enemyType = spawnList[randInt(0, ENEMY_TYPES_PER_AREA - 1)];

	if (SPAWN_TEST_ENEMY) {
		enemyType = ENEMY_TYPE_TEST;
	}

	spawnEnemy(enemyType, spawnX, spawnY);
}

void initEnemies(void) {
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		enemies[i].active = 0;
	}
}

void spawnEnemies(int currentArea, RoomType roomType) {
	if (roomType == ROOM_TYPE_BOSS) {
		if (currentArea == AREA_ECHOING_CAVERNS) {
			bossIndex = spawnEnemy(ENEMY_TYPE_ELDER_BOSS, MAP_WIDTH / 2 * CELL_SIZE, MAP_HEIGHT / 2 * CELL_SIZE);	
		} else if (currentArea == AREA_LOST_ARCHIVES) {
			bossIndex = spawnEnemy(ENEMY_TYPE_WRAITH_BOSS, MAP_WIDTH / 2 * CELL_SIZE, MAP_HEIGHT / 2 * CELL_SIZE);
		}
	} else if (roomType == ROOM_TYPE_ENEMY || roomType == ROOM_TYPE_ELITE) {
		int enemiesSpawned = getCurrentRoom() + 2;
		for (int i = 0; i < enemiesSpawned; i++) {
			int randMapX = randInt(ENEMY_SPAWN_MIN_X, ENEMY_SPAWN_MAX_X);
			int randMapY = randInt(ENEMY_SPAWN_MIN_Y, ENEMY_SPAWN_MAX_Y);

			while (map[randMapY * MAP_WIDTH + randMapX] != 0) {
				randMapX = randInt(ENEMY_SPAWN_MIN_X, ENEMY_SPAWN_MAX_X);
				randMapY = randInt(ENEMY_SPAWN_MIN_Y, ENEMY_SPAWN_MAX_Y);
			}

			int spawnX = randMapX * CELL_SIZE + CELL_SIZE / 2;
			int spawnY = randMapY * CELL_SIZE + CELL_SIZE / 2;
			spawnRandomEnemy(spawnX, spawnY);
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
		Enemy *curEnemy = &enemies[i];
		if (!curEnemy->active) {
			continue;
		}

		EnemyData *curEnemyData = &enemyData[curEnemy->type];

		if (SKIP_ENEMY_UPDATE) {
			addEntityToRenderQueue(curEnemy->x, curEnemy->y, ENEMY, i, curEnemyData->textureIndex, curEnemy->animationFrame);
			continue;
		}

		if (curEnemy->slowTime > 0) {
			curEnemy->slowTime -= deltaTime;
		}

		if (curEnemy->invincibilityTime > 0) {
			curEnemy->invincibilityTime -= deltaTime;
		}

		curEnemy->animationTime -= deltaTime;
		if (curEnemy->animationTime <= 0) {
			curEnemy->animationTime = curEnemyData->frameTime;
			curEnemy->animationFrame += 1;
			if (curEnemy->state == ENEMY_STATE_IDLE) {
				if (curEnemy->animationFrame >= curEnemyData->idleFrameEnd) {
					curEnemy->animationFrame = 0;
				}
			} else if (curEnemy->state == ENEMY_STATE_PREP) {
				if (curEnemy->animationFrame >= curEnemyData->prepFrameEnd) {
					curEnemy->animationFrame = curEnemyData->prepFrameEnd - 1;
				}
			} else if (curEnemy->state == ENEMY_STATE_ATTACK || curEnemy->state == ENEMY_STATE_ATTACK_END) {
				if (curEnemy->animationFrame >= curEnemyData->attackFrameEnd) {
					curEnemy->animationFrame = 0;
					if (curEnemyData->attackPattern != ATTACK_PATTERN_RUSH) {
						curEnemy->state = ENEMY_STATE_IDLE;
					}
				}
			} else if (curEnemy->state == ENEMY_STATE_DEAD) {
				if (curEnemy->animationFrame >= curEnemyData->deathFrameEnd) {
					enemies[i].active = 0;
					continue;
				}
			}
		}

		curEnemy->hitFlashTime -= deltaTime;
		if (curEnemy->hitFlashTime <= 0) {
			curEnemy->hitFlashTime = 0;
		}

		if (curEnemy->state == ENEMY_STATE_DEAD) {
			addEntityToRenderQueue(curEnemy->x, curEnemy->y, ENEMY, i, curEnemyData->textureIndex, curEnemy->animationFrame);
			continue;
		}

		if (curEnemy->attackCooldown > 0) {
			curEnemy->attackCooldown -= deltaTime;
		}

		if (curEnemy->abilityCooldown > 0) {
			curEnemy->abilityCooldown -= deltaTime;
		}

		if (curEnemyData->attackPattern == ATTACK_PATTERN_ELDER_BOSS) {
			boss.stateTimer -= deltaTime;
			if (curEnemy->state == ENEMY_STATE_IDLE) {
				if (playerInAttackDetectionRange(i)) {
					curEnemy->state = ENEMY_STATE_ATTACK;
					curEnemy->animationFrame = curEnemyData->attackFrameStart - 1;
					curEnemy->animationTime = curEnemyData->frameTime;
				} else if (boss.stateTimer <= 0) {
					boss.stateTimer = ELDER_BOSS_PREP_TIME;
					curEnemy->state = ENEMY_STATE_PREP;
					curEnemy->animationFrame = curEnemyData->prepFrameStart - 1;
					curEnemy->animationTime = curEnemyData->frameTime;

					// Spawn root attacks towards player in a line
					float dx = playerX - curEnemy->x;
					float dy = playerY - curEnemy->y;
					if (dx == 0 && dy == 0) {
						dx = 1;
						dy = 0;
					}
					float length = sqrtf(dx * dx + dy * dy);
					if (length > 0) {
						dx /= length;
						dy /= length;
					}

					for (int root = 0; root < ELDER_BOSS_ATTACK2_COUNT; root++) {
						float offsetX = dx * root * ELDER_BOSS_ATTACK2_SPACING;
						float offsetY = dy * root * ELDER_BOSS_ATTACK2_SPACING;
						spawnParticle(PARTICLE_TYPE_WARNING, curEnemy->x + offsetX, curEnemy->y + offsetY, PARTICLE_TYPE_BOSS_ROOT_ATTACK, true /* enemyParticle */);
					}
				} else {
					moveEnemy(curEnemy, i, playerX, playerY, curEnemyData->moveSpeed);
				}
			} else if (curEnemy->state == ENEMY_STATE_PREP) {
				if (boss.stateTimer <= 0) {
					boss.stateTimer = ELDER_BOSS_IDLE_TIME;
					curEnemy->state = ENEMY_STATE_IDLE;
				}
			} else if (curEnemy->state == ENEMY_STATE_ATTACK) {
				if (curEnemy->animationFrame >= curEnemyData->attackActiveFrame) {
					if (playerInAttackDistance(i)) {
						damagePlayer(curEnemyData->attackDamage);
					}
					curEnemy->state = ENEMY_STATE_ATTACK_END;
				}
			}
		} else if (curEnemyData->attackPattern == ATTACK_PATTERN_WRAITH_BOSS) {
			boss.stateTimer -= deltaTime;
			if (curEnemy->attackCooldown <= 0 && playerInAttackDistance(i)) {
				curEnemy->attackCooldown = curEnemyData->attackCooldown;
				damagePlayer(curEnemyData->attackDamage);
			}
			if (boss.bossState == BOSS_STATE_IDLE) {
				if (boss.stateTimer <= 0) {
					boss.bossState = BOSS_STATE_PREPARING;
					boss.stateTimer = WRAITH_BOSS_PREP_TIME;
					curEnemy->state = ENEMY_STATE_PREP;
					curEnemy->animationFrame = curEnemyData->prepFrameStart - 1;
					curEnemy->animationTime = curEnemyData->frameTime;
				}
			} else if (boss.bossState == BOSS_STATE_PREPARING) {
				if (boss.stateTimer <= 0) {
					if (boss.nextAttackState == BOSS_STATE_ATTACK1) {
						// == Dash attack ==
						boss.bossState = BOSS_STATE_ATTACK1;
						boss.stateTimer = WRAITH_BOSS_ATTACK1_TIME;
						boss.stateCount++;

						// Project line towards player and set target to first wall hit
						float targetX, targetY;
						castRayToWall(curEnemy->x, curEnemy->y, playerX, playerY, &targetX, &targetY);
						curEnemy->targetX = targetX;
						curEnemy->targetY = targetY;

						// Rush attack 3 times
						if (boss.stateCount >= WRAITH_BOSS_RUSH_COUNT) {
							boss.stateCount = 0;
							boss.nextAttackState = BOSS_STATE_ATTACK2;
						}
					} else if (boss.nextAttackState == BOSS_STATE_ATTACK2) {
						// == Projectile attack ==
						boss.bossState = BOSS_STATE_ATTACK2;
						boss.stateTimer = WRAITH_BOSS_ATTACK2_TIME;
						boss.nextAttackState = BOSS_STATE_ATTACK3;
					} else if (boss.nextAttackState == BOSS_STATE_ATTACK3) {
						// == Summon wraiths ==
						boss.bossState = BOSS_STATE_ATTACK3;
						boss.stateTimer = WRAITH_BOSS_ATTACK3_TIME;
						boss.nextAttackState = BOSS_STATE_ATTACK1;
						curEnemy->animationFrame = 0;
						curEnemy->state = ENEMY_STATE_IDLE;
						curEnemy->targetX = MAP_WIDTH / 2 * CELL_SIZE;
						curEnemy->targetY = MAP_HEIGHT / 2 * CELL_SIZE;
						spawnEnemy(ENEMY_TYPE_WRAITH, curEnemy->x, curEnemy->y);
						spawnEnemy(ENEMY_TYPE_WRAITH, curEnemy->x - CELL_SIZE, curEnemy->y);
						spawnEnemy(ENEMY_TYPE_WRAITH, curEnemy->x + CELL_SIZE, curEnemy->y);
					}
				}
			} else if (boss.bossState == BOSS_STATE_ATTACK1 || boss.bossState == BOSS_STATE_ATTACK2 || boss.bossState == BOSS_STATE_ATTACK3) {
				if (boss.stateTimer <= 0) {
					boss.bossState = BOSS_STATE_IDLE;
					boss.stateTimer = WRAITH_BOSS_IDLE_TIME;
					curEnemy->animationFrame = 0;
					curEnemy->state = ENEMY_STATE_IDLE;
				}
				if (boss.bossState == BOSS_STATE_ATTACK1) {
					moveEnemy(curEnemy, i, curEnemy->targetX, curEnemy->targetY, curEnemyData->abilityDataPrimary);
				} else if (boss.bossState == BOSS_STATE_ATTACK2) {
					if (curEnemy->attackCooldown <= 0) {
						curEnemy->attackCooldown = WRAITH_BOSS_PROJECTILE_COOLDOWN;
						float distanceToPlayer = sqrtf(distanceSquared(playerX, playerY, curEnemy->x, curEnemy->y));
						float deltaX = (playerX - curEnemy->x) / distanceToPlayer;
						float deltaY = (playerY - curEnemy->y) / distanceToPlayer;
						float projectileSpeed = curEnemyData->abilityDataSecondary;
						float damage = curEnemyData->attackDamage;
						float projectileX = curEnemy->x + deltaX * PROJECTILE_OFFSET;
						float projectileY = curEnemy->y + deltaY * PROJECTILE_OFFSET;
						fireEnemyProjectile(projectileX, projectileY, deltaX, deltaY, projectileSpeed, damage);
					}
				} else if (boss.bossState == BOSS_STATE_ATTACK3) {
					moveEnemy(curEnemy, i, curEnemy->targetX, curEnemy->targetY, curEnemyData->moveSpeed);
				}
			}
		} else {
			// Non-boss enemies
			if (curEnemy->state == ENEMY_STATE_IDLE) {
				AttackPattern curAttackPattern = curEnemyData->attackPattern;
				if (curEnemy->attackCooldown <= 0 && playerInAttackDetectionRange(i)) {
					curEnemy->attackCooldown = curEnemyData->attackCooldown;
					if (curAttackPattern == ATTACK_PATTERN_MELEE || curAttackPattern == ATTACK_PATTERN_PROJECTILE) {
						curEnemy->state = ENEMY_STATE_ATTACK;
						curEnemy->animationFrame = curEnemyData->attackFrameStart - 1;
						curEnemy->animationTime = curEnemyData->frameTime;
					} else if (curAttackPattern == ATTACK_PATTERN_PROXIMITY) {
						if (playerInAttackDistance(i)) {
							damagePlayer(curEnemyData->attackDamage);
						}
					} else if (curAttackPattern == ATTACK_PATTERN_RUSH) {
						float targetX, targetY;
						castRayToWall(curEnemy->x, curEnemy->y, playerX, playerY, &targetX, &targetY);
						curEnemy->targetX = targetX;
						curEnemy->targetY = targetY;
						curEnemy->state = ENEMY_STATE_ATTACK;
						curEnemy->animationFrame = curEnemyData->attackFrameStart - 1;
						curEnemy->animationTime = curEnemyData->frameTime;
						curEnemy->abilityCooldown = curEnemyData->abilityDataSecondary;
					}
				}

				if (curAttackPattern == ATTACK_PATTERN_PROJECTILE) {
					if (curEnemy->abilityCooldown <= 0) {
						if (randInt(0, 100) < 33) {
							curEnemy->targetX = playerX;
							curEnemy->targetY = playerY;
						} else {
							curEnemy->targetX = curEnemy->x + randInt(-1, 1) * CELL_SIZE;
							curEnemy->targetY = curEnemy->y + randInt(-1, 1) * CELL_SIZE;
						}
						curEnemy->abilityCooldown = curEnemyData->abilityDataSecondary;
					}
					moveEnemy(curEnemy, i, curEnemy->targetX, curEnemy->targetY, curEnemyData->moveSpeed);
				} else {
					moveEnemy(curEnemy, i, playerX, playerY, curEnemyData->moveSpeed);
				}
			} else if (curEnemy->state == ENEMY_STATE_ATTACK) {
				AttackPattern curAttackPattern = curEnemyData->attackPattern;
				if (curAttackPattern == ATTACK_PATTERN_RUSH) {
					moveEnemy(curEnemy, i, curEnemy->targetX, curEnemy->targetY, curEnemyData->abilityDataPrimary);
					if (playerInAttackDistance(i)) {
						damagePlayer(curEnemyData->attackDamage);
					}
					if (curEnemy->abilityCooldown <= 0) {
						curEnemy->attackCooldown = curEnemyData->attackCooldown;
						curEnemy->state = ENEMY_STATE_IDLE;
						curEnemy->animationFrame = 0;
					}
				} else if (curEnemy->animationFrame >= curEnemyData->attackActiveFrame) {
					if (curAttackPattern == ATTACK_PATTERN_MELEE) {
						if (playerInAttackDistance(i)) {
							damagePlayer(curEnemyData->attackDamage);
						}
					} else if (curAttackPattern == ATTACK_PATTERN_PROJECTILE) {
						float distanceToPlayer = sqrtf(distanceSquared(playerX, playerY, curEnemy->x, curEnemy->y));
						float deltaX = (playerX - curEnemy->x) / distanceToPlayer;
						float deltaY = (playerY - curEnemy->y) / distanceToPlayer;
						float projectileSpeed = curEnemyData->abilityDataPrimary;
						float damage = curEnemyData->attackDamage;
						float projectileX = curEnemy->x + deltaX * PROJECTILE_OFFSET;
						float projectileY = curEnemy->y + deltaY * PROJECTILE_OFFSET;
						fireEnemyProjectile(projectileX, projectileY, deltaX, deltaY, projectileSpeed, damage);
					}
					curEnemy->state = ENEMY_STATE_ATTACK_END;
				}
			}
		}

		addEntityToRenderQueue(curEnemy->x, curEnemy->y, ENEMY, i, curEnemyData->textureIndex, curEnemy->animationFrame);
	}
}

void enemyDeathEffect(int index) {
	Enemy curEnemy = enemies[index];
	EnemyType enemyType = curEnemy.type;
	if (enemyType == ENEMY_TYPE_SLIME) {
		int slimeSpawnCount = enemyData[enemyType].abilityDataPrimary;
		int offsetMax = CELL_SIZE / 4;
		int offsetMin = -offsetMax;
		for (int i = 0; i < slimeSpawnCount; i++) {
			int xOffset = randInt(offsetMin, offsetMax);
			int yOffset = randInt(offsetMin, offsetMax);
			spawnEnemy(ENEMY_TYPE_BABY_SLIME, curEnemy.x + xOffset, curEnemy.y + yOffset);
		}
	}
}

float getBossHealthPercentage(void) {
	Enemy boss = enemies[bossIndex];
	if (boss.state == ENEMY_STATE_DEAD) {
		return 0;
	}

	EnemyData bossData = enemyData[boss.type];
	return (float)boss.health / (float)bossData.health;
}

void slowEnemy(int index, float slowTime, float slowAmount) {
	Enemy curEnemy = enemies[index];
	if (curEnemy.state == ENEMY_STATE_DEAD || curEnemy.slowTime > 0 || curEnemy.invincibilityTime > 0) {
		return;
	}

	enemies[index].slowTime = slowTime;
	enemies[index].slowAmount = slowAmount;
}

bool damageEnemy(int index, int damage) {
	Enemy curEnemy = enemies[index];
	if (curEnemy.state == ENEMY_STATE_DEAD || curEnemy.invincibilityTime > 0) {
		return false;
	}

	EnemyData curEnemyData = enemyData[curEnemy.type];
	curEnemy.hitFlashTime = HIT_FLASH_TIME;
	curEnemy.health -= damage;
	playSound(SFX_ENEMY_HIT);

	if (curEnemy.health <= 0) {
		playSound(SFX_ENEMY_DEATH);
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
			setRoomCompleted(1);
		}
	}
	
	enemies[index] = curEnemy;
	return true;
}

void damageEnemiesAtMapPosition(int mapX, int mapY, int damage) {
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		Enemy enemy = enemies[i];
		if (!enemy.active) {
			continue;
		}
		int enemyMapX = (int)enemy.x / CELL_SIZE;
		int enemyMapY = (int)enemy.y / CELL_SIZE;
		if (enemyMapX == mapX && enemyMapY == mapY) {
			damageEnemy(i, damage);
		}
	}
}

void damageEnemiesInArea(float x, float y, int radiusSquared, int damage) {
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		Enemy enemy = enemies[i];
		if (!enemy.active) {
			continue;
		}
		float distSquared = distanceSquared(enemy.x, enemy.y, x, y);
		if (distSquared < radiusSquared) {
			damageEnemy(i, damage);
		}
	}
}

bool checkEnemyCollision(float x, float y, Set *hitEnemies, float collisionRange, int damage, float slowTime, float slowAmount) {
	bool collided = false; 
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		Enemy curEnemy = enemies[i];
		if (!curEnemy.active || setContains(hitEnemies, i)) {
			continue;
		}

		float distSquared = distanceSquared(x, y, curEnemy.x, curEnemy.y);
		if (distSquared < collisionRange) {
			if (slowTime > 0) {
				slowEnemy(i, slowTime, slowAmount);
			}
			bool enemyDamaged = damageEnemy(i, damage);
			if (enemyDamaged) {
				setAdd(hitEnemies, i);
				collided = true;
			}
		}
	}
	return collided;
}