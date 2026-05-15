#pragma once

#include "stdbool.h"
#include "globals/helpers.h"

#define MAX_ENEMY_COUNT 50

#define PROJECTILE_OFFSET 3 // Distance ahead of enemy to spawn projectiles

// Boss spawn timers — also used by spawnEnemy() in enemies.c

typedef enum EnemyType
{
	ENEMY_TYPE_TEST,
	ENEMY_TYPE_BAT,
	ENEMY_TYPE_SLIME,
	ENEMY_TYPE_BABY_SLIME,
	ENEMY_TYPE_SKELETON,
	ENEMY_TYPE_SERPENT,
	ENEMY_TYPE_TURRET,
	ENEMY_TYPE_ELDER_BOSS,
	ENEMY_TYPE_RAT,
	ENEMY_TYPE_IMP,
	ENEMY_TYPE_FLYING_TOME,
	ENEMY_TYPE_LIVING_ARMOR,
	ENEMY_TYPE_MUMMY,
	ENEMY_TYPE_WRAITH_BOSS,
	ENEMY_TYPE_WRAITH,
	ENEMY_TYPE_DUMMY,
	ENEMY_TYPE_COUNT
} EnemyType;

typedef enum EnemyState
{
	ENEMY_STATE_IDLE,
	ENEMY_STATE_PREP,
	ENEMY_STATE_ATTACK_1,
	ENEMY_STATE_ATTACK_2,
	ENEMY_STATE_ATTACK_3,
	ENEMY_STATE_POST_ATTACK,
	ENEMY_STATE_DEAD
} EnemyState;

typedef struct Enemy {
	float x;
	float y;
	float targetX;
	float targetY;
	int health;
	float invincibilityTime;
	float attackCooldown;
	float abilityCooldown;
	float hitFlashTime;
	float animationTime;
	int animationFrame;
	bool active;
	int state;
	int stateCount;
	float stateTimer;
	EnemyState nextState;
	int type;
	bool isMinion;
	bool attackedPlayer;
	float slowTime;
	float slowAmount;
	float knockbackSourceX;
	float knockbackSourceY;
	float knockbackTime;
	float knockbackTimeRemaining;
	float knockbackAmount;
	bool knockbackAwayFromPlayer;
	int phase;
	int attackCycle;
} Enemy;

extern Enemy enemies[MAX_ENEMY_COUNT];

// EnemyData: static per-type configuration. Private to the enemies/ subsystem.
typedef struct EnemyData EnemyData;
typedef void (*EnemyUpdateFn)(Enemy *enemy, EnemyData *data, int index, bool attackAnimationReset);

struct EnemyData {
	char* name;
	int health;
	int attackDetectionRange;
	int attackDistance;
	int attackDamage;
	float attackCooldown;
	float moveSpeed;
	float abilityDataPrimary; // Flexible property for enemy-specific behaviors or traits
	float abilityDataSecondary; // Flexible property for enemy-specific behaviors or traits
	float prepTime;       // Seconds spent in ENEMY_STATE_PREP before attacking
	float postAttackTime; // Seconds spent in ENEMY_STATE_POST_ATTACK after attack ends
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
	EnemyUpdateFn update;
	bool knockbackResistant;
};

extern EnemyData enemyData[ENEMY_TYPE_COUNT];

// Shared helpers
void moveEnemy(Enemy *enemy, int enemyIndex, float targetX, float targetY, float moveSpeed);
int playerInAttackDetectionRange(int enemyIndex);
int playerInAttackDistance(int enemyIndex);
void transitionToState(Enemy *enemy, EnemyState newState);

void initEnemies(void);
int getEnemiesDefeated(void);
int getActiveEnemyCount(void);
int getAliveNonMinionEnemyCount(void);
int spawnEnemy(EnemyType enemyType, int x, int y, bool isMinion);
void updateEnemies(float deltaTime);
float getBossHealthPercentage(void);
void killAllEnemies(void);
void killAllNonBossEnemies(void);
bool damageEnemy(int index, int damage);
void damageEnemiesAtMapPosition(int mapX, int mapY, int damage);
void damageEnemiesInArea(float x, float y, int radius, int damage);
void knockbackEnemiesInArea(float searchX, float searchY, int radius, float sourceX, float sourceY, float knockbackTime, float knockbackAmount, bool awayFromPlayer);
void slowEnemiesInArea(float x, float y, int radius, float slowTime, float slowAmount);
bool checkEnemyCollision(float x, float y, Set *hitEnemies, float collisionRange, int damage, float slowTime, float slowAmount);