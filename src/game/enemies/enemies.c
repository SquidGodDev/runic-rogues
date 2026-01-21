#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "game/map/map.h"
#include "game/enemies/enemies.h"
#include "game/player/player.h"
#include "game/entities/entities.h"
#include "game/entities/projectiles.h"
#include "game/rendering/textures.h"
#include "game/rendering/particles.h"
#include "game/systems/audioManager.h"
#include "game/systems/runeManager.h"
#include "game/systems/progressionManager.h"
#include "game/systems/waveManager.h"

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
#define ENEMY_SEPARATION_FORCE 0.3f
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

typedef enum AttackPattern
{
	ATTACK_PATTERN_PROXIMITY,
	ATTACK_PATTERN_MELEE,
	ATTACK_PATTERN_PROJECTILE,
	ATTACK_PATTERN_RUSH,
	ATTACK_PATTERN_CUSTOM_ATTACK,
	ATTACK_PATTERN_TURRET,
	ATTACK_PATTERN_ELDER_BOSS,
	ATTACK_PATTERN_WRAITH_BOSS
} AttackPattern;

Enemy enemies[MAX_ENEMY_COUNT] = {0};
static int bossIndex;
static int enemiesDefeated;

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
	bool knockbackResistant;
} EnemyData;

EnemyData enemyData[ENEMY_TYPE_COUNT] = {
	[ENEMY_TYPE_TEST] = (EnemyData){
		.name = "Test",
		.health = 4,
		.attackDetectionRange = 10,
		.attackDistance = 10,
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
		.health = 30,
		.attackDetectionRange = 55,
		.attackDistance = 10,
		.attackDamage = 15,
		.attackCooldown = 4.0f,
		.moveSpeed = 0.15f,
		.abilityDataPrimary = 0.35f, // Rush move speed
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
		.health = 40,
		.attackDetectionRange = 10,
		.attackDistance = 15,
		.attackDamage = 15,
		.attackCooldown = 2.0f,
		.moveSpeed = 0.25f,
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
		.health = 20,
		.attackDetectionRange = 10,
		.attackDistance = 15,
		.attackDamage = 5,
		.attackCooldown = 2.0f,
		.moveSpeed = 0.2f,
		.textureIndex = TEXTURE_BABY_SLIME,
		.idleFrameEnd = 6,
		.deathFrameStart = 7,
		.deathFrameEnd = 10,
		.frameTime = 0.1f,
		.attackPattern = ATTACK_PATTERN_PROXIMITY
	},
	[ENEMY_TYPE_SKELETON] = (EnemyData){
		.name = "Skeleton",
		.health = 50,
		.attackDetectionRange = 15,
		.attackDistance = 20,
		.attackDamage = 20,
		.attackCooldown = 1.0f,
		.moveSpeed = 0.25f,
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
	[ENEMY_TYPE_SERPENT] = (EnemyData){
		.name = "Serpent",
		.health = 40,
		.attackDetectionRange = 35,
		.attackDamage = 10,
		.attackCooldown = 8.0f,
		.moveSpeed = 0.3f,
		.abilityDataPrimary = 4, // Tail attack count
		.abilityDataSecondary = 0.3f, // Tail attack interval
		.textureIndex = TEXTURE_SERPENT,
		.idleFrameEnd = 12,
		.attackFrameStart = 14,
		.attackFrameEnd = 41,
		.attackActiveFrame = 19,
		.deathFrameStart = 42,
		.deathFrameEnd = 45,
		.frameTime = 0.08f,
		.attackPattern = ATTACK_PATTERN_CUSTOM_ATTACK
	},
	[ENEMY_TYPE_TURRET] = (EnemyData){
		.name = "Turret",
		.health = 10,
		.attackDetectionRange = 150,
		.attackDamage = 10,
		.attackCooldown = 0.5f,
		.moveSpeed = 0.0f,
		.abilityDataPrimary = 8, // Firing angles
		.abilityDataSecondary = 40.0f, // Projectile speed
		.textureIndex = TEXTURE_TURRET,
		.idleFrameEnd = 1,
		.deathFrameStart = 1,
		.deathFrameEnd = 11,
		.frameTime = 0.1f,
		.attackPattern = ATTACK_PATTERN_TURRET
	},
	[ENEMY_TYPE_ELDER_BOSS] = (EnemyData){
		.name = "Elder Boss",
		.health = 700,
		.attackDetectionRange = 20,
		.attackDistance = 20,
		.attackDamage = 20,
		.attackCooldown = 1.5f,
		.moveSpeed = 0.4f,
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
		.attackPattern = ATTACK_PATTERN_ELDER_BOSS,
		.knockbackResistant = true
	},
	[ENEMY_TYPE_RAT] = (EnemyData){
		.name = "Rat",
		.health = 30,
		.attackDetectionRange = 10,
		.attackDistance = 15,
		.attackDamage = 5,
		.attackCooldown = 2.0f,
		.moveSpeed = 0.2f,
		.textureIndex = TEXTURE_RAT,
		.idleFrameEnd = 9,
		.deathFrameStart = 10,
		.deathFrameEnd = 13,
		.frameTime = 0.1f,
		.attackPattern = ATTACK_PATTERN_PROXIMITY
	},
	[ENEMY_TYPE_IMP] = (EnemyData){
		.name = "Imp",
		.health = 40,
		.attackDetectionRange = 45,
		.attackDamage = 10,
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
		.health = 60,
		.attackDetectionRange = 55,
		.attackDistance = 10,
		.attackDamage = 20,
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
		.health = 100,
		.attackDetectionRange = 22,
		.attackDistance = 25,
		.attackDamage = 30,
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
		.attackPattern = ATTACK_PATTERN_MELEE,
		.knockbackResistant = true
	},
	[ENEMY_TYPE_MUMMY] = (EnemyData){
		.name = "Mummy",
		.health = 60,
		.attackDetectionRange = 60,
		.attackDamage = 12,
		.attackCooldown = 4.0f,
		.moveSpeed = 0.2f,
		.textureIndex = TEXTURE_MUMMY,
		.idleFrameEnd = 8,
		.attackFrameStart = 9,
		.attackFrameEnd = 17,
		.attackActiveFrame = 13,
		.deathFrameStart = 18,
		.deathFrameEnd = 21,
		.frameTime = 0.14f,
		.attackPattern = ATTACK_PATTERN_CUSTOM_ATTACK
	},
	[ENEMY_TYPE_WRAITH_BOSS] = (EnemyData){
		.name = "Wraith Boss",
		.health = 1000,
		.attackDetectionRange = 10,
		.attackDistance = 10,
		.attackDamage = 20,
		.attackCooldown = 1.5f,
		.moveSpeed = 0.25f,
		.abilityDataPrimary = 1.2f, // Rush move speed
		.abilityDataSecondary = 70.0f, // Projectile speed
		.textureIndex = TEXTURE_WRAITH_BOSS,
		.idleFrameEnd = 6,
		.prepFrameStart = 7,
		.prepFrameEnd = 10,
		.attackFrameStart = 10,
		.attackFrameEnd = 10,
		.deathFrameStart = 11,
		.deathFrameEnd = 14,
		.frameTime = 0.15f,
		.attackPattern = ATTACK_PATTERN_WRAITH_BOSS,
		.knockbackResistant = true
	},
	[ENEMY_TYPE_WRAITH] = (EnemyData){
		.name = "Wraith",
		.health = 20,
		.attackDetectionRange = 10,
		.attackDistance = 15,
		.attackDamage = 15,
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
			if (dist > 0.01f) {
				otherX /= dist;
				otherY /= dist;
				float force = ENEMY_SEPARATION_FORCE * (1.0f - dist / ENEMY_SEPARATION_RADIUS);
				dx += otherX * force;
				dy += otherY * force;
			}
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

int spawnEnemy(EnemyType enemyType, int x, int y, bool isMinion) {
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
		.stateCount = 0,
		.stateTimer = 0,
		.nextState = ENEMY_STATE_ATTACK_1,
		.type = enemyType,
		.isMinion = isMinion,
		.attackedPlayer = false,
		.slowTime = 0,
		.slowAmount = 0
	};

	if (enemyType == ENEMY_TYPE_WRAITH_BOSS) {
		bossIndex = newEnemyIndex;
		setBossActive(true);
		enemies[newEnemyIndex].stateTimer = WRAITH_BOSS_IDLE_TIME;
	} else if (enemyType == ENEMY_TYPE_ELDER_BOSS) {
		bossIndex = newEnemyIndex;
		setBossActive(true);
		enemies[newEnemyIndex].stateTimer = ELDER_BOSS_IDLE_TIME;
	}

	return newEnemyIndex;
}

void initEnemies(void) {
	bossIndex = -1;
	enemiesDefeated = 0;
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		enemies[i].active = 0;
	}
}

int getEnemiesDefeated(void) {
	return enemiesDefeated;
}

int getActiveEnemyCount(void) {
	int count = 0;
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		if (enemies[i].active) {
			count++;
		}
	}
	return count;
}

int playerInAttackDetectionRange(int enemyIndex) {
	Enemy curEnemy = enemies[enemyIndex];
	EnemyData curEnemyData = enemyData[curEnemy.type];
	return distanceSquared(curEnemy.x, curEnemy.y, playerX, playerY) <= curEnemyData.attackDetectionRange * curEnemyData.attackDetectionRange;
}

int playerInAttackDistance(int enemyIndex) {
	Enemy curEnemy = enemies[enemyIndex];
	EnemyData curEnemyData = enemyData[curEnemy.type];
	return distanceSquared(curEnemy.x, curEnemy.y, playerX, playerY) <= curEnemyData.attackDistance * curEnemyData.attackDistance;
}

void transitionToState(Enemy *enemy, EnemyState newState) {
	enemy->state = newState;
	EnemyData *curEnemyData = &enemyData[enemy->type];
	enemy->animationTime = curEnemyData->frameTime;
	if (newState == ENEMY_STATE_IDLE) {
		enemy->animationFrame = 0;
	} else if (newState == ENEMY_STATE_PREP) {
		enemy->animationFrame = curEnemyData->prepFrameStart - 1;
	} else if (newState == ENEMY_STATE_ATTACK_1 || newState == ENEMY_STATE_ATTACK_2 || newState == ENEMY_STATE_ATTACK_3) {
		enemy->animationFrame = curEnemyData->attackFrameStart - 1;
	} else if (newState == ENEMY_STATE_DEAD) {
		enemy->animationFrame = curEnemyData->deathFrameStart - 1;
	}
}

void proximityEnemyUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index) {
	// Constantly attack nearby player
	if (playerInAttackDistance(index)) {
		if (curEnemy->attackCooldown <= 0) {
			curEnemy->attackCooldown = curEnemyData->attackCooldown;
			damagePlayer(curEnemyData->attackDamage);
		}
	} else {
		curEnemy->attackCooldown = curEnemyData->attackCooldown;
	}

	if (!playerInAttackDetectionRange(index)) {
		// Constantly move towards player if not in attack range
		moveEnemy(curEnemy, index, playerX, playerY, curEnemyData->moveSpeed);
	}
}

void meleeEnemyUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	if (curEnemy->state == ENEMY_STATE_IDLE) {
		// Start attacking if player is in range
		if (playerInAttackDetectionRange(index)) {
			if (curEnemy->attackCooldown <= 0) {
				curEnemy->attackCooldown = curEnemyData->attackCooldown;
				transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
			}
		} else {
			curEnemy->attackCooldown = curEnemyData->attackCooldown;
		}

		// Move towards player
		moveEnemy(curEnemy, index, playerX, playerY, curEnemyData->moveSpeed);
	} else if (curEnemy->state == ENEMY_STATE_ATTACK_1) {
		// Damage player once attack animation passes active frame
		if (curEnemy->animationFrame >= curEnemyData->attackActiveFrame) {
			if (!curEnemy->attackedPlayer && playerInAttackDistance(index)) {
				curEnemy->attackedPlayer = true;
				damagePlayer(curEnemyData->attackDamage);
			}
		}
		
		// Transition back to idle state after attack animation ends
		if (attackAnimationReset) {
			curEnemy->attackedPlayer = false;
			curEnemy->attackCooldown = curEnemyData->attackCooldown;
			transitionToState(curEnemy, ENEMY_STATE_IDLE);
		}
	}
}

void projectileEnemyUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	if (curEnemy->state == ENEMY_STATE_IDLE) {
		// Start shooting projectile if player is in range
		if (curEnemy->attackCooldown <= 0 && playerInAttackDetectionRange(index)) {
			curEnemy->attackCooldown = curEnemyData->attackCooldown;
			transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
		}

		// Choose a random target position to move towards
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
		moveEnemy(curEnemy, index, curEnemy->targetX, curEnemy->targetY, curEnemyData->moveSpeed);
	} else if (curEnemy->state == ENEMY_STATE_ATTACK_1) {
		// Fire projectile towards player if attack animation passes active frame
		if (!curEnemy->attackedPlayer && curEnemy->animationFrame >= curEnemyData->attackActiveFrame) {
			curEnemy->attackedPlayer = true;
			float distanceToPlayer = sqrtf(distanceSquared(playerX, playerY, curEnemy->x, curEnemy->y));
			float deltaX = (playerX - curEnemy->x) / distanceToPlayer;
			float deltaY = (playerY - curEnemy->y) / distanceToPlayer;
			float projectileSpeed = curEnemyData->abilityDataPrimary;
			float damage = curEnemyData->attackDamage;
			float projectileX = curEnemy->x + deltaX * PROJECTILE_OFFSET;
			float projectileY = curEnemy->y + deltaY * PROJECTILE_OFFSET;
			fireEnemyProjectile(projectileX, projectileY, deltaX, deltaY, projectileSpeed, damage);
		}

		// Transition back to idle state after attack animation ends
		if (attackAnimationReset) {
			curEnemy->attackedPlayer = false;
			transitionToState(curEnemy, ENEMY_STATE_IDLE);
		}
	}
}

void rushEnemyUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index) {
	if (curEnemy->state == ENEMY_STATE_IDLE) {
		// Start rushing in fixed direction until hitting wall if player is in attack detection range
		if (curEnemy->attackCooldown <= 0 && playerInAttackDetectionRange(index)) {
			curEnemy->attackCooldown = curEnemyData->attackCooldown;
			curEnemy->abilityCooldown = curEnemyData->abilityDataSecondary;
			float targetX, targetY;
			castRayToWall(curEnemy->x, curEnemy->y, playerX, playerY, &targetX, &targetY);
			curEnemy->targetX = targetX;
			curEnemy->targetY = targetY;
			transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
		}

		// Move towards player at regular speed when idle
		moveEnemy(curEnemy, index, playerX, playerY, curEnemyData->moveSpeed);
	} else if (curEnemy->state == ENEMY_STATE_ATTACK_1) {
		// Damage player if nearby while rushing
		if (playerInAttackDistance(index)) {
			damagePlayer(curEnemyData->attackDamage);
		}

		// Once rush timer is over, transition back to idle state
		if (curEnemy->abilityCooldown <= 0) {
			curEnemy->attackCooldown = curEnemyData->attackCooldown;
			transitionToState(curEnemy, ENEMY_STATE_IDLE);
		}

		// Move towards target position at rush speed
		moveEnemy(curEnemy, index, curEnemy->targetX, curEnemy->targetY, curEnemyData->abilityDataPrimary);
	}
}

void customAttackEnemyUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	if (curEnemy->state == ENEMY_STATE_IDLE) {
		// Start custom attack if player is in range
		if (curEnemy->attackCooldown <= 0 && playerInAttackDetectionRange(index)) {
			curEnemy->attackCooldown = curEnemyData->attackCooldown;
			transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
		}

		moveEnemy(curEnemy, index, playerX, playerY, curEnemyData->moveSpeed);
	} else if (curEnemy->state == ENEMY_STATE_ATTACK_1) {
		// Fire custom attack towards player if attack animation passes active frame
		if (curEnemy->animationFrame >= curEnemyData->attackActiveFrame) {
			if (curEnemy->type == ENEMY_TYPE_MUMMY) {
				if (!curEnemy->attackedPlayer) {
					curEnemy->attackedPlayer = true;
					spawnEnemy(ENEMY_TYPE_WRAITH, curEnemy->x, curEnemy->y, true /*isMinion*/);
				}
			} else if (curEnemy->type == ENEMY_TYPE_SERPENT) {
				// Spawn tail attack particles at intervals during attack animation
				if (!curEnemy->attackedPlayer) {
					curEnemy->attackedPlayer = true;
					curEnemy->targetX = playerX;
					curEnemy->targetY = playerY;
				}
				int attackCount = curEnemy->stateCount;
				int maxAttackCount = curEnemyData->abilityDataPrimary;
				if (curEnemy->stateTimer <= 0 && attackCount < maxAttackCount) {
					float dx = curEnemy->targetX - curEnemy->x;
					float dy = curEnemy->targetY - curEnemy->y;
					if (dx == 0 && dy == 0) {
						dx = 1;
						dy = 0;
					}
					float length = sqrtf(dx * dx + dy * dy);
					if (length > 0) {
						dx /= length;
						dy /= length;
					}
					float offsetX = dx * (curEnemy->stateCount + 1) * CELL_SIZE / 2;
					float offsetY = dy * (curEnemy->stateCount + 1) * CELL_SIZE / 2;
					spawnParticle(PARTICLE_TYPE_SERPENT_TAIL, curEnemy->x + offsetX, curEnemy->y + offsetY, PARTICLE_TYPE_NONE, true /* enemyParticle */);
					curEnemy->stateCount++;
					curEnemy->stateTimer = curEnemyData->abilityDataSecondary;
				}
			}
		}

		// Transition back to idle state after attack animation ends
		if (attackAnimationReset) {
			curEnemy->attackedPlayer = false;
			curEnemy->stateTimer = 0;
			curEnemy->stateCount = 0;
			transitionToState(curEnemy, ENEMY_STATE_IDLE);
		}
	}
}

void turretEnemyUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index) {
	if (curEnemy->state == ENEMY_STATE_IDLE) {
		// Start attacking if player is in range
		if (curEnemy->attackCooldown <= 0 && playerInAttackDetectionRange(index)) {
			curEnemy->attackCooldown = curEnemyData->attackCooldown;
			
			float angle = curEnemy->stateCount * (PI * 2 / curEnemyData->abilityDataPrimary);
			float deltaX = cosf(angle);
			float deltaY = sinf(angle);
			float projectileSpeed = curEnemyData->abilityDataSecondary;
			float damage = curEnemyData->attackDamage;
			float projectileX = curEnemy->x + deltaX * PROJECTILE_OFFSET;
			float projectileY = curEnemy->y + deltaY * PROJECTILE_OFFSET;
			fireEnemyProjectile(projectileX, projectileY, deltaX, deltaY, projectileSpeed, damage);

			curEnemy->stateCount += 1;
			if (curEnemy->stateCount >= curEnemyData->abilityDataPrimary) {
				curEnemy->stateCount = 0;
			}
		}
	}
}

void elderBossUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	if (curEnemy->state == ENEMY_STATE_IDLE) {
		if (playerInAttackDetectionRange(index)) {
			transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
		} else if (curEnemy->stateTimer <= 0) {
			curEnemy->stateTimer = ELDER_BOSS_PREP_TIME;
			transitionToState(curEnemy, ENEMY_STATE_PREP);

			for (int dir = 0; dir < 8; dir++) {
				float angle = dir * (PI / 4);
				float dx = cosf(angle);
				float dy = sinf(angle);
				for (int root = 1; root <= ELDER_BOSS_ATTACK2_COUNT; root++) {
					float offsetX = dx * (float)(root * ELDER_BOSS_ATTACK2_SPACING);
					float offsetY = dy * (float)(root * ELDER_BOSS_ATTACK2_SPACING);
					spawnParticle(PARTICLE_TYPE_WARNING, curEnemy->x + offsetX, curEnemy->y + offsetY, PARTICLE_TYPE_BOSS_ROOT_ATTACK, true /* enemyParticle */);
				}
			}

		} else {
			moveEnemy(curEnemy, index, playerX, playerY, curEnemyData->moveSpeed);
		}
	} else if (curEnemy->state == ENEMY_STATE_PREP) {
		if (curEnemy->stateTimer <= 0) {
			curEnemy->stateTimer = ELDER_BOSS_IDLE_TIME;
			curEnemy->state = ENEMY_STATE_IDLE;
		}
	} else if (curEnemy->state == ENEMY_STATE_ATTACK_1) {
		if (!curEnemy->attackedPlayer && curEnemy->animationFrame >= curEnemyData->attackActiveFrame) {
			if (playerInAttackDistance(index)) {
				curEnemy->attackedPlayer = true;
				damagePlayer(curEnemyData->attackDamage);
			}
		}

		if (attackAnimationReset) {
			curEnemy->attackedPlayer = false;
			if (playerInAttackDetectionRange(index)) {
				transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
			} else {
				transitionToState(curEnemy, ENEMY_STATE_IDLE);
			}
		}
	}
}

void wraithBossUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	if (curEnemy->attackCooldown <= 0 && playerInAttackDistance(index)) {
		curEnemy->attackCooldown = curEnemyData->attackCooldown;
		damagePlayer(curEnemyData->attackDamage);
	}

	if (curEnemy->state == ENEMY_STATE_IDLE) {
		if (curEnemy->stateTimer <= 0) {
			transitionToState(curEnemy, ENEMY_STATE_PREP);
			curEnemy->stateTimer = WRAITH_BOSS_PREP_TIME;
		}
	} else if (curEnemy->state == ENEMY_STATE_PREP) {
		if (curEnemy->stateTimer <= 0) {
			if (curEnemy->nextState == ENEMY_STATE_ATTACK_1) {
				// == Dash attack ==
				transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
				curEnemy->stateTimer = WRAITH_BOSS_ATTACK1_TIME;
				curEnemy->stateCount++;

				// Project line towards player and set target to first wall hit
				float targetX, targetY;
				castRayToWall(curEnemy->x, curEnemy->y, playerX, playerY, &targetX, &targetY);
				curEnemy->targetX = targetX;
				curEnemy->targetY = targetY;

				// Rush attack 3 times
				if (curEnemy->stateCount >= WRAITH_BOSS_RUSH_COUNT) {
					curEnemy->stateCount = 0;
					curEnemy->nextState = ENEMY_STATE_ATTACK_2;
				}
			} else if (curEnemy->nextState == ENEMY_STATE_ATTACK_2) {
				// == Projectile attack ==
				transitionToState(curEnemy, ENEMY_STATE_ATTACK_2);
				curEnemy->stateTimer = WRAITH_BOSS_ATTACK2_TIME;
				curEnemy->nextState = ENEMY_STATE_ATTACK_3;
			} else if (curEnemy->nextState == ENEMY_STATE_ATTACK_3) {
				// == Summon wraiths ==
				transitionToState(curEnemy, ENEMY_STATE_IDLE);
				curEnemy->stateTimer = WRAITH_BOSS_ATTACK3_TIME;
				curEnemy->nextState = ENEMY_STATE_ATTACK_1;
				curEnemy->targetX = MAP_WIDTH / 2 * CELL_SIZE;
				curEnemy->targetY = MAP_HEIGHT / 2 * CELL_SIZE;
				spawnEnemy(ENEMY_TYPE_WRAITH, curEnemy->x, curEnemy->y, true /*isMinion*/);
				spawnEnemy(ENEMY_TYPE_WRAITH, curEnemy->x - CELL_SIZE, curEnemy->y, true /*isMinion*/);
				spawnEnemy(ENEMY_TYPE_WRAITH, curEnemy->x + CELL_SIZE, curEnemy->y, true /*isMinion*/);
			}
		}
	} else if (curEnemy->state == ENEMY_STATE_ATTACK_1 || curEnemy->state == ENEMY_STATE_ATTACK_2 || curEnemy->state == ENEMY_STATE_ATTACK_3) {
		if (curEnemy->stateTimer <= 0) {
			transitionToState(curEnemy, ENEMY_STATE_IDLE);
			curEnemy->stateTimer = WRAITH_BOSS_IDLE_TIME;
		}

		if (curEnemy->state == ENEMY_STATE_ATTACK_1) {
			moveEnemy(curEnemy, index, curEnemy->targetX, curEnemy->targetY, curEnemyData->abilityDataPrimary);
		} else if (curEnemy->state == ENEMY_STATE_ATTACK_2) {
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
		} else if (curEnemy->state == ENEMY_STATE_ATTACK_3) {
			moveEnemy(curEnemy, index, curEnemy->targetX, curEnemy->targetY, curEnemyData->moveSpeed);
		}
	}
}

void updateEnemyTimers(Enemy *curEnemy, float deltaTime) {
	if (curEnemy->slowTime > 0) {
		curEnemy->slowTime -= deltaTime;
	}

	if (curEnemy->knockbackTimeRemaining > 0) {
		curEnemy->knockbackTimeRemaining -= deltaTime;
	}

	if (curEnemy->invincibilityTime > 0) {
		curEnemy->invincibilityTime -= deltaTime;
	}

	if (curEnemy->hitFlashTime > 0) {
		curEnemy->hitFlashTime -= deltaTime;
	}

	if (curEnemy->attackCooldown > 0) {
		curEnemy->attackCooldown -= deltaTime;
	}

	if (curEnemy->abilityCooldown > 0) {
		curEnemy->abilityCooldown -= deltaTime;
	}

	if (curEnemy->stateTimer > 0) {
		curEnemy->stateTimer -= deltaTime;
	}
}

bool updateEnemyAnimation(Enemy *curEnemy, EnemyData *curEnemyData, float deltaTime) {
	bool attackAnimationReset = false;
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
		} else if (curEnemy->state == ENEMY_STATE_ATTACK_1 || curEnemy->state == ENEMY_STATE_ATTACK_2 || curEnemy->state == ENEMY_STATE_ATTACK_3) {
			if (curEnemy->animationFrame >= curEnemyData->attackFrameEnd) {
				curEnemy->animationFrame = curEnemyData->attackFrameStart - 1;
				attackAnimationReset = true;
			}
		} else if (curEnemy->state == ENEMY_STATE_DEAD) {
			if (curEnemy->animationFrame >= curEnemyData->deathFrameEnd) {
				curEnemy->active = 0;
			}
		}
	}
	return attackAnimationReset;
}

void applyEnemyKnockback(Enemy *enemy) {
	if (enemy->knockbackTimeRemaining > 0) {
		float knockbackX = enemy->x - enemy->knockbackSourceX;
		float knockbackY = enemy->y - enemy->knockbackSourceY;
		float knockbackDist = sqrtf(knockbackX*knockbackX + knockbackY*knockbackY);
		if (knockbackDist > 0.001f) {
			knockbackX /= knockbackDist;
			knockbackY /= knockbackDist;

			float timeScale = enemy->knockbackTimeRemaining / enemy->knockbackTime;

			float dx = knockbackX * enemy->knockbackAmount * timeScale;
			float dy = knockbackY * enemy->knockbackAmount * timeScale;

			// Wall avoidance
			int frontX = (int)((enemy->x + dx * WALL_CHECK_DISTANCE) / CELL_SIZE);
			int frontY = (int)((enemy->y + dy * WALL_CHECK_DISTANCE) / CELL_SIZE);
			if (isBlocked(frontX, frontY)) {
				dx = -dx * WALL_AVOID_FORCE;
				dy = -dy * WALL_AVOID_FORCE;
			}

			enemy->x += dx;
			enemy->y += dy;
		}
	}
}

void updateEnemies(float deltaTime) {
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		Enemy *curEnemy = &enemies[i];
		if (!curEnemy->active) {
			continue;
		}
		
		updateEnemyTimers(curEnemy, deltaTime);
		EnemyData *curEnemyData = &enemyData[curEnemy->type];
		bool attackAnimationReset = updateEnemyAnimation(curEnemy, curEnemyData, deltaTime);

		if (curEnemy->state == ENEMY_STATE_DEAD) {
			if(!curEnemy->active) {
				continue;
			}
			addEntityToRenderQueue(curEnemy->x, curEnemy->y, ENEMY, i, curEnemyData->textureIndex, curEnemy->animationFrame);
			continue;
		}

		applyEnemyKnockback(curEnemy);

		AttackPattern curAttackPattern = curEnemyData->attackPattern;
		if (curAttackPattern == ATTACK_PATTERN_PROXIMITY) {
			proximityEnemyUpdate(curEnemy, curEnemyData, i);
		} else if (curAttackPattern == ATTACK_PATTERN_MELEE) {
			meleeEnemyUpdate(curEnemy, curEnemyData, i, attackAnimationReset);
		} else if (curAttackPattern == ATTACK_PATTERN_PROJECTILE) {
			projectileEnemyUpdate(curEnemy, curEnemyData, i, attackAnimationReset);
		} else if (curAttackPattern == ATTACK_PATTERN_RUSH) {
			rushEnemyUpdate(curEnemy, curEnemyData, i);
		} else if (curAttackPattern == ATTACK_PATTERN_CUSTOM_ATTACK) {
			customAttackEnemyUpdate(curEnemy, curEnemyData, i, attackAnimationReset);
		} else if (curAttackPattern == ATTACK_PATTERN_TURRET) {
			turretEnemyUpdate(curEnemy, curEnemyData, i);
		} else if (curAttackPattern == ATTACK_PATTERN_ELDER_BOSS) {
			elderBossUpdate(curEnemy, curEnemyData, i, attackAnimationReset);
		} else if (curAttackPattern == ATTACK_PATTERN_WRAITH_BOSS) {
			wraithBossUpdate(curEnemy, curEnemyData, i, attackAnimationReset);
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
			spawnEnemy(ENEMY_TYPE_BABY_SLIME, curEnemy.x + xOffset, curEnemy.y + yOffset, true /*isMinion*/);
		}
	}
}

float getBossHealthPercentage(void) {
	if (bossIndex == -1) {
		return 0;
	}

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

void knockbackEnemy(int index, int knockbackSourceX, int knockbackSourceY, float knockbackTime, float knockbackAmount) {
	Enemy curEnemy = enemies[index];
	if (curEnemy.state == ENEMY_STATE_DEAD || curEnemy.invincibilityTime > 0) {
		return;
	}

	if (!enemyData[curEnemy.type].knockbackResistant && knockbackTime > 0 && knockbackAmount != 0) {
		enemies[index].knockbackTime = knockbackTime;
		enemies[index].knockbackTimeRemaining = knockbackTime;
		enemies[index].knockbackAmount = knockbackAmount;
		enemies[index].knockbackSourceX = knockbackSourceX;
		enemies[index].knockbackSourceY = knockbackSourceY;
	}
}

void killAllNonBossEnemies(void) {
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		if (i == bossIndex) {
			continue;
		}
		Enemy curEnemy = enemies[i];
		if (curEnemy.active && curEnemy.state != ENEMY_STATE_DEAD) {
			EnemyData curEnemyData = enemyData[curEnemy.type];
			curEnemy.state = ENEMY_STATE_DEAD;
			curEnemy.animationFrame = curEnemyData.deathFrameStart - 1;
			curEnemy.animationTime = curEnemyData.frameTime;
			curEnemy.hitFlashTime = 0;
			enemies[i] = curEnemy;
		}
	}
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

		curEnemy.state = ENEMY_STATE_DEAD;
		curEnemy.animationFrame = curEnemyData.deathFrameStart - 1;
		curEnemy.animationTime = curEnemyData.frameTime;
		curEnemy.hitFlashTime = 0;

		if (index == bossIndex) {
			killAllNonBossEnemies();
			setBossActive(false);
			setRoomCompleted(true);
		}

		if (index != bossIndex && !curEnemy.isMinion) {
			enemiesDefeated++;
		}

		if (runeIsOwned(RUNE_TYPE_VAMPIRISM)) {
			healPlayer(getRunePrimaryValue(RUNE_TYPE_VAMPIRISM));
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

void damageEnemiesInArea(float x, float y, int radius, int damage) {
	int radiusSquared = radius * radius;
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

void knockbackEnemiesInArea(float x, float y, int radius, float knockbackTime, float knockbackAmount) {
	int radiusSquared = radius * radius;
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		Enemy enemy = enemies[i];
		if (!enemy.active) {
			continue;
		}
		float distSquared = distanceSquared(enemy.x, enemy.y, x, y);
		if (distSquared < radiusSquared) {
			knockbackEnemy(i, x, y, knockbackTime, knockbackAmount);
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