#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "game/map/map.h"
#include "game/enemies/enemyAI.h"
#include "game/player/player.h"
#include "game/entities/projectiles.h"
#include "game/rendering/particles.h"
#include "game/systems/spawnManager.h"
#include "game/systems/bossCinematic.h"
#include "game/systems/audioManager.h"
#include "game/UI/UI.h"

#define BOSS_DIFFICULTY_COUNT 2
#define BOSS_PHASE_COUNT      2

// Plays the enemy's looping idle sound the tick the idle animation advances onto one of its
// two configured idle frames. An idleSFXFrame of 0 is treated as unset and ignored.
static void playEnemyIdleSFX(Enemy *curEnemy, EnemyData *curEnemyData) {
	if (curEnemyData->idleSFX == SFX_NONE || !curEnemy->animationFrameAdvanced) {
		return;
	}
	// The idle loop wraps to frame 0 on the tick it completes, so animationFrame never holds
	// idleFrameEnd. Frame 0 is only ever reached via that wrap, so treat it as landing on
	// idleFrameEnd — this lets a SFX frame configured as the final idle frame still fire.
	int frame = curEnemy->animationFrame;
	if (curEnemy->state == ENEMY_STATE_IDLE && frame == 0) {
		frame = curEnemyData->idleFrameEnd;
	}
	if ((curEnemyData->idleSFXFrame1 != 0 && frame == curEnemyData->idleSFXFrame1) ||
		(curEnemyData->idleSFXFrame2 != 0 && frame == curEnemyData->idleSFXFrame2)) {
		playSoundSpatial(curEnemyData->idleSFX, curEnemy->x, curEnemy->y);
	}
}

// Plays the enemy's prep windup sound. Called when the enemy enters its prep state.
static void playEnemyPrepSFX(Enemy *curEnemy, EnemyData *curEnemyData) {
	if (curEnemyData->prepSFX != SFX_NONE) {
		playSoundSpatial(curEnemyData->prepSFX, curEnemy->x, curEnemy->y);
	}
}

// Plays the enemy's attack sound the tick the attack animation advances onto its active frame.
static void playEnemyAttackSFX(Enemy *curEnemy, EnemyData *curEnemyData) {
	if (curEnemyData->attackSFX != SFX_NONE && curEnemy->animationFrameAdvanced &&
		curEnemy->animationFrame == curEnemyData->attackActiveFrame) {
		playSoundSpatial(curEnemyData->attackSFX, curEnemy->x, curEnemy->y);
	}
}

void dummyUpdate(Enemy *enemy, EnemyData *data, int index, bool attackAnimationReset) {
	(void)enemy;
	(void)data;
	(void)index;
	(void)attackAnimationReset;
}

// === Elder Boss properties ===
typedef struct ElderBossProperties {
	float attackCooldown;
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
	int   summonBudget;
	int   summonMinSpecials;
	float summonRadius;
	float summonTime;
} ElderBossProperties;

static const ElderBossProperties ELDER_BOSS_PROPERTIES[BOSS_DIFFICULTY_COUNT][BOSS_PHASE_COUNT] = {
	// Difficulty 0
	{
		// Phase 0
		{
			.attackCooldown     = 1.5f,
			.idleTime           = 5.0f,
			.prepTime           = 5.0f,
			.attack2Count       = 4, // Root attack count
			.attack2Spacing     = CELL_SIZE, // Root attack spacing
			.dashPrepTime       = 1.5f,
			.dashDistance       = CELL_SIZE * 2.0f,
			.dashCount          = 3,
			.dashSpeed          = 2.0f,
			.projectileWaves    = 5,
			.projectilesPerWave = 8,
			.projectileCooldown = 0.2f,
			.projectileDamage   = 10,
			.projectileSpeed    = 25.0f,
			.waveAngleOffset    = 5.0f * PI / 180.0f,
			.summonBudget       = 5,
			.summonMinSpecials  = 0,
			.summonRadius       = CELL_SIZE * 2.0f,
			.summonTime         = 2.0f,
		},
		// Phase 1
		{
			.attackCooldown     = 1.5f,
			.idleTime           = 3.0f,
			.prepTime           = 3.0f,
			.attack2Count       = 7, // Root attack count
			.attack2Spacing     = CELL_SIZE, // Root attack spacing
			.dashPrepTime       = 1.0f,
			.dashDistance       = CELL_SIZE * 3.0f,
			.dashCount          = 5,
			.dashSpeed          = 2.5f,
			.projectileWaves    = 8,
			.projectilesPerWave = 8,
			.projectileCooldown = 0.2f,
			.projectileDamage   = 10,
			.projectileSpeed    = 35.0f,
			.waveAngleOffset    = 5.0f * PI / 180.0f,
			.summonBudget       = 10,
			.summonMinSpecials  = 2,
			.summonRadius       = CELL_SIZE * 2.0f,
			.summonTime         = 2.0f,
		},
	},
	// Difficulty 1 — placeholder copies of difficulty 0
	{
		{
			.attackCooldown     = 1.5f,
			.idleTime           = 5.0f,
			.prepTime           = 5.0f,
			.attack2Count       = 5, // Root attack count
			.attack2Spacing     = CELL_SIZE, // Root attack spacing
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
			.summonBudget       = 8,
			.summonMinSpecials  = 1,
			.summonRadius       = CELL_SIZE * 2.0f,
			.summonTime         = 2.0f,
		},
		{
			.attackCooldown     = 1.5f,
			.idleTime           = 5.0f,
			.prepTime           = 5.0f,
			.attack2Count       = 5, // Root attack count
			.attack2Spacing     = CELL_SIZE, // Root attack spacing
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
			.summonBudget       = 8,
			.summonMinSpecials  = 1,
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
	float attackCooldown;     // Seconds between contact attacks
	float idleTime;
	float prepTime;
	float dashTime;
	int   teleportCount;      // Teleport-and-shoot repetitions in the PROJECTILE phase
	int   teleportShotCount;  // Aimed projectiles fired after each teleport
	float teleportIntentLead; // Attack intent warning shown on reveal before the shots start
	float projectileCooldown; // Seconds between aimed projectiles
	float summonTime;
	int   spawnCount;         // Count of wraiths spawned in a circle by the SUMMON phase
	int   spawnBudget;        // Budget passed to rollEnemiesFromBudget for the SUMMON phase
	int   spawnMinSpecials;   // Minimum specials passed to rollEnemiesFromBudget for the SUMMON phase
	float spawnRadius;        // Scatter radius for the budget spawns in the SUMMON phase
	int   rushCount;
	int   volleyCount;            // Projectiles fired by the VOLLEY phase
	float volleyShotSpacing;      // Seconds between volley shots
	float volleyConeAngle;        // Full cone angle the shots scatter within, radians
	float volleyProjectileSpeed;
} WraithBossProperties;

static const WraithBossProperties WRAITH_BOSS_PROPERTIES[BOSS_DIFFICULTY_COUNT][BOSS_PHASE_COUNT] = {
	// Difficulty 0
	{
		// Phase 0
		{
			.attackCooldown     = 1.5f,
			.idleTime           = 2.0f,
			.prepTime           = 1.0f,
			.dashTime           = 2.5f,
			.teleportCount      = 3,
			.teleportShotCount  = 3,
			.teleportIntentLead = 1.5f,
			.projectileCooldown = 0.5f,
			.summonTime         = 5.0f,
			.spawnCount         = 2,
			.spawnBudget        = 4,
			.spawnMinSpecials   = 0,
			.spawnRadius        = CELL_SIZE * 2.0f,
			.rushCount          = 3,
			.volleyCount           = 10,
			.volleyShotSpacing     = 0.1f,
			.volleyConeAngle       = 15.0f * PI / 180.0f,
			.volleyProjectileSpeed = 60.0f,
		},
		// Phase 1
		{
			.attackCooldown     = 1.5f,
			.idleTime           = 1.0f,
			.prepTime           = 0.5f,
			.dashTime           = 2.5f,
			.teleportCount      = 3,
			.teleportShotCount  = 5,
			.teleportIntentLead = 1.5f,
			.projectileCooldown = 0.3f,
			.summonTime         = 5.0f,
			.spawnCount         = 4,
			.spawnBudget        = 8,
			.spawnMinSpecials   = 2,
			.spawnRadius        = CELL_SIZE * 2.0f,
			.rushCount          = 4,
			.volleyCount           = 15,
			.volleyShotSpacing     = 0.1f,
			.volleyConeAngle       = 20.0f * PI / 180.0f,
			.volleyProjectileSpeed = 60.0f,
		},
	},
	// Difficulty 1 — placeholder copies of difficulty 0
	{
		{
			.attackCooldown     = 1.5f,
			.idleTime           = 2.0f,
			.prepTime           = 1.0f,
			.dashTime           = 2.5f,
			.teleportCount      = 2,
			.teleportShotCount  = 3,
			.teleportIntentLead = 1.5f,
			.projectileCooldown = 0.5f,
			.summonTime         = 5.0f,
			.spawnCount         = 2,
			.spawnBudget        = 8,
			.spawnMinSpecials   = 1,
			.spawnRadius        = CELL_SIZE * 2.0f,
			.rushCount          = 3,
			.volleyCount           = 10,
			.volleyShotSpacing     = 0.1f,
			.volleyConeAngle       = 15.0f * PI / 180.0f,
			.volleyProjectileSpeed = 60.0f,
		},
		{
			.attackCooldown     = 1.5f,
			.idleTime           = 1.0f,
			.prepTime           = 0.5f,
			.dashTime           = 2.5f,
			.teleportCount      = 3,
			.teleportShotCount  = 5,
			.teleportIntentLead = 1.5f,
			.projectileCooldown = 0.3f,
			.summonTime         = 5.0f,
			.spawnCount         = 4,
			.spawnBudget        = 8,
			.spawnMinSpecials   = 1,
			.spawnRadius        = CELL_SIZE * 2.0f,
			.rushCount          = 4,
			.volleyCount           = 15,
			.volleyShotSpacing     = 0.1f,
			.volleyConeAngle       = 20.0f * PI / 180.0f,
			.volleyProjectileSpeed = 60.0f,
		},
	},
};

static const WraithBossProperties *wraithProperties;

#define WRAITH_STATE_DASH        ENEMY_STATE_ATTACK_1
#define WRAITH_STATE_PROJECTILE  ENEMY_STATE_ATTACK_2
#define WRAITH_STATE_SUMMON      ENEMY_STATE_ATTACK_3
#define WRAITH_STATE_VOLLEY      ENEMY_STATE_POST_ATTACK  // Holds the attack pose while the volley fires

#define WRAITH_VOLLEY_WARNING_LEAD 0.4f

#define WRAITH_TELEPORT_HIDE_DELAY   0.1f  // Time the disguise particle covers the boss before it vanishes
#define WRAITH_TELEPORT_REVEAL_PRE_DELAY 0.55f  // Time the reveal particle plays before the boss reappears
#define WRAITH_TELEPORT_REVEAL_DELAY 1.5f  // Time spent invisible at the new spot before reappearing
#define WRAITH_TELEPORT_MIN_PLAYER_DISTANCE (CELL_SIZE * 2.0f)
#define WRAITH_TELEPORT_MAX_PLAYER_DISTANCE (CELL_SIZE * 6.0f)

// Teleport-attack sub-phases for WRAITH_STATE_PROJECTILE, stored in Enemy.attackCycle.
typedef enum WraithTeleportPhase {
	WRAITH_TELEPORT_PHASE_DISGUISE, // A spawn particle covers the boss before it goes invisible
	WRAITH_TELEPORT_PHASE_HIDDEN,   // Invisible at the new spot, waiting to drop the reveal particle
	WRAITH_TELEPORT_PHASE_REVEAL,   // Reveal particle playing, waiting to reappear under it
	WRAITH_TELEPORT_PHASE_FIRE      // Revealed, firing the aimed shots
} WraithTeleportPhase;

// === Dragon Boss properties ===
typedef struct DragonBossProperties {
	float attackCooldown;         // Seconds the dragon rests in idle between attacks
	// Attack 1 — slow projectile rings
	int   ringCount;              // Number of rings shot per attack
	float ringSpacing;           // Seconds between rings (animation holds last frame)
	float ringProjectileSpeed;
	int   ringProjectileDamage;
	// Attack 2 — four-armed sweeping sprinkler
	int   sprinklerShots;            // Number of 4-way bursts fired
	float sprinklerShotSpacing;      // Seconds between bursts
	float sprinklerSweepStep;        // Radians the sprinkler rotates per burst
	float sprinklerProjectileSpeed;
	int   sprinklerProjectileDamage;
	// Attack 3 — rapid aimed volleys
	int   volleyLineCount;       // Radial lines fired per shot (one aimed at the player)
	int   volleyShots;           // Projectiles per line per volley
	int   volleyCount;           // Number of volleys per attack
	float volleyShotSpacing;     // Seconds between shots within a volley
	float volleySpacing;         // Seconds between volleys (animation holds last frame)
	float volleyProjectileSpeed;
	int   volleyProjectileDamage;
	float volleyWarningLead;     // Seconds the warning lines show before the first shot of each volley
	// Attack 4 — summon adds
	int   summonCount;           // Enemies summoned per attack (also caps active non-boss enemies)
	int   summonMinSpecials;     // Minimum specials among the summons (0 = all basic fodder)
	float summonRadius;          // Spawn scatter radius around the dragon
	float summonTime;            // Seconds spent holding the summon pose before resuming
} DragonBossProperties;

static const DragonBossProperties DRAGON_BOSS_PROPERTIES[BOSS_DIFFICULTY_COUNT][BOSS_PHASE_COUNT] = {
	// Difficulty 0
	{
		// Phase 0
		{
			.attackCooldown        = 6.0f,
			.ringCount             = 3,
			.ringSpacing           = 0.5f,
			.ringProjectileSpeed   = 25.0f,
			.ringProjectileDamage  = 12,
			.sprinklerShots            = 10,
			.sprinklerShotSpacing      = 0.06f,
			.sprinklerSweepStep        = 6.0f * PI / 180.0f,
			.sprinklerProjectileSpeed  = 40.0f,
			.sprinklerProjectileDamage = 12,
			.volleyLineCount       = 16,
			.volleyShots           = 7,
			.volleyCount           = 3,
			.volleyShotSpacing     = 0.2f,
			.volleySpacing         = 1.5f,
			.volleyProjectileSpeed = 100.0f,
			.volleyProjectileDamage = 12,
			.volleyWarningLead     = 0.8f,
			.summonCount           = 4,
			.summonMinSpecials     = 0,
			.summonRadius          = CELL_SIZE * 2.0f,
			.summonTime            = 1.5f,
		},
		// Phase 1
		{
			.attackCooldown        = 3.0f,
			.ringCount             = 4,
			.ringSpacing           = 0.5f,
			.ringProjectileSpeed   = 25.0f,
			.ringProjectileDamage  = 14,
			.sprinklerShots            = 15,
			.sprinklerShotSpacing      = 0.06f,
			.sprinklerSweepStep        = 6.0f * PI / 180.0f,
			.sprinklerProjectileSpeed  = 40.0f,
			.sprinklerProjectileDamage = 14,
			.volleyLineCount       = 16,
			.volleyShots           = 10,
			.volleyCount           = 5,
			.volleyShotSpacing     = 0.2f,
			.volleySpacing         = 1.5f,
			.volleyProjectileSpeed = 100.0f,
			.volleyProjectileDamage = 14,
			.volleyWarningLead     = 0.6f,
			.summonCount           = 4,
			.summonMinSpecials     = 4,
			.summonRadius          = CELL_SIZE * 2.0f,
			.summonTime            = 1.5f,
		},
	},
	// Difficulty 1
	{
		{
			.attackCooldown        = 1.5f,
			.ringCount             = 3,
			.ringSpacing           = 0.5f,
			.ringProjectileSpeed   = 25.0f,
			.ringProjectileDamage  = 12,
			.sprinklerShots            = 20,
			.sprinklerShotSpacing      = 0.06f,
			.sprinklerSweepStep        = 6.0f * PI / 180.0f,
			.sprinklerProjectileSpeed  = 40.0f,
			.sprinklerProjectileDamage = 12,
			.volleyLineCount       = 16,
			.volleyShots           = 7,
			.volleyCount           = 3,
			.volleyShotSpacing     = 0.2f,
			.volleySpacing         = 1.5f,
			.volleyProjectileSpeed = 100.0f,
			.volleyProjectileDamage = 12,
			.volleyWarningLead     = 0.6f,
			.summonCount           = 4,
			.summonMinSpecials     = 0,
			.summonRadius          = CELL_SIZE * 2.0f,
			.summonTime            = 1.5f,
		},
		{
			.attackCooldown        = 1.5f,
			.ringCount             = 4,
			.ringSpacing           = 0.5f,
			.ringProjectileSpeed   = 25.0f,
			.ringProjectileDamage  = 14,
			.sprinklerShots            = 30,
			.sprinklerShotSpacing      = 0.06f,
			.sprinklerSweepStep        = 6.0f * PI / 180.0f,
			.sprinklerProjectileSpeed  = 40.0f,
			.sprinklerProjectileDamage = 14,
			.volleyLineCount       = 16,
			.volleyShots           = 10,
			.volleyCount           = 5,
			.volleyShotSpacing     = 0.2f,
			.volleySpacing         = 1.5f,
			.volleyProjectileSpeed = 100.0f,
			.volleyProjectileDamage = 14,
			.volleyWarningLead     = 0.4f,
			.summonCount           = 4,
			.summonMinSpecials     = 4,
			.summonRadius          = CELL_SIZE * 2.0f,
			.summonTime            = 1.5f,
		},
	},
};

static const DragonBossProperties *dragonProperties;

// Round-robin attack cycle, stored in Enemy.attackCycle.
typedef enum DragonAttackCycle {
	DRAGON_ATTACK_CYCLE_RINGS,
	DRAGON_ATTACK_CYCLE_SPRINKLER,
	DRAGON_ATTACK_CYCLE_VOLLEY,
	DRAGON_ATTACK_CYCLE_SUMMON,
	DRAGON_ATTACK_CYCLE_COUNT
} DragonAttackCycle;

// Summon attack holds in the post-attack (held) pose; the dragon otherwise never uses POST_ATTACK.
#define DRAGON_STATE_SUMMON ENEMY_STATE_POST_ATTACK

// Sub-state within a multi-shot attack, stored in Enemy.nextState.
#define DRAGON_SUBSTATE_ATTACKING 0  // Playing the attack animation / firing
#define DRAGON_SUBSTATE_SPACING   1  // Holding the last attack frame between shots

// Sprinkler firing sub-state, stored in Enemy.nextState while in ENEMY_STATE_ATTACK_2. The dragon
// homes in on the room center during its idle wander, so the sweep opens fire from there immediately.
#define DRAGON_SPRINKLER_FIRING 1  // Sweeping the four-armed burst

#define DRAGON_RING_SLOTS 32  // Ring positions before the two gaps are removed

void proximityEnemyUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	playEnemyIdleSFX(curEnemy, curEnemyData);

	// Constantly attack nearby player
	if (playerInAttackDistance(index)) {
		if (curEnemy->attackCooldown <= 0) {
			curEnemy->attackCooldown = curEnemyData->attackCooldown;
			damagePlayerFromDirection(curEnemyData->attackDamage, curEnemy->x, curEnemy->y);
			if (curEnemyData->attackSFX != SFX_NONE) {
				playSoundSpatial(curEnemyData->attackSFX, curEnemy->x, curEnemy->y);
			}
		}
	} else {
		curEnemy->attackCooldown = curEnemyData->attackCooldown;
	}

	if (!playerInAttackDetectionRange(index)) {
		// Constantly move towards player if not in attack range
		moveEnemy(curEnemy, index, playerX, playerY, curEnemyData->moveSpeed);
	}
}

void selfDestructEnemyUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	// Charge the player; on contact deal damage once and immediately die
	if (playerInAttackDistance(index)) {
		damagePlayerFromDirection(curEnemyData->attackDamage, curEnemy->x, curEnemy->y);
		killEnemy(index);
	} else {
		playEnemyIdleSFX(curEnemy, curEnemyData);
		moveEnemy(curEnemy, index, playerX, playerY, curEnemyData->moveSpeed);
	}
}

void meleeEnemyUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	if (curEnemy->state == ENEMY_STATE_IDLE) {
		playEnemyIdleSFX(curEnemy, curEnemyData);

		// Begin prep phase if player is in range and cooldown is ready
		if (playerInAttackDetectionRange(index)) {
			if (curEnemy->attackCooldown <= 0) {
				curEnemy->attackCooldown = curEnemyData->attackCooldown;
				curEnemy->stateTimer = curEnemyData->prepTime;
				transitionToState(curEnemy, ENEMY_STATE_PREP);
				playEnemyPrepSFX(curEnemy, curEnemyData);
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
		playEnemyAttackSFX(curEnemy, curEnemyData);

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

// Melee that scurries directly away from the player during recovery (hit-and-run)
void hitAndRunEnemyUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	if (curEnemy->state == ENEMY_STATE_IDLE) {
		if (curEnemy->stateCount == 1) {
			if (curEnemy->stateTimer > 0) {
				float awayX = curEnemy->x - playerX;
				float awayY = curEnemy->y - playerY;
				float length = sqrtf(awayX * awayX + awayY * awayY);
				if (length > 0.001f) {
					float retreatTargetX = curEnemy->x + (awayX / length) * CELL_SIZE * 4;
					float retreatTargetY = curEnemy->y + (awayY / length) * CELL_SIZE * 4;
					moveEnemy(curEnemy, index, retreatTargetX, retreatTargetY, curEnemyData->moveSpeed);
				}
				return;
			}
			curEnemy->stateCount = 0;
		}

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

		moveEnemy(curEnemy, index, playerX, playerY, curEnemyData->moveSpeed);
	} else if (curEnemy->state == ENEMY_STATE_PREP) {
		if (curEnemy->stateTimer <= 0) {
			if (curEnemyData->abilityDataPrimary > 0) {
				curEnemy->targetX = playerX;
				curEnemy->targetY = playerY;
				curEnemy->abilityCooldown = curEnemyData->abilityDataSecondary;
			}
			transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
		}
	} else if (curEnemy->state == ENEMY_STATE_ATTACK_1) {
		if (curEnemy->abilityCooldown > 0) {
			moveEnemy(curEnemy, index, curEnemy->targetX, curEnemy->targetY, curEnemyData->abilityDataPrimary);
		}

		if (curEnemy->animationFrame >= curEnemyData->attackActiveFrame) {
			if (!curEnemy->attackedPlayer && playerInAttackDistance(index)) {
				curEnemy->attackedPlayer = true;
				damagePlayerFromDirection(curEnemyData->attackDamage, curEnemy->x, curEnemy->y);
			}
		}

		if (attackAnimationReset) {
			curEnemy->attackedPlayer = false;
			curEnemy->attackCooldown = curEnemyData->attackCooldown;
			curEnemy->stateTimer = curEnemyData->postAttackTime;
			curEnemy->stateCount = 1;
			transitionToState(curEnemy, ENEMY_STATE_IDLE);
		}
	}
}

// Two-hit melee: a jab (ATTACK_1) that chains into a second swing (ATTACK_2).
void comboMeleeEnemyUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	if (curEnemy->state == ENEMY_STATE_IDLE) {
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

		moveEnemy(curEnemy, index, playerX, playerY, curEnemyData->moveSpeed);
	} else if (curEnemy->state == ENEMY_STATE_PREP) {
		if (curEnemy->stateTimer <= 0) {
			if (curEnemyData->abilityDataPrimary > 0) {
				curEnemy->targetX = playerX;
				curEnemy->targetY = playerY;
				curEnemy->abilityCooldown = curEnemyData->abilityDataSecondary;
			}
			transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
		}
	} else if (curEnemy->state == ENEMY_STATE_ATTACK_1) {
		// Lunge into the first hit
		if (curEnemy->abilityCooldown > 0) {
			moveEnemy(curEnemy, index, curEnemy->targetX, curEnemy->targetY, curEnemyData->abilityDataPrimary);
		}

		if (curEnemy->animationFrame >= curEnemyData->attackActiveFrame) {
			if (!curEnemy->attackedPlayer && playerInAttackDistance(index)) {
				curEnemy->attackedPlayer = true;
				damagePlayerFromDirection(curEnemyData->attackDamage, curEnemy->x, curEnemy->y);
			}
		}

		// Chain straight into the second swing
		if (attackAnimationReset) {
			curEnemy->attackedPlayer = false;
			transitionToState(curEnemy, ENEMY_STATE_ATTACK_2);
		}
	} else if (curEnemy->state == ENEMY_STATE_ATTACK_2) {
		if (curEnemy->animationFrame >= curEnemyData->attackActiveFrame) {
			if (!curEnemy->attackedPlayer && playerInAttackDistance(index)) {
				curEnemy->attackedPlayer = true;
				damagePlayerFromDirection(curEnemyData->attackDamage, curEnemy->x, curEnemy->y);
			}
		}

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

// Stationary windup into a self-centered AoE slam. abilityDataPrimary is the slam radius.
void slamEnemyUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	if (curEnemy->state == ENEMY_STATE_IDLE) {
		if (curEnemy->attackCooldown <= 0 && playerInAttackDetectionRange(index)) {
			curEnemy->attackCooldown = curEnemyData->attackCooldown;
			curEnemy->stateTimer = curEnemyData->prepTime;
			transitionToState(curEnemy, ENEMY_STATE_PREP);
			addAttackIntentIndicator(curEnemy->x, curEnemy->y, curEnemyData->prepTime);
			spawnParticle(PARTICLE_TYPE_WARNING, curEnemy->x, curEnemy->y, curEnemy->x, curEnemy->y, PARTICLE_TYPE_NONE, true /* enemyParticle */);
		}

		moveEnemy(curEnemy, index, playerX, playerY, curEnemyData->moveSpeed);
	} else if (curEnemy->state == ENEMY_STATE_PREP) {
		if (curEnemy->stateTimer <= 0) {
			transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
		}
	} else if (curEnemy->state == ENEMY_STATE_ATTACK_1) {
		if (curEnemy->animationFrame >= curEnemyData->attackActiveFrame) {
			if (!curEnemy->attackedPlayer) {
				curEnemy->attackedPlayer = true;
				float slamRadius = curEnemyData->abilityDataPrimary;
				if (distanceSquared(playerX, playerY, curEnemy->x, curEnemy->y) <= slamRadius * slamRadius) {
					damagePlayerFromDirection(curEnemyData->attackDamage, curEnemy->x, curEnemy->y);
				}
			}
		}

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
		playEnemyIdleSFX(curEnemy, curEnemyData);
		// Start shooting projectile if player is in range
		if (curEnemy->attackCooldown <= 0 && playerInAttackDetectionRange(index)) {
			playEnemyPrepSFX(curEnemy, curEnemyData);
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
			fireEnemyProjectile(projectileX, projectileY, curEnemy->x, curEnemy->y, deltaX, deltaY, projectileSpeed, damage, curEnemyData->projectileData);
			if (curEnemyData->attackSFX != SFX_NONE) {
				playSoundSpatial(curEnemyData->attackSFX, curEnemy->x, curEnemy->y);
			}
		}

		// Transition back to idle state after attack animation ends
		if (attackAnimationReset) {
			curEnemy->attackedPlayer = false;
			transitionToState(curEnemy, ENEMY_STATE_IDLE);
		}
	}
}

// Walks straight toward the player like a proximity enemy, then once the player is within
// attack detection range it stops to fire a single projectile (like projectileEnemyUpdate).
void proximityProjectileEnemyUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	if (curEnemy->state == ENEMY_STATE_IDLE) {
		playEnemyIdleSFX(curEnemy, curEnemyData);
		// Start shooting projectile if player is in range
		if (curEnemy->attackCooldown <= 0 && playerInAttackDetectionRange(index)) {
			playEnemyPrepSFX(curEnemy, curEnemyData);
			curEnemy->attackCooldown = curEnemyData->attackCooldown;
			transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
			addAttackIntentIndicator(curEnemy->x, curEnemy->y, 0.0f);
		}

		// Walk straight towards the player while not in attack range
		if (!playerInAttackDetectionRange(index)) {
			moveEnemy(curEnemy, index, playerX, playerY, curEnemyData->moveSpeed);
		}
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
			fireEnemyProjectile(projectileX, projectileY, curEnemy->x, curEnemy->y, deltaX, deltaY, projectileSpeed, damage, curEnemyData->projectileData);
			if (curEnemyData->attackSFX != SFX_NONE) {
				playSoundSpatial(curEnemyData->attackSFX, curEnemy->x, curEnemy->y);
			}
		}

		// Transition back to idle state after attack animation ends
		if (attackAnimationReset) {
			curEnemy->attackedPlayer = false;
			transitionToState(curEnemy, ENEMY_STATE_IDLE);
		}
	}
}

// Kites to a standoff distance and lobs a single slow, telegraphed projectile.
// abilityDataPrimary is projectile speed, abilityDataSecondary is the preferred standoff distance.
void lobberEnemyUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	if (curEnemy->state == ENEMY_STATE_IDLE) {
		if (curEnemy->attackCooldown <= 0 && playerInAttackDetectionRange(index)) {
			curEnemy->attackCooldown = curEnemyData->attackCooldown;
			curEnemy->stateTimer = curEnemyData->prepTime;
			transitionToState(curEnemy, ENEMY_STATE_PREP);
			addAttackIntentIndicator(curEnemy->x, curEnemy->y, curEnemyData->prepTime);
		}

		// Hold a preferred standoff distance: retreat when too close, approach when too far
		float standoff = curEnemyData->abilityDataSecondary;
		float distToPlayer = sqrtf(distanceSquared(playerX, playerY, curEnemy->x, curEnemy->y));
		if (distToPlayer < standoff * 0.75f) {
			float awayX = curEnemy->x - playerX;
			float awayY = curEnemy->y - playerY;
			float length = sqrtf(awayX * awayX + awayY * awayY);
			if (length > 0.001f) {
				float retreatTargetX = curEnemy->x + (awayX / length) * CELL_SIZE * 4;
				float retreatTargetY = curEnemy->y + (awayY / length) * CELL_SIZE * 4;
				moveEnemy(curEnemy, index, retreatTargetX, retreatTargetY, curEnemyData->moveSpeed);
			}
		} else if (distToPlayer > standoff) {
			moveEnemy(curEnemy, index, playerX, playerY, curEnemyData->moveSpeed);
		}
	} else if (curEnemy->state == ENEMY_STATE_PREP) {
		if (curEnemy->stateTimer <= 0) {
			transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
		}
	} else if (curEnemy->state == ENEMY_STATE_ATTACK_1) {
		if (!curEnemy->attackedPlayer && curEnemy->animationFrame >= curEnemyData->attackActiveFrame) {
			curEnemy->attackedPlayer = true;
			float distanceToPlayer = sqrtf(distanceSquared(playerX, playerY, curEnemy->x, curEnemy->y));
			float deltaX = (playerX - curEnemy->x) / distanceToPlayer;
			float deltaY = (playerY - curEnemy->y) / distanceToPlayer;
			float projectileSpeed = curEnemyData->abilityDataPrimary;
			float damage = curEnemyData->attackDamage;
			float projectileX = curEnemy->x + deltaX * PROJECTILE_OFFSET;
			float projectileY = curEnemy->y + deltaY * PROJECTILE_OFFSET;
			fireEnemyProjectile(projectileX, projectileY, curEnemy->x, curEnemy->y, deltaX, deltaY, projectileSpeed, damage, curEnemyData->projectileData);
		}

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
		playEnemyIdleSFX(curEnemy, curEnemyData);

		if (curEnemy->attackCooldown <= 0 && playerInAttackDetectionRange(index)) {
			curEnemy->attackCooldown = curEnemyData->attackCooldown;
			transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
		}

		moveEnemy(curEnemy, index, playerX, playerY, curEnemyData->moveSpeed);
	} else if (curEnemy->state == ENEMY_STATE_ATTACK_1) {
		playEnemyAttackSFX(curEnemy, curEnemyData);

		if (curEnemy->animationFrame >= curEnemyData->attackActiveFrame) {
			if (!curEnemy->attackedPlayer) {
				curEnemy->attackedPlayer = true;
				spawnEnemyWithSpawnParticle(ENEMY_TYPE_WRAITH, curEnemy->x, curEnemy->y);
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

// === Random room target ===
#define ROOM_TARGET_MAX_ATTEMPTS 20

// Picks a random floor cell in the enemy's room that is at least minDistance away from
// the enemy's current position, and stores it as the next target.
static void pickRandomRoomTarget(Enemy *curEnemy, float minDistance) {
	int roomIndex = getCurrentRoomIndex();
	Point interiorMin = getRoomInteriorMin(roomIndex);
	Point interiorMax = getRoomInteriorMax(roomIndex);
	float minDistanceSq = minDistance * minDistance;
	for (int attempt = 0; attempt < ROOM_TARGET_MAX_ATTEMPTS; attempt++) {
		int cellX = randInt(interiorMin.x, interiorMax.x);
		int cellY = randInt(interiorMin.y, interiorMax.y);
		if (isBlocked(cellX, cellY)) {
			continue;
		}
		float candidateX = cellX * CELL_SIZE + CELL_SIZE / 2.0f;
		float candidateY = cellY * CELL_SIZE + CELL_SIZE / 2.0f;
		float dx = candidateX - curEnemy->x;
		float dy = candidateY - curEnemy->y;
		if (dx * dx + dy * dy >= minDistanceSq) {
			curEnemy->targetX = candidateX;
			curEnemy->targetY = candidateY;
			return;
		}
	}
}

// Picks a random floor cell in the enemy's room within [minDistance, maxDistance] of the player,
// and stores it as the next target. Leaves the target unchanged if no candidate is found.
static void pickRandomRoomTargetNearPlayer(Enemy *curEnemy, float minDistance, float maxDistance) {
	int roomIndex = getCurrentRoomIndex();
	Point interiorMin = getRoomInteriorMin(roomIndex);
	Point interiorMax = getRoomInteriorMax(roomIndex);
	float minDistanceSq = minDistance * minDistance;
	float maxDistanceSq = maxDistance * maxDistance;
	for (int attempt = 0; attempt < ROOM_TARGET_MAX_ATTEMPTS; attempt++) {
		int cellX = randInt(interiorMin.x, interiorMax.x);
		int cellY = randInt(interiorMin.y, interiorMax.y);
		if (isBlocked(cellX, cellY)) {
			continue;
		}
		float candidateX = cellX * CELL_SIZE + CELL_SIZE / 2.0f;
		float candidateY = cellY * CELL_SIZE + CELL_SIZE / 2.0f;
		float distSq = distanceSquared(candidateX, candidateY, playerX, playerY);
		if (distSq >= minDistanceSq && distSq <= maxDistanceSq) {
			curEnemy->targetX = candidateX;
			curEnemy->targetY = candidateY;
			return;
		}
	}
}

// === Bomber constants ===
#define BOMBER_MIN_WANDER_COUNT 1
#define BOMBER_MAX_WANDER_COUNT 1
#define BOMBER_THROW_SPEED 45.0f

// Begins a new cycle: roll a wander count and queue up the first random position.
static void bomberStartCycle(Enemy *curEnemy, EnemyData *curEnemyData) {
	curEnemy->stateCount = randInt(BOMBER_MIN_WANDER_COUNT, BOMBER_MAX_WANDER_COUNT);
	pickRandomRoomTarget(curEnemy, curEnemyData->abilityDataPrimary);
}

void bomberUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	if (curEnemy->state == ENEMY_STATE_IDLE) {
		playEnemyIdleSFX(curEnemy, curEnemyData);

		// On spawn the bomber has no positions queued yet — start a fresh cycle
		if (curEnemy->stateCount == 0) {
			bomberStartCycle(curEnemy, curEnemyData);
		}

		float arrivalRadius = curEnemyData->abilityDataSecondary;
		float dx = curEnemy->targetX - curEnemy->x;
		float dy = curEnemy->targetY - curEnemy->y;
		bool arrived = dx * dx + dy * dy <= arrivalRadius * arrivalRadius;

		if (!arrived) {
			moveEnemy(curEnemy, index, curEnemy->targetX, curEnemy->targetY, curEnemyData->moveSpeed);
			return;
		}

		curEnemy->stateCount--;
		if (curEnemy->stateCount > 0) {
			// More random positions to visit before throwing
			pickRandomRoomTarget(curEnemy, curEnemyData->abilityDataPrimary);
		} else {
			// Done wandering — throw a bomb at the player from where we stand
			transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
			addAttackIntentIndicator(curEnemy->x, curEnemy->y, 0.0f);
		}
	} else if (curEnemy->state == ENEMY_STATE_ATTACK_1) {
		playEnemyAttackSFX(curEnemy, curEnemyData);

		// Lob a bomb projectile toward the player once the animation reaches the active frame.
		// It plants the bomb wherever it lands — on a wall, on the player, or at the target on timeout.
		if (!curEnemy->attackedPlayer && curEnemy->animationFrame >= curEnemyData->attackActiveFrame) {
			curEnemy->attackedPlayer = true;
			float toPlayerX = playerX - curEnemy->x;
			float toPlayerY = playerY - curEnemy->y;
			float distance = sqrtf(toPlayerX * toPlayerX + toPlayerY * toPlayerY);
			float deltaX = distance > 0.001f ? toPlayerX / distance : 1.0f;
			float deltaY = distance > 0.001f ? toPlayerY / distance : 0.0f;
			float projectileX = curEnemy->x + deltaX * PROJECTILE_OFFSET;
			float projectileY = curEnemy->y + deltaY * PROJECTILE_OFFSET;
			SpellProjectileData throwData = curEnemyData->projectileData;
			// Detonate at the player's position if nothing is hit along the way
			// throwData.activeTime = distance / BOMBER_THROW_SPEED;
			fireEnemyProjectile(projectileX, projectileY, curEnemy->x, curEnemy->y, deltaX, deltaY, BOMBER_THROW_SPEED, curEnemyData->attackDamage, throwData);
		}

		// Once the attack animation ends, start the next wander cycle
		if (attackAnimationReset) {
			curEnemy->attackedPlayer = false;
			bomberStartCycle(curEnemy, curEnemyData);
			transitionToState(curEnemy, ENEMY_STATE_IDLE);
		}
	}
}

// === Air Elemental constants ===
static const float AIR_ELEMENTAL_CARDINAL_X[4] = { 1.0f, -1.0f, 0.0f, 0.0f };
static const float AIR_ELEMENTAL_CARDINAL_Y[4] = { 0.0f, 0.0f, 1.0f, -1.0f };
#define AIR_ELEMENTAL_WARNING_CELLS 6

// Spawns a line of warning particles along each cardinal direction, spaced one cell apart, so the
// player can see where the projectiles will travel before they fire.
static void airElementalSpawnWarnings(Enemy *curEnemy) {
	for (int i = 0; i < 4; i++) {
		float deltaX = AIR_ELEMENTAL_CARDINAL_X[i];
		float deltaY = AIR_ELEMENTAL_CARDINAL_Y[i];
		for (int cell = 1; cell <= AIR_ELEMENTAL_WARNING_CELLS; cell++) {
			float distance = cell * CELL_SIZE;
			float spawnX = curEnemy->x + deltaX * distance;
			float spawnY = curEnemy->y + deltaY * distance;
			if (isBlocked((int)(spawnX / CELL_SIZE), (int)(spawnY / CELL_SIZE))) {
				break;
			}
			spawnParticle(PARTICLE_TYPE_WARNING, spawnX, spawnY, curEnemy->x, curEnemy->y, PARTICLE_TYPE_NONE, true /* enemyParticle */);
		}
	}
}

// Floats to random positions around the room; on arrival it fires a projectile in each of the
// four cardinal directions. abilityDataPrimary is the minimum distance between random positions,
// abilityDataSecondary is the arrival radius, and projectileData.speed is the projectile speed.
void airElementalUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	if (curEnemy->state == ENEMY_STATE_IDLE) {
		playEnemyIdleSFX(curEnemy, curEnemyData);

		// On spawn no position is queued yet — pick the first random position
		if (curEnemy->stateCount == 0) {
			pickRandomRoomTarget(curEnemy, curEnemyData->abilityDataPrimary);
			curEnemy->stateCount = 1;
		}

		float arrivalRadius = curEnemyData->abilityDataSecondary;
		float dx = curEnemy->targetX - curEnemy->x;
		float dy = curEnemy->targetY - curEnemy->y;
		if (dx * dx + dy * dy > arrivalRadius * arrivalRadius) {
			moveEnemy(curEnemy, index, curEnemy->targetX, curEnemy->targetY, curEnemyData->moveSpeed);
			return;
		}

		transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
		addAttackIntentIndicator(curEnemy->x, curEnemy->y, 0.0f);
		airElementalSpawnWarnings(curEnemy);
	} else if (curEnemy->state == ENEMY_STATE_ATTACK_1) {
		playEnemyAttackSFX(curEnemy, curEnemyData);

		// Fire a projectile in all four cardinal directions once the attack animation reaches the active frame
		if (!curEnemy->attackedPlayer && curEnemy->animationFrame >= curEnemyData->attackActiveFrame) {
			curEnemy->attackedPlayer = true;
			float projectileSpeed = curEnemyData->projectileData.speed;
			float damage = curEnemyData->attackDamage;
			for (int i = 0; i < 4; i++) {
				float deltaX = AIR_ELEMENTAL_CARDINAL_X[i];
				float deltaY = AIR_ELEMENTAL_CARDINAL_Y[i];
				float projectileX = curEnemy->x + deltaX * PROJECTILE_OFFSET;
				float projectileY = curEnemy->y + deltaY * PROJECTILE_OFFSET;
				fireEnemyProjectile(projectileX, projectileY, curEnemy->x, curEnemy->y, deltaX, deltaY, projectileSpeed, damage, curEnemyData->projectileData);
			}
		}

		// Once the attack animation ends, queue the next random position
		if (attackAnimationReset) {
			curEnemy->attackedPlayer = false;
			pickRandomRoomTarget(curEnemy, curEnemyData->abilityDataPrimary);
			transitionToState(curEnemy, ENEMY_STATE_IDLE);
		}
	}
}

// === Rock Golem ===
static void rockGolemSummonRubble(Enemy *curEnemy, EnemyData *curEnemyData) {
	int rubbleCount = (int)curEnemyData->abilityDataPrimary;
	float spacing = curEnemyData->abilityDataSecondary;

	float dx = playerX - curEnemy->x;
	float dy = playerY - curEnemy->y;
	float length = sqrtf(dx * dx + dy * dy);
	if (length < 0.001f) {
		dx = 1.0f;
		dy = 0.0f;
	} else {
		dx /= length;
		dy /= length;
	}

	playEnemyAttackSFX(curEnemy, curEnemyData);
	for (int i = 1; i <= rubbleCount; i++) {
		float distance = i * spacing;
		float spawnX = curEnemy->x + dx * distance;
		float spawnY = curEnemy->y + dy * distance;
		if (isBlocked((int)(spawnX / CELL_SIZE), (int)(spawnY / CELL_SIZE))) {
			break;
		}
		spawnParticle(PARTICLE_TYPE_WARNING, spawnX, spawnY, curEnemy->x, curEnemy->y, PARTICLE_TYPE_RUBBLE, true /* enemyParticle */);
	}
}

// Walks toward the player; once in attack detection range it slams, dealing damage to the player
// within attackDistance and summoning a line of rubble towards the player on the active attack frame.
void rockGolemUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	if (curEnemy->state == ENEMY_STATE_IDLE) {
		playEnemyIdleSFX(curEnemy, curEnemyData);

		bool forceAttack = curEnemy->stateTimer <= 0;
		if (forceAttack || (curEnemy->attackCooldown <= 0 && playerInAttackDetectionRange(index))) {
			playEnemyPrepSFX(curEnemy, curEnemyData);
			curEnemy->attackCooldown = curEnemyData->attackCooldown;
			curEnemy->stateTimer = curEnemyData->abilityDataTertiary + randFloat(-1.0f, 1.0f);
			transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
			addAttackIntentIndicator(curEnemy->x, curEnemy->y, 0.0f);
		}

		moveEnemy(curEnemy, index, playerX, playerY, curEnemyData->moveSpeed);
	} else if (curEnemy->state == ENEMY_STATE_ATTACK_1) {
		if (!curEnemy->attackedPlayer && curEnemy->animationFrame >= curEnemyData->attackActiveFrame) {
			curEnemy->attackedPlayer = true;

			// Slam: damage the player if within the radius around the golem
			if (playerInAttackDistance(index)) {
				damagePlayerFromDirection(curEnemyData->attackDamage, curEnemy->x, curEnemy->y);
			}

			rockGolemSummonRubble(curEnemy, curEnemyData);
		}

		if (attackAnimationReset) {
			curEnemy->attackedPlayer = false;
			transitionToState(curEnemy, ENEMY_STATE_IDLE);
		}
	}
}

// === Banshee constants ===
#define BANSHEE_VOLLEY_SHOT_COUNT 5
#define BANSHEE_VOLLEY_SHOT_SPACING 0.1f                 // Seconds between shots in the spray
#define BANSHEE_VOLLEY_CONE_ANGLE (15.0f * PI / 180.0f)  // Full cone the shots scatter within, radians
#define BANSHEE_VOLLEY_PROJECTILE_SPEED 40.0f

static void bossSpawnWarningLine(Enemy *curEnemy, float angle);

// Fires one projectile aimed at the locked target (curEnemy->targetX/Y), scattered within the cone.
static void bansheeFireVolleyShot(Enemy *curEnemy, EnemyData *curEnemyData) {
	float angle = atan2f(curEnemy->targetY - curEnemy->y, curEnemy->targetX - curEnemy->x) + randFloat(-0.5f, 0.5f) * BANSHEE_VOLLEY_CONE_ANGLE;
	float deltaX = cosf(angle);
	float deltaY = sinf(angle);
	float projectileX = curEnemy->x + deltaX * PROJECTILE_OFFSET;
	float projectileY = curEnemy->y + deltaY * PROJECTILE_OFFSET;
	fireEnemyProjectile(projectileX, projectileY, curEnemy->x, curEnemy->y, deltaX, deltaY, BANSHEE_VOLLEY_PROJECTILE_SPEED, curEnemyData->attackDamage, curEnemyData->projectileData);
}

// Floats to random room positions; on arrival it locks onto the player and sprays BANSHEE_VOLLEY_SHOT_COUNT
// projectiles within a cone, one every BANSHEE_VOLLEY_SHOT_SPACING seconds, then wanders to a new position and
// repeats. abilityDataPrimary is the minimum distance between random positions, abilityDataSecondary is the
// arrival radius. stateCount doubles as the queued-target flag while idle and the shot counter while attacking.
void bansheeUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	if (curEnemy->state == ENEMY_STATE_IDLE) {
		playEnemyIdleSFX(curEnemy, curEnemyData);

		// No position queued yet (fresh spawn or just finished a spray) — pick the next random position
		if (curEnemy->stateCount == 0) {
			pickRandomRoomTarget(curEnemy, curEnemyData->abilityDataPrimary);
			curEnemy->stateCount = 1;
		}

		float arrivalRadius = curEnemyData->abilityDataSecondary;
		float dx = curEnemy->targetX - curEnemy->x;
		float dy = curEnemy->targetY - curEnemy->y;
		if (dx * dx + dy * dy > arrivalRadius * arrivalRadius) {
			moveEnemy(curEnemy, index, curEnemy->targetX, curEnemy->targetY, curEnemyData->moveSpeed);
			return;
		}

		// Arrived: lock aim onto the player, telegraph the cone, and start the spray
		curEnemy->stateCount = 0;
		curEnemy->abilityCooldown = 0;
		curEnemy->targetX = playerX;
		curEnemy->targetY = playerY;
		float aimAngle = atan2f(playerY - curEnemy->y, playerX - curEnemy->x);
		float halfCone = BANSHEE_VOLLEY_CONE_ANGLE * 0.5f;
		bossSpawnWarningLine(curEnemy, aimAngle);
		bossSpawnWarningLine(curEnemy, aimAngle - halfCone);
		bossSpawnWarningLine(curEnemy, aimAngle + halfCone);
		transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
		addAttackIntentIndicator(curEnemy->x, curEnemy->y, 0.0f);
		playEnemyPrepSFX(curEnemy, curEnemyData);
	} else if (curEnemy->state == ENEMY_STATE_ATTACK_1) {
		playEnemyAttackSFX(curEnemy, curEnemyData);

		// Once the animation reaches the active frame, fire one shot per spacing interval
		if (curEnemy->animationFrame >= curEnemyData->attackActiveFrame && curEnemy->stateCount < BANSHEE_VOLLEY_SHOT_COUNT && curEnemy->abilityCooldown <= 0) {
			bansheeFireVolleyShot(curEnemy, curEnemyData);
			curEnemy->stateCount++;
			curEnemy->abilityCooldown = BANSHEE_VOLLEY_SHOT_SPACING;
		}

		// After every shot has fired, wander to a new position on the next animation loop
		if (curEnemy->stateCount >= BANSHEE_VOLLEY_SHOT_COUNT && attackAnimationReset) {
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
		playEnemyIdleSFX(curEnemy, curEnemyData);

		if (curEnemy->attackCooldown <= 0 && playerInAttackDetectionRange(index)) {
			curEnemy->attackCooldown = curEnemyData->attackCooldown;
			playEnemyPrepSFX(curEnemy, curEnemyData);
			transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
			addAttackIntentIndicator(curEnemy->x, curEnemy->y, 0.0f);
		}

		moveEnemy(curEnemy, index, playerX, playerY, curEnemyData->moveSpeed);
	} else if (curEnemy->state == ENEMY_STATE_ATTACK_1) {
		// Spawn tail attack particles at intervals during attack animation
		if (curEnemy->animationFrame >= curEnemyData->attackActiveFrame) {
			if (!curEnemy->attackedPlayer) {
				playEnemyAttackSFX(curEnemy, curEnemyData);
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
				float tailX = curEnemy->x + offsetX;
				float tailY = curEnemy->y + offsetY;
				if (curEnemyData->attackSFX != SFX_NONE) {
					playSoundSpatial(SFX_SERPENT_TAIL, tailX, tailY);
				}
				spawnParticle(PARTICLE_TYPE_SERPENT_TAIL, tailX, tailY, curEnemy->x, curEnemy->y, PARTICLE_TYPE_NONE, true /* enemyParticle */);

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
			fireEnemyProjectile(projectileX, projectileY, curEnemy->x, curEnemy->y, deltaX, deltaY, projectileSpeed, damage, curEnemyData->projectileData);

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

#define BOSS_MAX_SUMMONS 20

// Spawns the rolled summons scattered around the boss, but caps the spawn so the active non-boss enemy
// total never climbs past `count` (this summon's count). The player can thin adds out between summons
// instead of drowning under an ever-growing pile.
static void bossSpawnSummonsCapped(Enemy *curEnemy, const EnemyType *summons, int count, float summonRadius) {
	int toSpawn = count - getAliveMinionCount();
	for (int i = 0; i < toSpawn; i++) {
		float angle = randFloat(0.0f, 2.0f * PI);
		float radius = randFloat(0.0f, summonRadius);
		float sx = curEnemy->x + cosf(angle) * radius;
		float sy = curEnemy->y + sinf(angle) * radius;
		spawnEnemyWithSpawnParticle(summons[i], sx, sy);
	}
}

static void bossSpawnSummonsFromBudget(Enemy *curEnemy, int summonBudget, int summonMinSpecials, float summonRadius) {
	EnemyType summons[BOSS_MAX_SUMMONS];
	int count = rollEnemiesFromBudget(summons, BOSS_MAX_SUMMONS, summonBudget, summonMinSpecials);
	bossSpawnSummonsCapped(curEnemy, summons, count, summonRadius);
}

#define BOSS_WARNING_LINE_SPACING CELL_SIZE

// Lays a line of short warning particles from the boss out along the given angle until it hits a wall,
// telegraphing the path projectiles will take before any are fired.
static void bossSpawnWarningLine(Enemy *curEnemy, float angle) {
	float nx = cosf(angle);
	float ny = sinf(angle);
	float farDistance = (MAP_WIDTH + MAP_HEIGHT) * CELL_SIZE;
	float farX = curEnemy->x + nx * farDistance;
	float farY = curEnemy->y + ny * farDistance;
	float wallX, wallY;
	castRayToWall(curEnemy->x, curEnemy->y, farX, farY, &wallX, &wallY);
	float wallDist = sqrtf(distanceSquared(wallX, wallY, curEnemy->x, curEnemy->y));
	for (float d = BOSS_WARNING_LINE_SPACING; d < wallDist; d += BOSS_WARNING_LINE_SPACING) {
		float x = curEnemy->x + nx * d;
		float y = curEnemy->y + ny * d;
		spawnParticle(PARTICLE_TYPE_WARNING, x, y, x, y, PARTICLE_TYPE_NONE, true /* enemyParticle */);
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
		fireEnemyProjectile(px, py, curEnemy->x, curEnemy->y, dx, dy, elderProperties->projectileSpeed, elderProperties->projectileDamage, curEnemyData->projectileData);
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
			bossSpawnSummonsFromBudget(curEnemy, elderProperties->summonBudget, elderProperties->summonMinSpecials, elderProperties->summonRadius);
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

// Fires a single volley shot at the aim point locked when the volley started, deflected by a
// random angle within the cone.
static void wraithFireVolleyShot(Enemy *curEnemy, EnemyData *curEnemyData) {
	float angle = atan2f(curEnemy->targetY - curEnemy->y, curEnemy->targetX - curEnemy->x) + randFloat(-0.5f, 0.5f) * wraithProperties->volleyConeAngle;
	float deltaX = cosf(angle);
	float deltaY = sinf(angle);
	float projectileX = curEnemy->x + deltaX * PROJECTILE_OFFSET;
	float projectileY = curEnemy->y + deltaY * PROJECTILE_OFFSET;
	fireEnemyProjectile(projectileX, projectileY, curEnemy->x, curEnemy->y, deltaX, deltaY, wraithProperties->volleyProjectileSpeed, curEnemyData->attackDamage, curEnemyData->projectileData);
}

// Starts one teleport rep: a spawn particle covers the boss, then it vanishes and relocates.
static void wraithBeginTeleportDisguise(Enemy *curEnemy) {
	spawnParticle(PARTICLE_TYPE_SPAWN, curEnemy->x, curEnemy->y, curEnemy->x, curEnemy->y, PARTICLE_TYPE_NONE, false /*enemyParticle*/);
	curEnemy->attackCycle = WRAITH_TELEPORT_PHASE_DISGUISE;
	curEnemy->abilityCooldown = WRAITH_TELEPORT_HIDE_DELAY;
}

// Runs the teleport-attack sub-phases stored in attackCycle: hide under a spawn particle, vanish
// and relocate, reveal with an attack intent, then fire aimed shots. stateCount counts total shots;
// every teleportShotCount shots completes one teleport rep.
static void wraithUpdateTeleportAttack(Enemy *curEnemy, EnemyData *curEnemyData) {
	if (curEnemy->abilityCooldown > 0) {
		return;
	}

	if (curEnemy->attackCycle == WRAITH_TELEPORT_PHASE_DISGUISE) {
		curEnemy->invisible = true;
		// Fall back to teleporting in place if no candidate cell is found near the player
		curEnemy->targetX = curEnemy->x;
		curEnemy->targetY = curEnemy->y;
		pickRandomRoomTargetNearPlayer(curEnemy, WRAITH_TELEPORT_MIN_PLAYER_DISTANCE, WRAITH_TELEPORT_MAX_PLAYER_DISTANCE);
		curEnemy->x = curEnemy->targetX;
		curEnemy->y = curEnemy->targetY;
		curEnemy->attackCycle = WRAITH_TELEPORT_PHASE_HIDDEN;
		curEnemy->abilityCooldown = WRAITH_TELEPORT_REVEAL_DELAY - WRAITH_TELEPORT_REVEAL_PRE_DELAY;
	} else if (curEnemy->attackCycle == WRAITH_TELEPORT_PHASE_HIDDEN) {
		// Drop the reveal particle so it leads the reappearance by the same delay as the disguise
		spawnParticle(PARTICLE_TYPE_SPAWN, curEnemy->x, curEnemy->y, curEnemy->x, curEnemy->y, PARTICLE_TYPE_NONE, false /*enemyParticle*/);
		curEnemy->attackCycle = WRAITH_TELEPORT_PHASE_REVEAL;
		curEnemy->abilityCooldown = WRAITH_TELEPORT_REVEAL_PRE_DELAY;
	} else if (curEnemy->attackCycle == WRAITH_TELEPORT_PHASE_REVEAL) {
		curEnemy->invisible = false;
		addAttackIntentIndicator(curEnemy->x, curEnemy->y, wraithProperties->teleportIntentLead);
		curEnemy->attackCycle = WRAITH_TELEPORT_PHASE_FIRE;
		curEnemy->abilityCooldown = wraithProperties->teleportIntentLead;
	} else {
		float distanceToPlayer = sqrtf(distanceSquared(playerX, playerY, curEnemy->x, curEnemy->y));
		float deltaX = (playerX - curEnemy->x) / distanceToPlayer;
		float deltaY = (playerY - curEnemy->y) / distanceToPlayer;
		float projectileX = curEnemy->x + deltaX * PROJECTILE_OFFSET;
		float projectileY = curEnemy->y + deltaY * PROJECTILE_OFFSET;
		fireEnemyProjectile(projectileX, projectileY, curEnemy->x, curEnemy->y, deltaX, deltaY, curEnemyData->abilityDataSecondary, curEnemyData->attackDamage, curEnemyData->projectileData);
		curEnemy->stateCount++;
		curEnemy->abilityCooldown = wraithProperties->projectileCooldown;
		if (curEnemy->stateCount % wraithProperties->teleportShotCount == 0) {
			if (curEnemy->stateCount >= wraithProperties->teleportCount * wraithProperties->teleportShotCount) {
				curEnemy->stateCount = 0;
				transitionToState(curEnemy, ENEMY_STATE_IDLE);
				curEnemy->stateTimer = wraithProperties->idleTime;
			} else {
				wraithBeginTeleportDisguise(curEnemy);
			}
		}
	}
}

void wraithBossUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	if (isBossCinematicActive()) {
		return;
	}
	wraithProperties = &WRAITH_BOSS_PROPERTIES[0][curEnemy->phase];

	if (curEnemy->attackCooldown <= 0 && playerInAttackDistance(index)) {
		curEnemy->attackCooldown = wraithProperties->attackCooldown;
		damagePlayerFromDirection(curEnemyData->attackDamage, curEnemy->x, curEnemy->y);
	}

	if (curEnemy->state == ENEMY_STATE_PREP && curEnemy->nextState == ENEMY_STATE_IDLE) {
		// The phase-transition cinematic leaves the boss in PREP; begin from idle with a fresh attack cycle.
		transitionToState(curEnemy, ENEMY_STATE_IDLE);
		curEnemy->stateTimer = wraithProperties->idleTime;
		curEnemy->nextState = WRAITH_STATE_DASH;
		return;
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
				// == Teleporting projectile attack: vanish, relocate, reveal, fire aimed shots ==
				transitionToState(curEnemy, WRAITH_STATE_PROJECTILE);
				curEnemy->nextState = WRAITH_STATE_VOLLEY;
				curEnemy->stateCount = 0;
				wraithBeginTeleportDisguise(curEnemy);
			} else if (curEnemy->nextState == WRAITH_STATE_VOLLEY) {
				// == Volley: rapid shots scattered within a cone locked onto the player ==
				transitionToState(curEnemy, WRAITH_STATE_VOLLEY);
				curEnemy->stateTimer = WRAITH_VOLLEY_WARNING_LEAD + wraithProperties->volleyCount * wraithProperties->volleyShotSpacing;
				curEnemy->stateCount = 0;
				curEnemy->abilityCooldown = WRAITH_VOLLEY_WARNING_LEAD;
				curEnemy->nextState = WRAITH_STATE_SUMMON;
				curEnemy->targetX = playerX;
				curEnemy->targetY = playerY;
				// Telegraph the cone: one line down the center and one along each edge
				float aimAngle = atan2f(playerY - curEnemy->y, playerX - curEnemy->x);
				float halfCone = wraithProperties->volleyConeAngle * 0.5f;
				bossSpawnWarningLine(curEnemy, aimAngle);
				bossSpawnWarningLine(curEnemy, aimAngle - halfCone);
				bossSpawnWarningLine(curEnemy, aimAngle + halfCone);
			} else if (curEnemy->nextState == WRAITH_STATE_SUMMON) {
				// == Summon wraiths in a circle around the boss, plus enemies from the area pool ==
				transitionToState(curEnemy, ENEMY_STATE_IDLE);
				curEnemy->stateTimer = wraithProperties->summonTime;
				curEnemy->nextState = WRAITH_STATE_DASH;
				curEnemy->stateCount = 0;  // The volley leaves this at volleyCount; clear it for the dash cycle
				Point bossRoomCenter = getRoomWorldCenter(DUNGEON_ROOM_COUNT - 1);
				curEnemy->targetX = bossRoomCenter.x;
				curEnemy->targetY = bossRoomCenter.y;
				// Cap the circle so active non-boss enemies never exceed the summon count
				int toSpawn = wraithProperties->spawnCount - getAliveMinionCount();
				for (int i = 0; i < toSpawn; i++) {
					float angle = (float)i / (float)wraithProperties->spawnCount * 2.0f * PI;
					float offsetX = cosf(angle) * CELL_SIZE;
					float offsetY = sinf(angle) * CELL_SIZE;
					spawnEnemyWithSpawnParticle(ENEMY_TYPE_WRAITH, curEnemy->x + offsetX, curEnemy->y + offsetY);
				}
				bossSpawnSummonsFromBudget(curEnemy, wraithProperties->spawnBudget, wraithProperties->spawnMinSpecials, wraithProperties->spawnRadius);
			}
		}
	} else if (curEnemy->state == WRAITH_STATE_DASH || curEnemy->state == WRAITH_STATE_PROJECTILE || curEnemy->state == WRAITH_STATE_SUMMON || curEnemy->state == WRAITH_STATE_VOLLEY) {
		// The PROJECTILE teleport attack manages its own exit; the others run on stateTimer
		if (curEnemy->state != WRAITH_STATE_PROJECTILE && curEnemy->stateTimer <= 0) {
			transitionToState(curEnemy, ENEMY_STATE_IDLE);
			curEnemy->stateTimer = wraithProperties->idleTime;
		}

		if (curEnemy->state == WRAITH_STATE_DASH) {
			moveEnemy(curEnemy, index, curEnemy->targetX, curEnemy->targetY, curEnemyData->abilityDataPrimary);
		} else if (curEnemy->state == WRAITH_STATE_PROJECTILE) {
			wraithUpdateTeleportAttack(curEnemy, curEnemyData);
		} else if (curEnemy->state == WRAITH_STATE_SUMMON) {
			moveEnemy(curEnemy, index, curEnemy->targetX, curEnemy->targetY, curEnemyData->moveSpeed);
		} else if (curEnemy->state == WRAITH_STATE_VOLLEY) {
			if (curEnemy->stateCount < wraithProperties->volleyCount && curEnemy->abilityCooldown <= 0) {
				wraithFireVolleyShot(curEnemy, curEnemyData);
				curEnemy->stateCount++;
				curEnemy->abilityCooldown = wraithProperties->volleyShotSpacing;
			}
		}
	}
}

// === Dragon Boss ===

// Fires a full ring of projectiles. In phase 0 it leaves four evenly-spaced gaps, one per
// quadrant: a random slot plus the slots a quarter, half, and three-quarters of the way around,
// leaving DRAGON_RING_SLOTS - 4 projectiles. In phase 1 the ring is fired solid with no gaps.
static void dragonFireRing(Enemy *curEnemy, EnemyData *curEnemyData) {
	bool leaveGaps = (curEnemy->phase == 0);
	int gap = randInt(0, DRAGON_RING_SLOTS - 1);
	int gap2 = (gap + DRAGON_RING_SLOTS / 4) % DRAGON_RING_SLOTS;
	int gap3 = (gap + DRAGON_RING_SLOTS / 2) % DRAGON_RING_SLOTS;
	int gap4 = (gap + 3 * DRAGON_RING_SLOTS / 4) % DRAGON_RING_SLOTS;
	for (int i = 0; i < DRAGON_RING_SLOTS; i++) {
		if (leaveGaps && (i == gap || i == gap2 || i == gap3 || i == gap4)) {
			continue;
		}
		float angle = i * (2.0f * PI / (float)DRAGON_RING_SLOTS);
		float dx = cosf(angle);
		float dy = sinf(angle);
		float px = curEnemy->x + dx * PROJECTILE_OFFSET;
		float py = curEnemy->y + dy * PROJECTILE_OFFSET;
		fireEnemyProjectile(px, py, curEnemy->x, curEnemy->y, dx, dy, dragonProperties->ringProjectileSpeed, dragonProperties->ringProjectileDamage, curEnemyData->projectileData);
	}
}

// Base angle for a volley's radial lines: aimed at the locked target, with odd volleys rotated half a
// sector so their lines fall in the previous volley's gaps and force the player to keep moving.
static float dragonVolleyBaseAngle(Enemy *curEnemy, int lineCount, int volleyIndex) {
	float angleToTarget = atan2f(curEnemy->targetY - curEnemy->y, curEnemy->targetX - curEnemy->x);
	float sector = 2.0f * PI / lineCount;
	return angleToTarget + (volleyIndex % 2) * (sector * 0.5f);
}

// Fires one projectile down each of lineCount evenly-spaced directions, starting from baseAngle.
static void dragonFireVolley(Enemy *curEnemy, EnemyData *curEnemyData, float baseAngle, int lineCount) {
	float sector = 2.0f * PI / lineCount;
	for (int k = 0; k < lineCount; k++) {
		float angle = baseAngle + k * sector;
		float dx = cosf(angle);
		float dy = sinf(angle);
		float px = curEnemy->x + dx * PROJECTILE_OFFSET;
		float py = curEnemy->y + dy * PROJECTILE_OFFSET;
		fireEnemyProjectile(px, py, curEnemy->x, curEnemy->y, dx, dy, dragonProperties->volleyProjectileSpeed, dragonProperties->volleyProjectileDamage, curEnemyData->projectileData);
	}
}

// Computes the world-space center of the current boss room.
static void dragonRoomCenter(float *centerX, float *centerY) {
	int roomIndex = getCurrentRoomIndex();
	Point interiorMin = getRoomInteriorMin(roomIndex);
	Point interiorMax = getRoomInteriorMax(roomIndex);
	float centerCellX = (interiorMin.x + interiorMax.x) / 2.0f;
	float centerCellY = (interiorMin.y + interiorMax.y) / 2.0f;
	*centerX = centerCellX * CELL_SIZE + CELL_SIZE / 2.0f;
	*centerY = centerCellY * CELL_SIZE + CELL_SIZE / 2.0f;
}

// Fires a single four-way burst (90 degrees apart) starting from baseAngle.
static void dragonFireSprinkler(Enemy *curEnemy, EnemyData *curEnemyData, float baseAngle) {
	for (int k = 0; k < 4; k++) {
		float angle = baseAngle + k * (PI / 2.0f);
		float dx = cosf(angle);
		float dy = sinf(angle);
		float px = curEnemy->x + dx * PROJECTILE_OFFSET;
		float py = curEnemy->y + dy * PROJECTILE_OFFSET;
		fireEnemyProjectile(px, py, curEnemy->x, curEnemy->y, dx, dy, dragonProperties->sprinklerProjectileSpeed, dragonProperties->sprinklerProjectileDamage, curEnemyData->projectileData);
	}
}

// Summons summonCount adds around the dragon (basic fodder, or specials when summonMinSpecials is set).
// The roll is capped by the count, which doubles as the active non-boss enemy limit in bossSpawnSummonsCapped.
static void dragonSummonAdds(Enemy *curEnemy) {
	int count = dragonProperties->summonCount;
	if (count > BOSS_MAX_SUMMONS) {
		count = BOSS_MAX_SUMMONS;
	}
	EnemyType summons[BOSS_MAX_SUMMONS];
	// Budget is kept generous so the roll always affords `count` picks; the count itself caps the roll.
	int rolled = rollEnemiesFromBudget(summons, count, count * 10, dragonProperties->summonMinSpecials);
	bossSpawnSummonsCapped(curEnemy, summons, rolled, dragonProperties->summonRadius);
}

#define DRAGON_WANDER_MIN_SPACING (CELL_SIZE * 4.0f)
#define DRAGON_WANDER_ARRIVAL_RADIUS (CELL_SIZE * 0.5f)

static void dragonReturnToIdle(Enemy *curEnemy) {
	transitionToState(curEnemy, ENEMY_STATE_IDLE);
	curEnemy->stateTimer = dragonProperties->attackCooldown;
	curEnemy->stateCount = 0;
	curEnemy->attackedPlayer = false;
	pickRandomRoomTarget(curEnemy, DRAGON_WANDER_MIN_SPACING);
}

// Reads the current round-robin attack from attackCycle, enters its state, and advances the cycle.
static void dragonBeginAttack(Enemy *curEnemy) {
	DragonAttackCycle cycle = (DragonAttackCycle)curEnemy->attackCycle;
	curEnemy->attackCycle = (curEnemy->attackCycle + 1) % DRAGON_ATTACK_CYCLE_COUNT;
	curEnemy->stateCount = 0;
	curEnemy->attackedPlayer = false;
	curEnemy->nextState = DRAGON_SUBSTATE_ATTACKING;
	curEnemy->abilityCooldown = 0;

	switch (cycle) {
		case DRAGON_ATTACK_CYCLE_RINGS: {
			transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
			addAttackIntentIndicator(curEnemy->x, curEnemy->y, 0.0f);
			break;
		}
		case DRAGON_ATTACK_CYCLE_SPRINKLER: {
			// Already homed in on the room center during idle, so open fire immediately
			transitionToState(curEnemy, ENEMY_STATE_ATTACK_2);
			curEnemy->nextState = DRAGON_SPRINKLER_FIRING;
			curEnemy->targetX = atan2f(playerY - curEnemy->y, playerX - curEnemy->x);
			break;
		}
		case DRAGON_ATTACK_CYCLE_VOLLEY: {
			transitionToState(curEnemy, ENEMY_STATE_ATTACK_3);
			addAttackIntentIndicator(curEnemy->x, curEnemy->y, 0.0f);
			break;
		}
		case DRAGON_ATTACK_CYCLE_SUMMON: {
			transitionToState(curEnemy, DRAGON_STATE_SUMMON);
			curEnemy->stateTimer = dragonProperties->summonTime;
			addAttackIntentIndicator(curEnemy->x, curEnemy->y, 0.0f);
			dragonSummonAdds(curEnemy);
			break;
		}
		default: {
			break;
		}
	}
}

void dragonBossUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	if (isBossCinematicActive()) {
		return;
	}
	dragonProperties = &DRAGON_BOSS_PROPERTIES[0][curEnemy->phase];

	if (curEnemy->state == ENEMY_STATE_IDLE) {
		// Wander between random room positions until the attack cooldown elapses, then start the next attack.
		// When the sprinkler is up next, home in on the room center instead so the sweep opens fire from there.
		if (curEnemy->stateTimer <= 0) {
			dragonBeginAttack(curEnemy);
		} else if (curEnemy->attackCycle == DRAGON_ATTACK_CYCLE_SPRINKLER) {
			dragonRoomCenter(&curEnemy->targetX, &curEnemy->targetY);
			moveEnemy(curEnemy, index, curEnemy->targetX, curEnemy->targetY, curEnemyData->moveSpeed);
		} else {
			float dx = curEnemy->targetX - curEnemy->x;
			float dy = curEnemy->targetY - curEnemy->y;
			if (dx * dx + dy * dy <= DRAGON_WANDER_ARRIVAL_RADIUS * DRAGON_WANDER_ARRIVAL_RADIUS) {
				pickRandomRoomTarget(curEnemy, DRAGON_WANDER_MIN_SPACING);
			}
			moveEnemy(curEnemy, index, curEnemy->targetX, curEnemy->targetY, curEnemyData->moveSpeed);
		}
		return;
	}

	if (curEnemy->state == ENEMY_STATE_PREP) {
		// Only reached when the phase-transition cinematic leaves the boss in PREP; resume the loop
		dragonReturnToIdle(curEnemy);
		return;
	}

	if (curEnemy->state == DRAGON_STATE_SUMMON) {
		// == Summon adds ==
		// Adds were spawned on entry; hold the pose until the summon timer elapses, then resume.
		if (curEnemy->stateTimer <= 0) {
			dragonReturnToIdle(curEnemy);
		}
		return;
	}

	if (curEnemy->state == ENEMY_STATE_ATTACK_1) {
		// == Slow projectile rings ==
		if (curEnemy->nextState == DRAGON_SUBSTATE_SPACING) {
			// Hold the last attack frame during the gap between rings
			curEnemy->animationFrame = curEnemyData->attackFrameEnd - 1;
			if (curEnemy->stateTimer <= 0) {
				curEnemy->nextState = DRAGON_SUBSTATE_ATTACKING;
				curEnemy->attackedPlayer = false;
				curEnemy->animationFrame = curEnemyData->attackFrameStart - 1;
				curEnemy->animationTime = curEnemyData->frameTime;
			}
			return;
		}

		// Fire one ring once the attack animation reaches the active frame
		if (!curEnemy->attackedPlayer && curEnemy->animationFrame >= curEnemyData->attackActiveFrame) {
			curEnemy->attackedPlayer = true;
			dragonFireRing(curEnemy, curEnemyData);
			curEnemy->stateCount++;
		}

		// Once the animation finishes, either rest or hold for the next ring
		if (attackAnimationReset) {
			if (curEnemy->stateCount >= dragonProperties->ringCount) {
				dragonReturnToIdle(curEnemy);
			} else {
				curEnemy->nextState = DRAGON_SUBSTATE_SPACING;
				curEnemy->stateTimer = dragonProperties->ringSpacing;
				curEnemy->animationFrame = curEnemyData->attackFrameEnd - 1;
			}
		}
		return;
	}

	if (curEnemy->state == ENEMY_STATE_ATTACK_2) {
		// == Four-armed sweeping sprinkler ==
		// The reposition-to-center happens in ENEMY_STATE_IDLE; here the dragon is already centered.
		// Fire one rotating burst per shot until the sweep is spent
		if (curEnemy->abilityCooldown <= 0) {
			float baseAngle = curEnemy->targetX - curEnemy->stateCount * dragonProperties->sprinklerSweepStep;
			dragonFireSprinkler(curEnemy, curEnemyData, baseAngle);
			curEnemy->stateCount++;
			curEnemy->abilityCooldown = dragonProperties->sprinklerShotSpacing;
			if (curEnemy->stateCount >= dragonProperties->sprinklerShots) {
				dragonReturnToIdle(curEnemy);
			}
		}
		return;
	}

	if (curEnemy->state == ENEMY_STATE_ATTACK_3) {
		// == Rapid aimed volleys ==
		int lineCount = dragonProperties->volleyLineCount;
		int shotsPerVolley = dragonProperties->volleyShots;
		int totalShots = shotsPerVolley * dragonProperties->volleyCount;

		if (curEnemy->nextState == DRAGON_SUBSTATE_SPACING) {
			// Hold the last attack frame during the gap between volleys
			curEnemy->animationFrame = curEnemyData->attackFrameEnd - 1;
			if (curEnemy->stateTimer <= 0) {
				curEnemy->nextState = DRAGON_SUBSTATE_ATTACKING;
				curEnemy->animationFrame = curEnemyData->attackFrameStart - 1;
				curEnemy->animationTime = curEnemyData->frameTime;
				curEnemy->abilityCooldown = 0;
			}
			return;
		}

		bool firstShotOfVolley = (curEnemy->stateCount % shotsPerVolley == 0);

		// Lay down warning lines along every volley direction before the first shot of each volley
		if (firstShotOfVolley && !curEnemy->attackedPlayer) {
			// Lock the aim once at the very start of the attack so one line points at the player.
			// Later volleys reuse this aim, with dragonVolleyBaseAngle's alternating half-sector
			// offset dropping their lines into the previous volley's gaps.
			if (curEnemy->stateCount == 0) {
				curEnemy->targetX = playerX;
				curEnemy->targetY = playerY;
			}
			float baseAngle = dragonVolleyBaseAngle(curEnemy, lineCount, curEnemy->stateCount / shotsPerVolley);
			float sector = 2.0f * PI / lineCount;
			for (int k = 0; k < lineCount; k++) {
				bossSpawnWarningLine(curEnemy, baseAngle + k * sector);
			}
			curEnemy->attackedPlayer = true;
			curEnemy->abilityCooldown = dragonProperties->volleyWarningLead;
		}

		// The first shot of each volley waits for the attack animation to wind up to the active frame
		bool windupReady = !firstShotOfVolley || curEnemy->animationFrame >= curEnemyData->attackActiveFrame;
		if (curEnemy->abilityCooldown <= 0 && windupReady) {
			float baseAngle = dragonVolleyBaseAngle(curEnemy, lineCount, curEnemy->stateCount / shotsPerVolley);
			dragonFireVolley(curEnemy, curEnemyData, baseAngle, lineCount);
			curEnemy->stateCount++;
			curEnemy->abilityCooldown = dragonProperties->volleyShotSpacing;

			if (curEnemy->stateCount % shotsPerVolley == 0) {
				curEnemy->attackedPlayer = false;
				if (curEnemy->stateCount >= totalShots) {
					dragonReturnToIdle(curEnemy);
				} else {
					curEnemy->nextState = DRAGON_SUBSTATE_SPACING;
					curEnemy->stateTimer = dragonProperties->volleySpacing;
					curEnemy->animationFrame = curEnemyData->attackFrameEnd - 1;
				}
			}
		}
		return;
	}
}

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

// Computes and stores the charge destination toward the player, capped at the charge distance
// (or the nearest wall). Locked in at prep-start so the telegraph matches the actual charge.
static void lockChargeTarget(Enemy *curEnemy, float chargeDistance) {
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
}

// Starts the charge toward the already-locked target, estimating its travel time.
static void beginCharge(Enemy *curEnemy, float chargeSpeed) {
    float dx = curEnemy->targetX - curEnemy->x;
    float dy = curEnemy->targetY - curEnemy->y;
    float dist = sqrtf(dx * dx + dy * dy);
    curEnemy->attackedPlayer = false;
    // Trapezoidal (accel+coast+decel) total time estimate: 1.5 × (dist / speed_per_frame / fps)
    float totalDuration = dist / (chargeSpeed * (float)DEFAULT_TARGET_FPS) * 1.5f;
    curEnemy->stateTimer = totalDuration;      // countdown: 0 signals charge end
    curEnemy->abilityCooldown = -totalDuration; // negative so auto-decrement is skipped; used as reference in ATTACK_1
}

static void setupCharge(Enemy *curEnemy, EnemyData *curEnemyData, float chargeDistance, float chargeSpeed) {
    lockChargeTarget(curEnemy, chargeDistance);
    beginCharge(curEnemy, chargeSpeed);
}

#define CHARGE_WARNING_SPACING CELL_SIZE

// Lays a line of short warning particles along the locked charge path, from the enemy to its target.
static void chargeSpawnWarningLine(Enemy *curEnemy) {
    float dx = curEnemy->targetX - curEnemy->x;
    float dy = curEnemy->targetY - curEnemy->y;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist <= 0.0f) {
        return;
    }
    float nx = dx / dist;
    float ny = dy / dist;
    for (float d = CHARGE_WARNING_SPACING; d < dist; d += CHARGE_WARNING_SPACING) {
        float x = curEnemy->x + nx * d;
        float y = curEnemy->y + ny * d;
        spawnParticle(PARTICLE_TYPE_WARNING, x, y, x, y, PARTICLE_TYPE_NONE, true /* enemyParticle */);
    }
}

#define CHARGE_WANDER_MIN_SPACING (CELL_SIZE * 3.0f)
#define CHARGE_WANDER_ARRIVAL_RADIUS (CELL_SIZE * 0.5f)

static void chargeEnemyIdle(Enemy *curEnemy, EnemyData *curEnemyData, int index) {
    playEnemyIdleSFX(curEnemy, curEnemyData);

    if (playerInAttackDetectionRange(index)) {
        if (curEnemy->attackCooldown <= 0) {
            curEnemy->attackCooldown = curEnemyData->attackCooldown;
            curEnemy->stateCount = (randInt(0, 1) == 0) ? 1 : -1;
            curEnemy->stateTimer = curEnemyData->prepTime;
            transitionToState(curEnemy, ENEMY_STATE_PREP);
            playEnemyPrepSFX(curEnemy, curEnemyData);
            lockChargeTarget(curEnemy, curEnemyData->abilityDataSecondary);
            addAttackIntentIndicator(curEnemy->x, curEnemy->y, curEnemy->stateTimer);
            chargeSpawnWarningLine(curEnemy);
            return;
        }
    } else {
        curEnemy->attackCooldown = curEnemyData->attackCooldown;
    }

    // Wander between random room positions while waiting to attack
    float dx = curEnemy->targetX - curEnemy->x;
    float dy = curEnemy->targetY - curEnemy->y;
    if (dx * dx + dy * dy <= CHARGE_WANDER_ARRIVAL_RADIUS * CHARGE_WANDER_ARRIVAL_RADIUS) {
        pickRandomRoomTarget(curEnemy, CHARGE_WANDER_MIN_SPACING);
    }
    moveEnemy(curEnemy, index, curEnemy->targetX, curEnemy->targetY, curEnemyData->moveSpeed);
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
        // Hold position while winding up; the charge target was locked in at prep-start
        if (curEnemy->stateTimer <= 0) {
            beginCharge(curEnemy, curEnemyData->abilityDataPrimary);
            transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
            if (curEnemyData->attackSFX != SFX_NONE) {
                playSoundSpatial(curEnemyData->attackSFX, curEnemy->x, curEnemy->y);
            }
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
        // Hold position while winding up; the charge target was locked in at prep-start
        if (curEnemy->stateTimer <= 0) {
            beginCharge(curEnemy, curEnemyData->abilityDataPrimary);
            transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
            if (curEnemyData->attackSFX != SFX_NONE) {
                playSoundSpatial(curEnemyData->attackSFX, curEnemy->x, curEnemy->y);
            }
        }
    } else if (curEnemy->state == ENEMY_STATE_ATTACK_1) {
        chargeEnemyAttack(curEnemy, curEnemyData, index);
    } else if (curEnemy->state == ENEMY_STATE_ATTACK_2) {
        if (curEnemy->stateTimer <= 0) {
            transitionToState(curEnemy, ENEMY_STATE_IDLE);
        }
    }
}
