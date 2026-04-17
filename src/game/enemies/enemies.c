#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/helpers.h"
#include "game/map/map.h"
#include "game/enemies/enemies.h"
#include "game/enemies/enemyAI.h"
#include "game/player/player.h"
#include "game/entities/entities.h"
#include "game/entities/projectiles.h"
#include "game/entities/pickups.h"
#include "game/rendering/textures.h"
#include "game/rendering/particles.h"
#include "game/systems/audioManager.h"
#include "game/systems/runeManager.h"
#include "game/systems/progressionManager.h"
#include "game/systems/waveManager.h"

#define SPAWN_TEST_ENEMY 0

#define HIT_FLASH_TIME 0.1f // Seconds

#define MIN_DIST_TO_PLAYER 5
#define ENEMY_SEPARATION_RADIUS 20
#define ENEMY_SEPARATION_RADIUS_SQ (ENEMY_SEPARATION_RADIUS * ENEMY_SEPARATION_RADIUS)
#define ENEMY_SEPARATION_FORCE 0.3f
#define WALL_CHECK_DISTANCE 4
#define WALL_AVOID_FORCE 0.1f

Enemy enemies[MAX_ENEMY_COUNT] = {0};
static int bossIndex;
static int enemiesDefeated;

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
		.update = proximityEnemyUpdate
	},
	[ENEMY_TYPE_BAT] = (EnemyData){
		.name = "Bat",
		.health = 40,
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
		.update = rushEnemyUpdate
	},
	[ENEMY_TYPE_SLIME] = (EnemyData){
		.name = "Slime",
		.health = 80,
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
		.update = proximityEnemyUpdate
	},
	[ENEMY_TYPE_BABY_SLIME] = (EnemyData){
		.name = "Baby Slime",
		.health = 50,
		.attackDetectionRange = 35,
		.attackDistance = 20,
		.attackDamage = 5,
		.attackCooldown = 2.0f,
		.moveSpeed = 0.2f,
		.prepTime = 0.5f,
		.postAttackTime = 0.5f,
		.textureIndex = TEXTURE_BABY_SLIME,
		.idleFrameEnd = 6,
		.prepFrameStart = 7,
		.prepFrameEnd = 11,
		.attackFrameStart = 12,
		.attackFrameEnd = 17,
		.attackActiveFrame = 16,
		.deathFrameStart = 18,
		.deathFrameEnd = 21,
		.frameTime = 0.1f,
		.update = meleeEnemyUpdate
	},
	[ENEMY_TYPE_SKELETON] = (EnemyData){
		.name = "Skeleton",
		.health = 100,
		.attackDetectionRange = 25,
		.attackDistance = 15,
		.attackDamage = 20,
		.attackCooldown = 1.0f,
		.moveSpeed = 0.25f,
		.prepTime = 1.5f,
		.postAttackTime = 1.0f,
		.textureIndex = TEXTURE_SKELETON,
		.idleFrameEnd = 6,
		.prepFrameStart = 7,
		.prepFrameEnd = 11,
		.attackFrameStart = 12,
		.attackFrameEnd = 15,
		.attackActiveFrame = 12,
		.deathFrameStart = 16,
		.deathFrameEnd = 19,
		.frameTime = 0.12f,
		.update = meleeEnemyUpdate
	},
	[ENEMY_TYPE_SERPENT] = (EnemyData){
		.name = "Serpent",
		.health = 80,
		.attackDetectionRange = 35,
		.attackDamage = 10,
		.attackCooldown = 8.0f,
		.moveSpeed = 0.3f,
		.abilityDataPrimary = 5, // Tail attack count
		.abilityDataSecondary = 0.3f, // Tail attack interval
		.textureIndex = TEXTURE_SERPENT,
		.idleFrameEnd = 12,
		.attackFrameStart = 14,
		.attackFrameEnd = 41,
		.attackActiveFrame = 19,
		.deathFrameStart = 42,
		.deathFrameEnd = 45,
		.frameTime = 0.08f,
		.update = serpentUpdate
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
		.update = turretEnemyUpdate
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
		.update = elderBossUpdate,
		.knockbackResistant = true
	},
	[ENEMY_TYPE_RAT] = (EnemyData){
		.name = "Rat",
		.health = 60,
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
		.update = proximityEnemyUpdate
	},
	[ENEMY_TYPE_IMP] = (EnemyData){
		.name = "Imp",
		.health = 70,
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
		.update = projectileEnemyUpdate
	},
	[ENEMY_TYPE_FLYING_TOME] = (EnemyData){
		.name = "Flying Tome",
		.health = 80,
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
		.update = rushEnemyUpdate
	},
	[ENEMY_TYPE_LIVING_ARMOR] = (EnemyData){
		.name = "Living Armor",
		.health = 120,
		.attackDetectionRange = 22,
		.attackDistance = 25,
		.attackDamage = 30,
		.attackCooldown = 0,
		.moveSpeed = 0.4f,
		.prepTime = 0.5f,
		.postAttackTime = 1.0f,
		.textureIndex = TEXTURE_LIVING_ARMOR,
		.idleFrameEnd = 6,
		.prepFrameStart = 7,
		.prepFrameEnd = 11,
		.attackFrameStart = 12,
		.attackFrameEnd = 16,
		.attackActiveFrame = 12,
		.deathFrameStart = 17,
		.deathFrameEnd = 20,
		.frameTime = 0.1f,
		.update = meleeEnemyUpdate,
		.knockbackResistant = true
	},
	[ENEMY_TYPE_MUMMY] = (EnemyData){
		.name = "Mummy",
		.health = 80,
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
		.update = mummyUpdate
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
		.update = wraithBossUpdate,
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
		.update = proximityEnemyUpdate
	}
};

void moveEnemy(Enemy *enemy, int enemyIndex, float targetX, float targetY, float moveSpeed) {
	float dx = (targetX - enemy->x);
	float dy = (targetY - enemy->y);
	float distToTargetSq = dx * dx + dy * dy;

	if (distToTargetSq < MIN_DIST_TO_PLAYER * MIN_DIST_TO_PLAYER) {
		return;
	}

	float invDistToTarget = fastRsqrt(distToTargetSq);
	dx *= invDistToTarget;
	dy *= invDistToTarget;

	// Repulsion from other enemies
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		if (i == enemyIndex || !enemies[i].active) {
			continue;
		}
		Enemy *other = &enemies[i];

		float otherX = enemy->x - other->x;
		float otherY = enemy->y - other->y;
		float distSquared = otherX * otherX + otherY * otherY;

		if (distSquared < ENEMY_SEPARATION_RADIUS_SQ && distSquared > 0.01f) {
			float invDist = fastRsqrt(distSquared);
			float dist = distSquared * invDist;
			otherX *= invDist;
			otherY *= invDist;
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
	float magnitudeSq = dx * dx + dy * dy;
	if (magnitudeSq > 0.01f) {
		float invMagnitude = fastRsqrt(magnitudeSq);
		dx = dx * invMagnitude * moveSpeed;
		dy = dy * invMagnitude * moveSpeed;
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
		.slowAmount = 0,
		.phase = 0
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

int getAliveNonMinionEnemyCount(void) {
	int count = 0;
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		if (enemies[i].active && enemies[i].state != ENEMY_STATE_DEAD && !enemies[i].isMinion) {
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
		} else if (curEnemy->state == ENEMY_STATE_POST_ATTACK) {
			// Hold last frame of attack animation
			curEnemy->animationFrame = curEnemyData->attackFrameEnd - 1;
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
		float sourceX = enemy->knockbackAwayFromPlayer ? playerX : enemy->knockbackSourceX;
		float sourceY = enemy->knockbackAwayFromPlayer ? playerY : enemy->knockbackSourceY;
		float knockbackX = enemy->x - sourceX;
		float knockbackY = enemy->y - sourceY;
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

		if (curEnemyData->update) {
			curEnemyData->update(curEnemy, curEnemyData, i, attackAnimationReset);
		}

		addEntityToRenderQueue(curEnemy->x, curEnemy->y, ENEMY, i, curEnemyData->textureIndex, curEnemy->animationFrame);
	}
}

bool enemyDeathEffect(int index) {
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
		return true;
	}
	return false;
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

void knockbackEnemy(int index, int knockbackSourceX, int knockbackSourceY, float knockbackTime, float knockbackAmount, bool awayFromPlayer) {
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
		enemies[index].knockbackAwayFromPlayer = awayFromPlayer;
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
	signalRuneTrigger(RUNE_TRIGGER_DAMAGE_ENEMY, 1.0f);
	curEnemy.health -= damage;
	playSound(SFX_ENEMY_HIT);

	if (curEnemy.health <= 0) {
		signalRuneTrigger(RUNE_TRIGGER_KILL_ENEMY, 1.0f);
		playSound(SFX_ENEMY_DEATH);
		bool summonsMinions = enemyDeathEffect(index);

		curEnemy.state = ENEMY_STATE_DEAD;
		curEnemy.animationFrame = curEnemyData.deathFrameStart - 1;
		curEnemy.animationTime = curEnemyData.frameTime;
		curEnemy.hitFlashTime = 0;

		if (index == bossIndex) {
			killAllNonBossEnemies();
			setBossActive(false);
			setRoomCompleted(true);
		} else if (!summonsMinions) {
			spawnPickup(curEnemy.x, curEnemy.y, PICKUP_TYPE_XP_ORB);
		}

		if (index != bossIndex && !curEnemy.isMinion) {
			enemiesDefeated++;
		}

		if (runeIsOwned(RUNE_TYPE_VAMPIRISM)) {
			healPlayer(getRuneValue(RUNE_TYPE_VAMPIRISM));
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

void knockbackEnemiesInArea(float x, float y, int radius, float knockbackTime, float knockbackAmount, bool awayFromPlayer) {
	int radiusSquared = radius * radius;
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		Enemy enemy = enemies[i];
		if (!enemy.active) {
			continue;
		}
		float distSquared = distanceSquared(enemy.x, enemy.y, x, y);
		if (distSquared < radiusSquared) {
			knockbackEnemy(i, x, y, knockbackTime, knockbackAmount, awayFromPlayer);
		}
	}
}

void slowEnemiesInArea(float x, float y, int radius, float slowTime, float slowAmount) {
	int radiusSquared = radius * radius;
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		Enemy enemy = enemies[i];
		if (!enemy.active) {
			continue;
		}
		float distSquared = distanceSquared(enemy.x, enemy.y, x, y);
		if (distSquared < radiusSquared) {
			slowEnemy(i, slowTime, slowAmount);
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

