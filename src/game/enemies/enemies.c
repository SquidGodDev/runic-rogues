#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/helpers.h"
#include "game/map/map.h"
#include "game/enemies/enemies.h"
#include "game/enemies/enemyAI.h"
#include "game/player/player.h"
#include "game/player/playerStats.h"
#include "game/entities/entities.h"
#include "game/entities/projectiles.h"
#include "game/entities/pickups.h"
#include "game/rendering/textures.h"
#include "game/rendering/particles.h"
#include "game/systems/audioManager.h"
#include "game/systems/runeManager.h"
#include "game/systems/progressionManager.h"
#include "game/systems/spawnManager.h"
#include "game/systems/bossCinematic.h"
#include "game/systems/runStats.h"

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
		.idleSFX = SFX_NONE,
		.prepSFX = SFX_NONE,
		.attackSFX = SFX_NONE,
		.health = 4,
		.attackDetectionRange = 10,
		.attackDistance = 10,
		.attackDamage = 1,
		.attackCooldown = 1.0f,
		.moveSpeed = 0.15f,
		.textureIndex = TEXTURE_BANSHEE,
		.idleFrameEnd = 6,
		.deathFrameStart = 7,
		.deathFrameEnd = 10,
		.frameTime = 0.1f,
		.update = proximityEnemyUpdate
	},
	[ENEMY_TYPE_DUMMY] = (EnemyData){
		.name = "Dummy",
		.health = 15,
		.attackDetectionRange = 0,
		.attackDistance = 0,
		.attackDamage = 0,
		.attackCooldown = 0,
		.moveSpeed = 0,
		.textureIndex = TEXTURE_SKELETON,
		.idleFrameEnd = 6,
		.deathFrameStart = 16,
		.deathFrameEnd = 19,
		.frameTime = 0.12f,
		.update = dummyUpdate
	},
	[ENEMY_TYPE_BAT] = (EnemyData){
		.name = "Bat",
		.idleSFX = SFX_BAT_FLAP,
		.idleSFXFrame1 = 2,
		.prepSFX = SFX_BAT_PREP,
		.attackSFX = SFX_BAT_ATTACK,
		.health = 40,
		.attackDetectionRange = 55,
		.attackDistance = 10,
		.attackDamage = 15,
		.attackCooldown = 4.0f,
		.moveSpeed = 0.15f,
		.abilityDataPrimary = 1.2f, // Charge speed
		.abilityDataSecondary = CELL_SIZE * 6.0f, // Charge distance
		.prepTime = 1.5f,
		.postAttackTime = 1.0f,
		.textureIndex = TEXTURE_BAT,
		.idleFrameEnd = 6,
		.prepFrameStart = 7,
		.prepFrameEnd = 11,
		.attackFrameStart = 1,
		.attackFrameEnd = 6,
		.deathFrameStart = 12,
		.deathFrameEnd = 15,
		.frameTime = 0.12f,
		.update = circlingChargeEnemyUpdate
	},
	[ENEMY_TYPE_SLIME] = (EnemyData){
		.name = "Slime",
		.idleSFX = SFX_SLIME_WALK,
		.idleSFXFrame1 = 3,
		.prepSFX = SFX_SLIME_PREP,
		.attackSFX = SFX_SLIME_ATTACK,
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
		.idleSFX = SFX_BABY_SLIME_WALK,
		.idleSFXFrame1 = 3,
		.prepSFX = SFX_BABY_SLIME_PREP,
		.attackSFX = SFX_BABY_SLIME_ATTACK,
		.health = 50,
		.attackDetectionRange = 35,
		.attackDistance = 20,
		.attackDamage = 5,
		.attackCooldown = 2.0f,
		.moveSpeed = 0.2f,
		.abilityDataPrimary = 0.3f, // Lunge speed
		.abilityDataSecondary = 0.5f, // Lunge duration
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
		.idleSFX = SFX_SKELETON_WALK,
		.idleSFXFrame1 = 3,
		.idleSFXFrame2 = 6,
		.prepSFX = SFX_SKELETON_PREP,
		.attackSFX = SFX_SKELETON_ATTACK,
		.health = 100,
		.attackDetectionRange = 25,
		.attackDistance = 15,
		.attackDamage = 20,
		.attackCooldown = 1.0f,
		.moveSpeed = 0.25f,
		.abilityDataPrimary = 0.9f, // Lunge speed
		.abilityDataSecondary = 0.2f, // Lunge duration
		.prepTime = 1.0f,
		.postAttackTime = 0.75f,
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
		.idleSFX = SFX_SERPENT_SLITHER,
		.idleSFXFrame1 = 1,
		.idleSFXFrame2 = 7,
		.prepSFX = SFX_SERPENT_PREP,
		.attackSFX = SFX_SERPENT_ATTACK,
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
		.attackDetectionRange = 10,
		.attackDistance = 10,
		.attackDamage = 20,
		.moveSpeed = 0.4f,
		.textureIndex = TEXTURE_ELDER_BOSS,
		.idleFrameEnd = 14,
		.prepFrameStart = 15,
		.prepFrameEnd = 22,
		.attackFrameStart = 23,
		.attackFrameEnd = 31,
		.attackActiveFrame = 27,
		.deathFrameStart = 32,
		.deathFrameEnd = 41,
		.frameTime = 0.15f,
		.update = elderBossUpdate,
		.knockbackImmune = true
	},
	[ENEMY_TYPE_RAT] = (EnemyData){
		.name = "Rat",
		.idleSFX = SFX_RAT_JUMP,
		.idleSFXFrame1 = 5,
		.prepSFX = SFX_RAT_PREP,
		.attackSFX = SFX_RAT_ATTACK,
		.health = 60,
		.attackDetectionRange = 35,
		.attackDistance = 20,
		.attackDamage = 8,
		.attackCooldown = 2.0f,
		.moveSpeed = 0.35f,
		.abilityDataPrimary = 0.3f, // Lunge speed
		.abilityDataSecondary = 0.5f, // Lunge duration
		.prepTime = 0.5f,
		.postAttackTime = 0.5f,
		.textureIndex = TEXTURE_RAT,
		.idleFrameEnd = 9,
		.prepFrameStart = 10,
		.prepFrameEnd = 14,
		.attackFrameStart = 15,
		.attackFrameEnd = 28,
		.attackActiveFrame = 20,
		.deathFrameStart = 29,
		.deathFrameEnd = 32,
		.frameTime = 0.1f,
		.update = meleeEnemyUpdate
	},
	[ENEMY_TYPE_IMP] = (EnemyData){
		.name = "Imp",
		.idleSFX = SFX_IMP_WALK,
		.idleSFXFrame1 = 1,
		.idleSFXFrame2 = 4,
		.prepSFX = SFX_IMP_PREP,
		.attackSFX = SFX_IMP_ATTACK,
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
		.idleSFX = SFX_FLYING_TOME_FLAP,
		.idleSFXFrame1 = 4,
		.prepSFX = SFX_FLYING_TOME_PREP,
		.attackSFX = SFX_FLYING_TOME_ATTACK,
		.health = 80,
		.attackDetectionRange = 65,
		.attackDistance = 10,
		.attackDamage = 20,
		.attackCooldown = 4.0f,
		.moveSpeed = 0.2f,
		.prepTime = 1.0f,
		.abilityDataPrimary = 1.4f, // Charge speed
		.abilityDataSecondary = CELL_SIZE * 8.0f, // Charge distance
		.textureIndex = TEXTURE_FLYING_TOME,
		.idleFrameEnd = 6,
		.prepFrameStart = 7,
		.prepFrameEnd = 11,
		.attackFrameStart = 7,
		.attackFrameEnd = 11,
		.deathFrameStart = 12,
		.deathFrameEnd = 15,
		.frameTime = 0.1f,
		.update = chargeEnemyUpdate
	},
	[ENEMY_TYPE_LIVING_ARMOR] = (EnemyData){
		.name = "Living Armor",
		.idleSFX = SFX_LIVING_ARMOR_WALK,
		.idleSFXFrame1 = 1,
		.prepSFX = SFX_LIVING_ARMOR_PREP,
		.attackSFX = SFX_LIVING_ARMOR_ATTACK,
		.health = 120,
		.attackDetectionRange = 22,
		.attackDistance = 12,
		.attackDamage = 20,
		.attackCooldown = 0,
		.moveSpeed = 0.37f,
		.abilityDataPrimary = 0.8f, // Lunge speed
		.abilityDataSecondary = 0.3f, // Lunge duration
		.prepTime = 1.2f,
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
		.idleSFX = SFX_MUMMY_WALK,
		.idleSFXFrame1 = 1,
		.idleSFXFrame2 = 5,
		.prepSFX = SFX_MUMMY_PREP,
		.attackSFX = SFX_MUMMY_ATTACK,
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
		.health = 850,
		.attackDetectionRange = 10,
		.attackDistance = 10,
		.attackDamage = 10,
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
		.deathFrameEnd = 20,
		.frameTime = 0.15f,
		.update = wraithBossUpdate,
		.knockbackImmune = true
	},
	[ENEMY_TYPE_WRAITH] = (EnemyData){
		.name = "Wraith",
		.idleSFX = SFX_WRAITH_IDLE,
		.idleSFXFrame1 = 1,
		.health = 20,
		.attackDetectionRange = 10,
		.attackDistance = 5,
		.attackDamage = 15,
		.attackCooldown = 1.0f,
		.moveSpeed = 0.6f,
		.textureIndex = TEXTURE_WRAITH,
		.idleFrameEnd = 4,
		.deathFrameStart = 5,
		.deathFrameEnd = 8,
		.frameTime = 0.15f,
		.update = selfDestructEnemyUpdate
	},
	[ENEMY_TYPE_BABY_DRAGON] = (EnemyData){
		.name = "Baby Dragon",
		.idleSFX = SFX_NONE,
		.prepSFX = SFX_NONE,
		.attackSFX = SFX_NONE,
		.health = 100,
		.attackDetectionRange = 25,
		.attackDamage = 12,
		.attackCooldown = 3.0f,
		.moveSpeed = 0.3f,
		.abilityDataPrimary = 30.0f, // Projectile speed
		.textureIndex = TEXTURE_BABY_DRAGON,
		.idleFrameEnd = 6,
		.attackFrameStart = 7,
		.attackFrameEnd = 18,
		.attackActiveFrame = 11,
		.deathFrameStart = 19,
		.deathFrameEnd = 22,
		.frameTime = 0.1f,
		.update = proximityProjectileEnemyUpdate
	},
	[ENEMY_TYPE_BOMBER] = (EnemyData){
		.name = "Bomber",
		.idleSFX = SFX_BOMBER_WALK,
		.idleSFXFrame1 = 2,
		.idleSFXFrame2 = 7,
		.prepSFX = SFX_NONE,
		.attackSFX = SFX_BOMBER_ATTACK,
		.health = 130,
		.attackDamage = 1,
		.attackCooldown = 0,
		.moveSpeed = 0.6f,
		.abilityDataPrimary = CELL_SIZE * 5.0f, // Minimum distance between random positions
		.abilityDataSecondary = CELL_SIZE * 0.75f, // Arrival radius for reaching a position
		.textureIndex = TEXTURE_BOMBER,
		.idleFrameEnd = 10,
		.attackFrameStart = 11,
		.attackFrameEnd = 21,
		.attackActiveFrame = 14,
		.deathFrameStart = 22,
		.deathFrameEnd = 25,
		.frameTime = 0.1f,
		.projectileData = {
			.textureIndex = TEXTURE_BOMB_PROJECTILE,
			.frameCount = 1,
			.hitParticle = PARTICLE_TYPE_ENEMY_BOMB,
			.timeoutParticle = PARTICLE_TYPE_ENEMY_BOMB,
		},
		.update = bomberUpdate
	},
	[ENEMY_TYPE_AIR_ELEMENTAL] = (EnemyData){
		.name = "Air Elemental",
		.idleSFX = SFX_NONE,
		.prepSFX = SFX_NONE,
		.attackSFX = SFX_NONE,
		.health = 100,
		.attackDetectionRange = 25,
		.attackDamage = 10,
		.attackCooldown = 0,
		.moveSpeed = 0.5f,
		.abilityDataPrimary = CELL_SIZE * 4.0f, // Minimum distance between random positions
		.abilityDataSecondary = CELL_SIZE * 0.75f, // Arrival radius for reaching a position
		.textureIndex = TEXTURE_AIR_ELEMENTAL,
		.idleFrameEnd = 8,
		.attackFrameStart = 9,
		.attackFrameEnd = 18,
		.attackActiveFrame = 17,
		.deathFrameStart = 19,
		.deathFrameEnd = 22,
		.frameTime = 0.1f,
		.projectileData = {
			.speed = 25.0f,
			.textureIndex = TEXTURE_TORNADO,
			.frameCount = 8,
			.frameTime = 0.1f,
		},
		.update = airElementalUpdate
	},
	[ENEMY_TYPE_ROCK_GOLEM] = (EnemyData){
		.name = "Rock Golem",
		.idleSFX = SFX_NONE,
		.prepSFX = SFX_NONE,
		.attackSFX = SFX_NONE,
		.health = 200,
		.attackDetectionRange = 20,
		.attackDistance = 15,
		.attackDamage = 20,
		.attackCooldown = 2.0f,
		.moveSpeed = 0.2f,
		.abilityDataPrimary = 6, // Number of rubble spawns
		.abilityDataSecondary = CELL_SIZE, // Spacing between rubble in the line
		.abilityDataTertiary = 5.0f, // Seconds before forcing an attack if it hasn't attacked
		.textureIndex = TEXTURE_ROCK_GOLEM,
		.idleFrameEnd = 8,
		.attackFrameStart = 9,
		.attackFrameEnd = 29,
		.attackActiveFrame = 18,
		.deathFrameStart = 30,
		.deathFrameEnd = 33,
		.frameTime = 0.175f,
		.update = rockGolemUpdate,
		.knockbackResistant = true
	},
	[ENEMY_TYPE_BANSHEE] = (EnemyData){
		.name = "Banshee",
		.idleSFX = SFX_NONE,
		.prepSFX = SFX_NONE,
		.attackSFX = SFX_NONE,
		.health = 200,
		.attackDetectionRange = 150,
		.attackDamage = 10,
		.attackCooldown = 0,
		.moveSpeed = 0.45f,
		.abilityDataPrimary = CELL_SIZE * 4.0f, // Minimum distance between random positions
		.abilityDataSecondary = CELL_SIZE * 0.75f, // Arrival radius for reaching a position
		.textureIndex = TEXTURE_BANSHEE,
		.idleFrameEnd = 6,
		.attackFrameStart = 7,
		.attackFrameEnd = 15,
		.attackActiveFrame = 11,
		.deathFrameStart = 16,
		.deathFrameEnd = 19,
		.frameTime = 0.12f,
		.update = bansheeUpdate
	},
	[ENEMY_TYPE_DRAGON_BOSS] = (EnemyData){
		.name = "Dragon",
		.health = 1200,
		.attackDetectionRange = 10,
		.attackDistance = 10,
		.attackDamage = 20,
		.moveSpeed = 0.3f,
		.textureIndex = TEXTURE_DRAGON,
		.idleFrameEnd = 14,
		.prepFrameStart = 15,
		.prepFrameEnd = 22,
		.attackFrameStart = 23,
		.attackFrameEnd = 33,
		.attackActiveFrame = 23,
		.deathFrameStart = 34,
		.deathFrameEnd = 47,
		.frameTime = 0.1f,
		.update = dragonBossUpdate,
		.knockbackImmune = true
	}
};

// Per-frame movement scale so enemy travel is frame-rate independent. Enemy
// move speeds and knockback are authored as per-frame steps at DEFAULT_TARGET_FPS;
// scaling by deltaTime * DEFAULT_TARGET_FPS keeps their real-time speed constant
// across the selectable refresh rates (and equals 1.0 at the default 40 FPS).
// Set at the top of updateEnemies each frame; defaults to 1.0 for any move issued
// before the first enemy update.
static float enemyFrameScale = 1.0f;

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
	enemy->x += dx * enemyFrameScale;
	enemy->y += dy * enemyFrameScale;
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
		.invisible = false,
		.state = ENEMY_STATE_IDLE,
		.stateCount = 0,
		.stateTimer = 0,
		.nextState = ENEMY_STATE_ATTACK_1,
		.type = enemyType,
		.isMinion = isMinion,
		.attackedPlayer = false,
		.slowTime = 0,
		.slowAmount = 0,
		.phase = 0,
		.attackCycle = 0
	};

	if (enemyType == ENEMY_TYPE_WRAITH_BOSS) {
		bossIndex = newEnemyIndex;
		enemies[newEnemyIndex].stateTimer = 2.0f; // Initial idle time before first attack
	} else if (enemyType == ENEMY_TYPE_ELDER_BOSS) {
		bossIndex = newEnemyIndex;
		enemies[newEnemyIndex].stateTimer = 2.0f; // Initial idle time before first attack
	} else if (enemyType == ENEMY_TYPE_DRAGON_BOSS) {
		bossIndex = newEnemyIndex;
		enemies[newEnemyIndex].stateTimer = 2.0f; // Initial idle time before first attack
	} else if (enemyType == ENEMY_TYPE_ROCK_GOLEM) {
		enemies[newEnemyIndex].stateTimer = curEnemyData.abilityDataTertiary + randFloat(-1.0f, 1.0f); // Force-attack timer
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

int getAliveMinionCount(void) {
	int count = 0;
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		if (enemies[i].active && enemies[i].state != ENEMY_STATE_DEAD && enemies[i].isMinion) {
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
	curEnemy->animationFrameAdvanced = false;
	curEnemy->animationTime -= deltaTime;
	if (curEnemy->animationTime <= 0) {
		curEnemy->animationFrameAdvanced = true;
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

			enemy->x += dx * enemyFrameScale;
			enemy->y += dy * enemyFrameScale;
		}
	}
}

void updateEnemies(float deltaTime) {
	enemyFrameScale = deltaTime * DEFAULT_TARGET_FPS;
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

		if (!curEnemy->invisible) {
			addEntityToRenderQueue(curEnemy->x, curEnemy->y, ENEMY, i, curEnemyData->textureIndex, curEnemy->animationFrame);
		}
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

EnemyType getBossEnemyType(void) {
	if (bossIndex == -1) {
		return ENEMY_TYPE_COUNT;
	}
	return enemies[bossIndex].type;
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

	if (enemyData[curEnemy.type].knockbackImmune) {
		return;
	}

	float appliedKnockback = knockbackAmount;
	if (enemyData[curEnemy.type].knockbackResistant) {
		appliedKnockback *= 0.5f;
	}

	if (knockbackTime > 0 && appliedKnockback != 0) {
		enemies[index].knockbackTime = knockbackTime;
		enemies[index].knockbackTimeRemaining = knockbackTime;
		enemies[index].knockbackAmount = appliedKnockback;
		enemies[index].knockbackSourceX = knockbackSourceX;
		enemies[index].knockbackSourceY = knockbackSourceY;
		enemies[index].knockbackAwayFromPlayer = awayFromPlayer;
	}
}

void cancelEnemyAttack(int index) {
	Enemy *curEnemy = &enemies[index];
	if (!curEnemy->active || enemyData[curEnemy->type].knockbackImmune) {
		return;
	}

	if (curEnemy->state == ENEMY_STATE_PREP ||
		curEnemy->state == ENEMY_STATE_ATTACK_1 ||
		curEnemy->state == ENEMY_STATE_ATTACK_2 ||
		curEnemy->state == ENEMY_STATE_ATTACK_3) {
		curEnemy->attackedPlayer = false;
		curEnemy->stateTimer = 0;
		curEnemy->stateCount = 0;
		curEnemy->attackCooldown = enemyData[curEnemy->type].attackCooldown;
		transitionToState(curEnemy, ENEMY_STATE_IDLE);
	}
}

void killEnemy(int index) {
	Enemy curEnemy = enemies[index];
	if (curEnemy.active && curEnemy.state != ENEMY_STATE_DEAD) {
		EnemyData curEnemyData = enemyData[curEnemy.type];
		curEnemy.state = ENEMY_STATE_DEAD;
		curEnemy.animationFrame = curEnemyData.deathFrameStart - 1;
		curEnemy.animationTime = curEnemyData.frameTime;
		curEnemy.hitFlashTime = 0;
		curEnemy.invisible = false;
		enemies[index] = curEnemy;
	}
}

void killAllEnemies(void) {
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		killEnemy(i);
	}
}

void killAllNonBossEnemies(void) {
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		if (i == bossIndex) {
			continue;
		}
		killEnemy(i);
	}
}

bool damageEnemy(int index, int damage) {
	Enemy curEnemy = enemies[index];
	if (curEnemy.state == ENEMY_STATE_DEAD || curEnemy.invincibilityTime > 0) {
		return false;
	}

	if (runeIsOwned(RUNE_TYPE_VIKING) && playerHealth < getPlayerMaxHealth() * 0.5f) {
		damage = (int)(damage * getRuneValue(RUNE_TYPE_VIKING));
	}

	runStatsRecordDamageDealt(damage);
	runStatsCreditSyncHit();

	if (runeIsOwned(RUNE_TYPE_VAMPIRISM) && randFloat(0, 1) < getRuneValue(RUNE_TYPE_VAMPIRISM)) {
		healPlayer(1);
	}

	EnemyData curEnemyData = enemyData[curEnemy.type];
	curEnemy.hitFlashTime = HIT_FLASH_TIME;
	signalRuneTrigger(RUNE_TRIGGER_DAMAGE_ENEMY, 1.0f);
	curEnemy.health -= damage;
	playSoundSpatial(SFX_ENEMY_HIT, curEnemy.x, curEnemy.y);

	if (curEnemy.health <= 0) {
		if (index == bossIndex && curEnemy.phase == 0) {
			// Transition to phase 2 instead of dying
			curEnemy.phase = 1;
			curEnemy.health = curEnemyData.health;
			curEnemy.hitFlashTime = 0;
			curEnemy.invisible = false;
			enemies[index] = curEnemy;
			clearProjectiles();
			clearParticles();
			killAllNonBossEnemies();
			if (!DEBUG.instantKillBoss) {
				startBossCinematic(index);
				return true;
			}
		}

		signalRuneTrigger(RUNE_TRIGGER_KILL_ENEMY, 1.0f);
		playSoundSpatial(SFX_ENEMY_DEATH, curEnemy.x, curEnemy.y);
		bool summonsMinions = enemyDeathEffect(index);

		curEnemy.state = ENEMY_STATE_DEAD;
		curEnemy.animationFrame = curEnemyData.deathFrameStart - 1;
		curEnemy.animationTime = curEnemyData.frameTime;
		curEnemy.hitFlashTime = 0;
		curEnemy.invisible = false;

		if (index == bossIndex) {
			clearProjectiles();
			clearParticles();
			killAllNonBossEnemies();
			startBossDeathCinematic(index);
		} else if (!summonsMinions) {
			// Removed in favor of shop on each room completion
			// spawnPickup(curEnemy.x, curEnemy.y, PICKUP_TYPE_XP_ORB);
		}

		if (index != bossIndex && !curEnemy.isMinion) {
			enemiesDefeated++;
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

void knockbackEnemiesInArea(float searchX, float searchY, int radius, float sourceX, float sourceY, float knockbackTime, float knockbackAmount, bool awayFromPlayer) {
	int radiusSquared = radius * radius;
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		Enemy enemy = enemies[i];
		if (!enemy.active) {
			continue;
		}
		float distSquared = distanceSquared(enemy.x, enemy.y, searchX, searchY);
		if (distSquared < radiusSquared) {
			knockbackEnemy(i, sourceX, sourceY, knockbackTime, knockbackAmount, awayFromPlayer);
		}
	}
}

void cancelEnemyAttacksInArea(float x, float y, int radius) {
	int radiusSquared = radius * radius;
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		Enemy enemy = enemies[i];
		if (!enemy.active) {
			continue;
		}
		float distSquared = distanceSquared(enemy.x, enemy.y, x, y);
		if (distSquared < radiusSquared) {
			cancelEnemyAttack(i);
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

int checkEnemyCollision(float x, float y, Set *hitEnemies, float collisionRange, int damage, float slowTime, float slowAmount) {
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
				return i;
			}
		}
	}
	return -1;
}

