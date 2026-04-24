#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "game/map/map.h"
#include "game/enemies/enemyAI.h"
#include "game/player/player.h"
#include "game/entities/projectiles.h"
#include "game/rendering/particles.h"
#include "game/systems/waveManager.h"
#include "game/systems/bossCinematic.h"
#include "game/UI/UI.h"

#define BOSS_DIFFICULTY_COUNT 2
#define BOSS_PHASE_COUNT      2

// === Elder Boss properties ===
typedef struct ElderBossProperties {
	float idleTime;
	float prepTime;
	int   attack2Count;
	float attack2Spacing;
	float dashPrepTime;
	float dashDistance;
	int   dashCount;
	float dashSpeed;
	int   projectileWaves;
	int   projectilesPerWave;
	float projectileCooldown;
	int   projectileDamage;
	float projectileSpeed;
	float waveAngleOffset;
	int   basicSpawnCount;
	int   eliteSpawnCount;
	int   specialSpawnCount;
	int   tankSpawnCount;
	float summonRadius;
	float summonTime;
} ElderBossProperties;

static const ElderBossProperties ELDER_BOSS_PROPERTIES[BOSS_DIFFICULTY_COUNT][BOSS_PHASE_COUNT] = {
	// Difficulty 0
	{
		// Phase 0
		{
			.idleTime           = 5.0f,
			.prepTime           = 5.0f,
			.attack2Count       = 5,
			.attack2Spacing     = CELL_SIZE,
			.dashPrepTime       = 1.5f,
			.dashDistance       = CELL_SIZE * 2.0f,
			.dashCount          = 5,
			.dashSpeed          = 2.0f,
			.projectileWaves    = 5,
			.projectilesPerWave = 8,
			.projectileCooldown = 0.2f,
			.projectileDamage   = 10,
			.projectileSpeed    = 25.0f,
			.waveAngleOffset    = 5.0f * PI / 180.0f,
			.basicSpawnCount    = 3,
			.eliteSpawnCount    = 1,
			.specialSpawnCount  = 1,
			.tankSpawnCount     = 0,
			.summonRadius       = CELL_SIZE * 2.0f,
			.summonTime         = 2.0f,
		},
		// Phase 1 — placeholder copy of phase 0
		{
			.idleTime           = 5.0f,
			.prepTime           = 5.0f,
			.attack2Count       = 5,
			.attack2Spacing     = CELL_SIZE,
			.dashPrepTime       = 1.5f,
			.dashDistance       = CELL_SIZE * 2.0f,
			.dashCount          = 5,
			.dashSpeed          = 2.0f,
			.projectileWaves    = 5,
			.projectilesPerWave = 8,
			.projectileCooldown = 0.2f,
			.projectileDamage   = 10,
			.projectileSpeed    = 25.0f,
			.waveAngleOffset    = 5.0f * PI / 180.0f,
			.basicSpawnCount    = 3,
			.eliteSpawnCount    = 1,
			.specialSpawnCount  = 1,
			.tankSpawnCount     = 0,
			.summonRadius       = CELL_SIZE * 2.0f,
			.summonTime         = 2.0f,
		},
	},
	// Difficulty 1 — placeholder copies of difficulty 0
	{
		{
			.idleTime           = 5.0f,
			.prepTime           = 5.0f,
			.attack2Count       = 5,
			.attack2Spacing     = CELL_SIZE,
			.dashPrepTime       = 1.5f,
			.dashDistance       = CELL_SIZE * 2.0f,
			.dashCount          = 5,
			.dashSpeed          = 2.0f,
			.projectileWaves    = 5,
			.projectilesPerWave = 8,
			.projectileCooldown = 0.2f,
			.projectileDamage   = 10,
			.projectileSpeed    = 25.0f,
			.waveAngleOffset    = 5.0f * PI / 180.0f,
			.basicSpawnCount    = 3,
			.eliteSpawnCount    = 1,
			.specialSpawnCount  = 1,
			.tankSpawnCount     = 0,
			.summonRadius       = CELL_SIZE * 2.0f,
			.summonTime         = 2.0f,
		},
		{
			.idleTime           = 5.0f,
			.prepTime           = 5.0f,
			.attack2Count       = 5,
			.attack2Spacing     = CELL_SIZE,
			.dashPrepTime       = 1.5f,
			.dashDistance       = CELL_SIZE * 2.0f,
			.dashCount          = 5,
			.dashSpeed          = 2.0f,
			.projectileWaves    = 5,
			.projectilesPerWave = 8,
			.projectileCooldown = 0.2f,
			.projectileDamage   = 10,
			.projectileSpeed    = 25.0f,
			.waveAngleOffset    = 5.0f * PI / 180.0f,
			.basicSpawnCount    = 3,
			.eliteSpawnCount    = 1,
			.specialSpawnCount  = 1,
			.tankSpawnCount     = 0,
			.summonRadius       = CELL_SIZE * 2.0f,
			.summonTime         = 2.0f,
		},
	},
};

static const ElderBossProperties *elderProperties;

#define ELDER_BOSS_PROJECTILE_INTENT_TIME 0.5f
#define ELDER_BOSS_DASH_PREP_HOLD_FRAME 25  // 0-indexed
#define ELDER_BOSS_DASH_EXEC_HOLD_FRAME 28  // 0-indexed

typedef enum ElderAttackCycle {
	ELDER_ATTACK_CYCLE_SUMMON,
	ELDER_ATTACK_CYCLE_ROOT,
	ELDER_ATTACK_CYCLE_PROJECTILES,
	ELDER_ATTACK_CYCLE_DASH,
	ELDER_ATTACK_CYCLE_COUNT
} ElderAttackCycle;

// Phase state aliases. DASH_PREP shares ATTACK_1; ROOT_CAST/DASH_EXEC share ATTACK_2.
#define ELDER_STATE_DASH_PREP  ENEMY_STATE_ATTACK_1
#define ELDER_STATE_ROOT_CAST  ENEMY_STATE_ATTACK_2
#define ELDER_STATE_DASH_EXEC  ENEMY_STATE_ATTACK_2
#define ELDER_STATE_PROJECTILE ENEMY_STATE_ATTACK_3

// === Wraith Boss properties ===
typedef struct WraithBossProperties {
	float idleTime;
	float prepTime;
	float dashTime;
	float projectileTime;
	float summonTime;
	int   rushCount;
	float projectileCooldown;
} WraithBossProperties;

static const WraithBossProperties WRAITH_BOSS_PROPERTIES[BOSS_DIFFICULTY_COUNT][BOSS_PHASE_COUNT] = {
	// Difficulty 0
	{
		// Phase 0
		{
			.idleTime           = 2.0f,
			.prepTime           = 1.0f,
			.dashTime           = 2.5f,
			.projectileTime     = 2.0f,
			.summonTime         = 5.0f,
			.rushCount          = 3,
			.projectileCooldown = 0.5f,
		},
		// Phase 1 — placeholder copy of phase 0
		{
			.idleTime           = 2.0f,
			.prepTime           = 1.0f,
			.dashTime           = 2.5f,
			.projectileTime     = 2.0f,
			.summonTime         = 5.0f,
			.rushCount          = 3,
			.projectileCooldown = 0.5f,
		},
	},
	// Difficulty 1 — placeholder copies of difficulty 0
	{
		{
			.idleTime           = 2.0f,
			.prepTime           = 1.0f,
			.dashTime           = 2.5f,
			.projectileTime     = 2.0f,
			.summonTime         = 5.0f,
			.rushCount          = 3,
			.projectileCooldown = 0.5f,
		},
		{
			.idleTime           = 2.0f,
			.prepTime           = 1.0f,
			.dashTime           = 2.5f,
			.projectileTime     = 2.0f,
			.summonTime         = 5.0f,
			.rushCount          = 3,
			.projectileCooldown = 0.5f,
		},
	},
};

static const WraithBossProperties *wraithProperties;

#define WRAITH_STATE_DASH        ENEMY_STATE_ATTACK_1
#define WRAITH_STATE_PROJECTILE  ENEMY_STATE_ATTACK_2
#define WRAITH_STATE_SUMMON      ENEMY_STATE_ATTACK_3

void proximityEnemyUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	// Constantly attack nearby player
	if (playerInAttackDistance(index)) {
		if (curEnemy->attackCooldown <= 0) {
			curEnemy->attackCooldown = curEnemyData->attackCooldown;
			damagePlayerFromDirection(curEnemyData->attackDamage, curEnemy->x, curEnemy->y);
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
		// Begin prep phase if player is in range and cooldown is ready
		if (playerInAttackDetectionRange(index)) {
			if (curEnemy->attackCooldown <= 0) {
				curEnemy->attackCooldown = curEnemyData->attackCooldown;
				curEnemy->stateTimer = curEnemyData->prepTime;
				transitionToState(curEnemy, ENEMY_STATE_PREP);
				addAttackIntentIndicator(curEnemy->x, curEnemy->y, curEnemyData->prepTime);
			}
		} else {
			curEnemy->attackCooldown = curEnemyData->attackCooldown;
		}

		// Move towards player
		moveEnemy(curEnemy, index, playerX, playerY, curEnemyData->moveSpeed);
	} else if (curEnemy->state == ENEMY_STATE_PREP) {
		if (curEnemy->stateTimer <= 0) {
			// Lock in lunge direction toward the player at commit time
			if (curEnemyData->abilityDataPrimary > 0) {
				curEnemy->targetX = playerX;
				curEnemy->targetY = playerY;
				curEnemy->abilityCooldown = curEnemyData->abilityDataSecondary;
			}
			transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
		}
	} else if (curEnemy->state == ENEMY_STATE_ATTACK_1) {
		// Brief lunge toward locked target while the lunge timer is active
		if (curEnemy->abilityCooldown > 0) {
			moveEnemy(curEnemy, index, curEnemy->targetX, curEnemy->targetY, curEnemyData->abilityDataPrimary);
		}

		// Damage player once attack animation passes active frame
		if (curEnemy->animationFrame >= curEnemyData->attackActiveFrame) {
			if (!curEnemy->attackedPlayer && playerInAttackDistance(index)) {
				curEnemy->attackedPlayer = true;
				damagePlayerFromDirection(curEnemyData->attackDamage, curEnemy->x, curEnemy->y);
			}
		}

		// Transition to post attack state after attack animation ends
		if (attackAnimationReset) {
			curEnemy->attackedPlayer = false;
			curEnemy->attackCooldown = curEnemyData->attackCooldown;
			curEnemy->stateTimer = curEnemyData->postAttackTime;
			curEnemy->animationFrame = curEnemyData->attackFrameEnd - 1;
			transitionToState(curEnemy, ENEMY_STATE_POST_ATTACK);
		}
	} else if (curEnemy->state == ENEMY_STATE_POST_ATTACK) {
		if (curEnemy->stateTimer <= 0) {
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
			addAttackIntentIndicator(curEnemy->x, curEnemy->y, 0.0f);
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
			fireEnemyProjectile(projectileX, projectileY, curEnemy->x, curEnemy->y, deltaX, deltaY, projectileSpeed, damage);
		}

		// Transition back to idle state after attack animation ends
		if (attackAnimationReset) {
			curEnemy->attackedPlayer = false;
			transitionToState(curEnemy, ENEMY_STATE_IDLE);
		}
	}
}

void rushEnemyUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
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
			addAttackIntentIndicator(curEnemy->x, curEnemy->y, 0.0f);
		}

		// Move towards player at regular speed when idle
		moveEnemy(curEnemy, index, playerX, playerY, curEnemyData->moveSpeed);
	} else if (curEnemy->state == ENEMY_STATE_ATTACK_1) {
		// Damage player if nearby while rushing
		if (playerInAttackDistance(index)) {
			damagePlayerFromDirection(curEnemyData->attackDamage, curEnemy->x, curEnemy->y);
		}

		// Once rush timer is over, transition back to idle state
		if (curEnemy->abilityCooldown <= 0) {
			curEnemy->attackCooldown = curEnemyData->attackCooldown;
			transitionToState(curEnemy, ENEMY_STATE_IDLE);
		}

		// Decelerate from initial speed to zero over the rush duration (quadratic falloff)
		float totalDuration = curEnemyData->abilityDataSecondary;
		float progress = 1.0f - (curEnemy->abilityCooldown / totalDuration);
		if (progress < 0.0f) {
			progress = 0.0f;
		}
		if (progress > 1.0f) {
			progress = 1.0f;
		}
		float remaining = 1.0f - progress;
		float speed = curEnemyData->abilityDataPrimary * remaining * remaining;
		moveEnemy(curEnemy, index, curEnemy->targetX, curEnemy->targetY, speed);
	}
}

void mummyUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	if (curEnemy->state == ENEMY_STATE_IDLE) {
		if (curEnemy->attackCooldown <= 0 && playerInAttackDetectionRange(index)) {
			curEnemy->attackCooldown = curEnemyData->attackCooldown;
			transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
		}

		moveEnemy(curEnemy, index, playerX, playerY, curEnemyData->moveSpeed);
	} else if (curEnemy->state == ENEMY_STATE_ATTACK_1) {
		if (curEnemy->animationFrame >= curEnemyData->attackActiveFrame) {
			if (!curEnemy->attackedPlayer) {
				curEnemy->attackedPlayer = true;
				spawnEnemy(ENEMY_TYPE_WRAITH, curEnemy->x, curEnemy->y, true /*isMinion*/);
			}
		}

		if (attackAnimationReset) {
			curEnemy->attackedPlayer = false;
			curEnemy->stateTimer = 0;
			curEnemy->stateCount = 0;
			transitionToState(curEnemy, ENEMY_STATE_IDLE);
		}
	}
}

// === Serpent constants ===
#define SERPENT_TAIL_SPREAD_ANGLE_DEG 30.0f
#define SERPENT_TAIL_SPREAD_ANGLE_RAD (SERPENT_TAIL_SPREAD_ANGLE_DEG * PI / 180.0f)

void serpentUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	if (curEnemy->state == ENEMY_STATE_IDLE) {
		if (curEnemy->attackCooldown <= 0 && playerInAttackDetectionRange(index)) {
			curEnemy->attackCooldown = curEnemyData->attackCooldown;
			transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
			addAttackIntentIndicator(curEnemy->x, curEnemy->y, 0.0f);
		}

		moveEnemy(curEnemy, index, playerX, playerY, curEnemyData->moveSpeed);
	} else if (curEnemy->state == ENEMY_STATE_ATTACK_1) {
		// Spawn tail attack particles at intervals during attack animation
		if (curEnemy->animationFrame >= curEnemyData->attackActiveFrame) {
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
				float distance = (curEnemy->stateCount + 1) * CELL_SIZE / 2;
				float offsetX = dx * distance;
				float offsetY = dy * distance;
				spawnParticle(PARTICLE_TYPE_SERPENT_TAIL, curEnemy->x + offsetX, curEnemy->y + offsetY, curEnemy->x, curEnemy->y, PARTICLE_TYPE_NONE, true /* enemyParticle */);

				// (Use when increasing enemy difficulty) Fire off two additional tail particles spread out at angles from the main tail
				// float cosSpread = cosf(SERPENT_TAIL_SPREAD_ANGLE_RAD);
				// float sinSpread = sinf(SERPENT_TAIL_SPREAD_ANGLE_RAD);
				// float leftDx = dx * cosSpread - dy * sinSpread;
				// float leftDy = dx * sinSpread + dy * cosSpread;
				// spawnParticle(PARTICLE_TYPE_SERPENT_TAIL, curEnemy->x + leftDx * distance, curEnemy->y + leftDy * distance, curEnemy->x, curEnemy->y, PARTICLE_TYPE_NONE, true /* enemyParticle */);

				// float rightDx = dx * cosSpread + dy * sinSpread;
				// float rightDy = -dx * sinSpread + dy * cosSpread;
				// spawnParticle(PARTICLE_TYPE_SERPENT_TAIL, curEnemy->x + rightDx * distance, curEnemy->y + rightDy * distance, curEnemy->x, curEnemy->y, PARTICLE_TYPE_NONE, true /* enemyParticle */);
				
				curEnemy->stateCount++;
				curEnemy->stateTimer = curEnemyData->abilityDataSecondary;
			}
		}

		if (attackAnimationReset) {
			curEnemy->attackedPlayer = false;
			curEnemy->stateTimer = 0;
			curEnemy->stateCount = 0;
			transitionToState(curEnemy, ENEMY_STATE_IDLE);
		}
	}
}

void turretEnemyUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
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
			fireEnemyProjectile(projectileX, projectileY, curEnemy->x, curEnemy->y, deltaX, deltaY, projectileSpeed, damage);

			curEnemy->stateCount += 1;
			if (curEnemy->stateCount >= curEnemyData->abilityDataPrimary) {
				curEnemy->stateCount = 0;
			}
		}
	}
}

// Forward declarations for charge helpers defined later in this file.
static void setupCharge(Enemy *curEnemy, EnemyData *curEnemyData, float chargeDistance, float chargeSpeed);
static float chargeSpeedAt(float progressRatio, float maxSpeed);

static void elderSpawnRootWarnings(Enemy *curEnemy) {
	for (int dir = 0; dir < 8; dir++) {
		float angle = dir * (PI / 4);
		float dx = cosf(angle);
		float dy = sinf(angle);
		for (int root = 1; root <= elderProperties->attack2Count; root++) {
			float offsetX = dx * (float)(root * elderProperties->attack2Spacing);
			float offsetY = dy * (float)(root * elderProperties->attack2Spacing);
			spawnParticle(PARTICLE_TYPE_WARNING, curEnemy->x + offsetX, curEnemy->y + offsetY, curEnemy->x, curEnemy->y, PARTICLE_TYPE_BOSS_ROOT_ATTACK, true /*enemyParticle*/);
		}
	}
}

static void elderSpawnSummons(Enemy *curEnemy) {
	const int counts[4] = {
		elderProperties->basicSpawnCount,
		elderProperties->eliteSpawnCount,
		elderProperties->specialSpawnCount,
		elderProperties->tankSpawnCount,
	};
	for (int role = 0; role < 4; role++) {
		for (int i = 0; i < counts[role]; i++) {
			float angle = randFloat(0.0f, 2.0f * PI);
			float radius = randFloat(0.0f, elderProperties->summonRadius);
			float sx = curEnemy->x + cosf(angle) * radius;
			float sy = curEnemy->y + sinf(angle) * radius;
			EnemyType type;
			switch (role) {
				case 1: {
					type = getRandomEliteEnemyType();
					break;
				}
				case 2: {
					type = getRandomSpecialEnemyType();
					break;
				}
				case 3: {
					type = getRandomTankEnemyType();
					break;
				}
				default: {
					type = getRandomBasicEnemyType();
					break;
				}
			}
			spawnEnemy(type, (int)sx, (int)sy, true /*isMinion*/);
		}
	}
}

static void elderFireProjectileWave(Enemy *curEnemy, EnemyData *curEnemyData, int waveIndex) {
	float angleOffset = (float)waveIndex * elderProperties->waveAngleOffset;
	for (int i = 0; i < elderProperties->projectilesPerWave; i++) {
		float angle = angleOffset + i * (2.0f * PI / (float)elderProperties->projectilesPerWave);
		float dx = cosf(angle);
		float dy = sinf(angle);
		float px = curEnemy->x + dx * PROJECTILE_OFFSET;
		float py = curEnemy->y + dy * PROJECTILE_OFFSET;
		fireEnemyProjectile(px, py, curEnemy->x, curEnemy->y, dx, dy, elderProperties->projectileSpeed, elderProperties->projectileDamage);
	}
}

static void elderBeginNextAttackCycle(Enemy *curEnemy) {
	curEnemy->attackCycle = (curEnemy->attackCycle + 1) % ELDER_ATTACK_CYCLE_COUNT;
	curEnemy->stateCount = 0;
	curEnemy->attackedPlayer = false;

	switch ((ElderAttackCycle)curEnemy->attackCycle) {
		case ELDER_ATTACK_CYCLE_SUMMON: {
			transitionToState(curEnemy, ENEMY_STATE_PREP);
			curEnemy->stateTimer = elderProperties->prepTime;
			elderSpawnSummons(curEnemy);
			break;
		}
		case ELDER_ATTACK_CYCLE_ROOT: {
			transitionToState(curEnemy, ENEMY_STATE_PREP);
			curEnemy->stateTimer = elderProperties->prepTime;
			addAttackIntentIndicator(curEnemy->x, curEnemy->y, elderProperties->prepTime);
			elderSpawnRootWarnings(curEnemy);
			break;
		}
		case ELDER_ATTACK_CYCLE_PROJECTILES: {
			transitionToState(curEnemy, ENEMY_STATE_PREP);
			curEnemy->abilityCooldown = ELDER_BOSS_PROJECTILE_INTENT_TIME; // delay wave 0 until the warning indicator lands
			addAttackIntentIndicator(curEnemy->x, curEnemy->y, ELDER_BOSS_PROJECTILE_INTENT_TIME);
			break;
		}
		case ELDER_ATTACK_CYCLE_DASH: {
			transitionToState(curEnemy, ELDER_STATE_DASH_PREP);
			curEnemy->stateTimer = elderProperties->dashPrepTime;
			addAttackIntentIndicator(curEnemy->x, curEnemy->y, elderProperties->dashPrepTime);
			break;
		}
		default: {
			break;
		}
	}
}

void elderBossUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	if (isBossCinematicActive()) {
		return;
	}
	elderProperties = &ELDER_BOSS_PROPERTIES[0][curEnemy->phase];

	if (curEnemy->state == ENEMY_STATE_IDLE) {
		if (curEnemy->stateTimer <= 0) {
			elderBeginNextAttackCycle(curEnemy);
		} else {
			moveEnemy(curEnemy, index, playerX, playerY, curEnemyData->moveSpeed);
		}
		return;
	}

	ElderAttackCycle attackCycle = (ElderAttackCycle)curEnemy->attackCycle;

	if (curEnemy->state == ENEMY_STATE_PREP) {
		if (attackCycle == ELDER_ATTACK_CYCLE_SUMMON) {
			// PREP state holds on prepFrameEnd-1 and never sets attackAnimationReset,
			// so exit on a timer instead.
			if (curEnemy->stateTimer <= 0) {
				transitionToState(curEnemy, ENEMY_STATE_IDLE);
				curEnemy->stateTimer = elderProperties->idleTime;
			}
		} else if (attackCycle == ELDER_ATTACK_CYCLE_PROJECTILES) {
			if (curEnemy->stateCount < elderProperties->projectileWaves && curEnemy->abilityCooldown <= 0) {
				elderFireProjectileWave(curEnemy, curEnemyData, curEnemy->stateCount);
				curEnemy->stateCount++;
				curEnemy->abilityCooldown = elderProperties->projectileCooldown;
			}
			if (curEnemy->stateCount >= elderProperties->projectileWaves && curEnemy->abilityCooldown <= 0) {
				transitionToState(curEnemy, ENEMY_STATE_IDLE);
				curEnemy->stateTimer = elderProperties->idleTime;
			}
		} else {
			// Root: warning particles were spawned on entry; wait out the prep then rest.
			if (curEnemy->stateTimer <= 0) {
				transitionToState(curEnemy, ENEMY_STATE_IDLE);
				curEnemy->stateTimer = elderProperties->idleTime;
			}
		}
		return;
	}

	if (curEnemy->state == ENEMY_STATE_ATTACK_1) {
		if (attackCycle == ELDER_ATTACK_CYCLE_DASH) {
			// Dash prep: hold animation on frame 21 (0-indexed 20)
			if (curEnemy->animationFrame > ELDER_BOSS_DASH_PREP_HOLD_FRAME) {
				curEnemy->animationFrame = ELDER_BOSS_DASH_PREP_HOLD_FRAME;
			}
			if (curEnemy->stateTimer <= 0) {
				setupCharge(curEnemy, curEnemyData, elderProperties->dashDistance, elderProperties->dashSpeed);
				transitionToState(curEnemy, ELDER_STATE_DASH_EXEC);
				// Resume mid-animation at frame 21 so the next tick lands on frame 22.
				curEnemy->animationFrame = ELDER_BOSS_DASH_PREP_HOLD_FRAME;
				curEnemy->animationTime = curEnemyData->frameTime;
			}
		}
		return;
	}

	if (curEnemy->state == ENEMY_STATE_ATTACK_2) {
		if (attackCycle == ELDER_ATTACK_CYCLE_ROOT) {
			if (attackAnimationReset) {
				transitionToState(curEnemy, ENEMY_STATE_IDLE);
				curEnemy->stateTimer = elderProperties->idleTime;
			}
		} else if (attackCycle == ELDER_ATTACK_CYCLE_DASH) {
			// Dash attack plays frames 22-24 and holds on frame 24 for the rest of the dash.
			if (curEnemy->animationFrame > ELDER_BOSS_DASH_EXEC_HOLD_FRAME) {
				curEnemy->animationFrame = ELDER_BOSS_DASH_EXEC_HOLD_FRAME;
			}
			float totalDuration = -curEnemy->abilityCooldown;
			if (totalDuration > 0.001f) {
				float progressRatio = 1.0f - curEnemy->stateTimer / totalDuration;
				float speed = chargeSpeedAt(progressRatio, elderProperties->dashSpeed);
				moveEnemy(curEnemy, index, curEnemy->targetX, curEnemy->targetY, speed);
			}
			if (!curEnemy->attackedPlayer && playerInAttackDistance(index)) {
				curEnemy->attackedPlayer = true;
				damagePlayerFromDirection(curEnemyData->attackDamage, curEnemy->x, curEnemy->y);
			}
			if (curEnemy->stateTimer <= 0.0f) {
				curEnemy->stateCount++;
				if (curEnemy->stateCount < elderProperties->dashCount) {
					transitionToState(curEnemy, ELDER_STATE_DASH_PREP);
					curEnemy->stateTimer = elderProperties->dashPrepTime;
					curEnemy->attackedPlayer = false;
					addAttackIntentIndicator(curEnemy->x, curEnemy->y, elderProperties->dashPrepTime);
				} else {
					transitionToState(curEnemy, ENEMY_STATE_IDLE);
					curEnemy->stateTimer = elderProperties->idleTime;
				}
			}
		}
		return;
	}

}

void wraithBossUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	if (isBossCinematicActive()) {
		return;
	}
	wraithProperties = &WRAITH_BOSS_PROPERTIES[0][curEnemy->phase];

	if (curEnemy->attackCooldown <= 0 && playerInAttackDistance(index)) {
		curEnemy->attackCooldown = curEnemyData->attackCooldown;
		damagePlayerFromDirection(curEnemyData->attackDamage, curEnemy->x, curEnemy->y);
	}

	if (curEnemy->state == ENEMY_STATE_IDLE) {
		if (curEnemy->stateTimer <= 0) {
			transitionToState(curEnemy, ENEMY_STATE_PREP);
			curEnemy->stateTimer = wraithProperties->prepTime;
			addAttackIntentIndicator(curEnemy->x, curEnemy->y, wraithProperties->prepTime);
		}
	} else if (curEnemy->state == ENEMY_STATE_PREP) {
		if (curEnemy->stateTimer <= 0) {
			if (curEnemy->nextState == WRAITH_STATE_DASH) {
				// == Dash attack ==
				transitionToState(curEnemy, WRAITH_STATE_DASH);
				curEnemy->stateTimer = wraithProperties->dashTime;
				curEnemy->stateCount++;

				// Project line towards player and set target to first wall hit
				float targetX, targetY;
				castRayToWall(curEnemy->x, curEnemy->y, playerX, playerY, &targetX, &targetY);
				curEnemy->targetX = targetX;
				curEnemy->targetY = targetY;

				// Rush attack 3 times
				if (curEnemy->stateCount >= wraithProperties->rushCount) {
					curEnemy->stateCount = 0;
					curEnemy->nextState = WRAITH_STATE_PROJECTILE;
				}
			} else if (curEnemy->nextState == WRAITH_STATE_PROJECTILE) {
				// == Projectile attack ==
				transitionToState(curEnemy, WRAITH_STATE_PROJECTILE);
				curEnemy->stateTimer = wraithProperties->projectileTime;
				curEnemy->nextState = WRAITH_STATE_SUMMON;
			} else if (curEnemy->nextState == WRAITH_STATE_SUMMON) {
				// == Summon wraiths ==
				transitionToState(curEnemy, ENEMY_STATE_IDLE);
				curEnemy->stateTimer = wraithProperties->summonTime;
				curEnemy->nextState = WRAITH_STATE_DASH;
				Point bossRoomCenter = getRoomWorldCenter(DUNGEON_ROOM_COUNT - 1);
				curEnemy->targetX = bossRoomCenter.x;
				curEnemy->targetY = bossRoomCenter.y;
				spawnEnemy(ENEMY_TYPE_WRAITH, curEnemy->x, curEnemy->y, true /*isMinion*/);
				spawnEnemy(ENEMY_TYPE_WRAITH, curEnemy->x - CELL_SIZE, curEnemy->y, true /*isMinion*/);
				spawnEnemy(ENEMY_TYPE_WRAITH, curEnemy->x + CELL_SIZE, curEnemy->y, true /*isMinion*/);
			}
		}
	} else if (curEnemy->state == WRAITH_STATE_DASH || curEnemy->state == WRAITH_STATE_PROJECTILE || curEnemy->state == WRAITH_STATE_SUMMON) {
		if (curEnemy->stateTimer <= 0) {
			transitionToState(curEnemy, ENEMY_STATE_IDLE);
			curEnemy->stateTimer = wraithProperties->idleTime;
		}

		if (curEnemy->state == WRAITH_STATE_DASH) {
			moveEnemy(curEnemy, index, curEnemy->targetX, curEnemy->targetY, curEnemyData->abilityDataPrimary);
		} else if (curEnemy->state == WRAITH_STATE_PROJECTILE) {
			if (curEnemy->attackCooldown <= 0) {
				curEnemy->attackCooldown = wraithProperties->projectileCooldown;
				float distanceToPlayer = sqrtf(distanceSquared(playerX, playerY, curEnemy->x, curEnemy->y));
				float deltaX = (playerX - curEnemy->x) / distanceToPlayer;
				float deltaY = (playerY - curEnemy->y) / distanceToPlayer;
				float projectileSpeed = curEnemyData->abilityDataSecondary;
				float damage = curEnemyData->attackDamage;
				float projectileX = curEnemy->x + deltaX * PROJECTILE_OFFSET;
				float projectileY = curEnemy->y + deltaY * PROJECTILE_OFFSET;
				fireEnemyProjectile(projectileX, projectileY, curEnemy->x, curEnemy->y, deltaX, deltaY, projectileSpeed, damage);
			}
		} else if (curEnemy->state == WRAITH_STATE_SUMMON) {
			moveEnemy(curEnemy, index, curEnemy->targetX, curEnemy->targetY, curEnemyData->moveSpeed);
		}
	}
}

// === Circling charge constants ===
#define CIRCLING_RANGE   48.0f   // ~3 cells: distance at which enemy starts circling/pausing
#define ORBIT_RADIUS     48.0f   // orbit distance from player during circling
#define ORBIT_LEAD       0.15f   // radians ahead of current angle for orbit target (must keep target > 5 units from enemy)

// Trapezoidal speed curve: accelerate 0-25%, coast 25-75%, decelerate 75-100%.
static float chargeSpeedAt(float progressRatio, float maxSpeed) {
    if (progressRatio < 0.0f) { progressRatio = 0.0f; }
    if (progressRatio > 1.0f) { progressRatio = 1.0f; }
    if (progressRatio < 0.25f) {
        return maxSpeed * (progressRatio / 0.25f);
    } else if (progressRatio < 0.75f) {
        return maxSpeed;
    } else {
        return maxSpeed * ((1.0f - progressRatio) / 0.25f);
    }
}

static void setupCharge(Enemy *curEnemy, EnemyData *curEnemyData, float chargeDistance, float chargeSpeed) {
    float wallX, wallY;
    castRayToWall(curEnemy->x, curEnemy->y, playerX, playerY, &wallX, &wallY);
    float dx = wallX - curEnemy->x;
    float dy = wallY - curEnemy->y;
    float wallDist = sqrtf(dx * dx + dy * dy);
    float dist = (chargeDistance < wallDist) ? chargeDistance : wallDist;
    if (wallDist > 0.0f) {
        float nx = dx / wallDist;
        float ny = dy / wallDist;
        curEnemy->targetX = curEnemy->x + nx * dist;
        curEnemy->targetY = curEnemy->y + ny * dist;
    } else {
        curEnemy->targetX = curEnemy->x;
        curEnemy->targetY = curEnemy->y;
    }
    curEnemy->attackedPlayer = false;
    // Trapezoidal (accel+coast+decel) total time estimate: 1.5 × (dist / speed_per_frame / fps)
    float totalDuration = dist / (chargeSpeed * (float)TARGET_FPS) * 1.5f;
    curEnemy->stateTimer = totalDuration;      // countdown: 0 signals charge end
    curEnemy->abilityCooldown = -totalDuration; // negative so auto-decrement is skipped; used as reference in ATTACK_1
}

static void chargeEnemyIdle(Enemy *curEnemy, EnemyData *curEnemyData, int index) {
    float distSq = distanceSquared(curEnemy->x, curEnemy->y, playerX, playerY);
    if (distSq <= CIRCLING_RANGE * CIRCLING_RANGE) {
        curEnemy->stateCount = (randInt(0, 1) == 0) ? 1 : -1;
        curEnemy->stateTimer = curEnemyData->prepTime;
        transitionToState(curEnemy, ENEMY_STATE_PREP);
        addAttackIntentIndicator(curEnemy->x, curEnemy->y, curEnemy->stateTimer);
    } else {
        moveEnemy(curEnemy, index, playerX, playerY, curEnemyData->moveSpeed);
    }
}

static void chargeEnemyAttack(Enemy *curEnemy, EnemyData *curEnemyData, int index) {
    if (playerInAttackDistance(index) && !curEnemy->attackedPlayer) {
        curEnemy->attackedPlayer = true;
        damagePlayerFromDirection(curEnemyData->attackDamage, curEnemy->x, curEnemy->y);
    }
    float totalDuration = -curEnemy->abilityCooldown;
    if (totalDuration < 0.001f) {
        curEnemy->stateTimer = curEnemyData->postAttackTime;
        transitionToState(curEnemy, ENEMY_STATE_POST_ATTACK);
        return;
    }
    float progressRatio = 1.0f - curEnemy->stateTimer / totalDuration;
    float speed = chargeSpeedAt(progressRatio, curEnemyData->abilityDataPrimary);
    moveEnemy(curEnemy, index, curEnemy->targetX, curEnemy->targetY, speed);
    if (curEnemy->stateTimer <= 0.0f) {
        curEnemy->stateTimer = curEnemyData->postAttackTime;
        transitionToState(curEnemy, ENEMY_STATE_ATTACK_2);
    }
}

void circlingChargeEnemyUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
    if (curEnemy->state == ENEMY_STATE_IDLE) {
        chargeEnemyIdle(curEnemy, curEnemyData, index);
    } else if (curEnemy->state == ENEMY_STATE_PREP) {
        float dx = curEnemy->x - playerX;
        float dy = curEnemy->y - playerY;
        float angle = atan2f(dy, dx) + ORBIT_LEAD * curEnemy->stateCount;
        float orbitX = playerX + cosf(angle) * ORBIT_RADIUS;
        float orbitY = playerY + sinf(angle) * ORBIT_RADIUS;
        moveEnemy(curEnemy, index, orbitX, orbitY, curEnemyData->moveSpeed);

        if (curEnemy->stateTimer <= 0) {
            setupCharge(curEnemy, curEnemyData, curEnemyData->abilityDataSecondary, curEnemyData->abilityDataPrimary);
            transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
        }
    } else if (curEnemy->state == ENEMY_STATE_ATTACK_1) {
        chargeEnemyAttack(curEnemy, curEnemyData, index);
    } else if (curEnemy->state == ENEMY_STATE_ATTACK_2) {
        if (curEnemy->stateTimer <= 0) {
            transitionToState(curEnemy, ENEMY_STATE_IDLE);
        }
    }
}

void chargeEnemyUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
    if (curEnemy->state == ENEMY_STATE_IDLE) {
        chargeEnemyIdle(curEnemy, curEnemyData, index);
    } else if (curEnemy->state == ENEMY_STATE_PREP) {
        if (curEnemy->stateTimer <= 0) {
            setupCharge(curEnemy, curEnemyData, curEnemyData->abilityDataSecondary, curEnemyData->abilityDataPrimary);
            transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
        }
    } else if (curEnemy->state == ENEMY_STATE_ATTACK_1) {
        chargeEnemyAttack(curEnemy, curEnemyData, index);
    } else if (curEnemy->state == ENEMY_STATE_ATTACK_2) {
        if (curEnemy->stateTimer <= 0) {
            transitionToState(curEnemy, ENEMY_STATE_IDLE);
        }
    }
}
