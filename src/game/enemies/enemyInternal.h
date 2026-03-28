#pragma once

#include "game/enemies/enemies.h"

#define PROJECTILE_OFFSET 3 // Distance ahead of enemy to spawn projectiles

// Boss spawn timers — also used by spawnEnemy() in enemies.c
#define ELDER_BOSS_IDLE_TIME 4.0f
#define WRAITH_BOSS_IDLE_TIME 2.0f

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
extern Enemy enemies[MAX_ENEMY_COUNT];

// Shared helpers used by pattern and boss files
void moveEnemy(Enemy *enemy, int enemyIndex, float targetX, float targetY, float moveSpeed);
int playerInAttackDetectionRange(int enemyIndex);
int playerInAttackDistance(int enemyIndex);
void transitionToState(Enemy *enemy, EnemyState newState);

// Generic enemy update functions (enemyPatterns.c)
void proximityEnemyUpdate(Enemy *enemy, EnemyData *data, int index, bool attackAnimationReset);
void meleeEnemyUpdate(Enemy *enemy, EnemyData *data, int index, bool attackAnimationReset);
void projectileEnemyUpdate(Enemy *enemy, EnemyData *data, int index, bool attackAnimationReset);
void rushEnemyUpdate(Enemy *enemy, EnemyData *data, int index, bool attackAnimationReset);
void turretEnemyUpdate(Enemy *enemy, EnemyData *data, int index, bool attackAnimationReset);
void mummyUpdate(Enemy *enemy, EnemyData *data, int index, bool attackAnimationReset);
void serpentUpdate(Enemy *enemy, EnemyData *data, int index, bool attackAnimationReset);

// Boss update functions
void elderBossUpdate(Enemy *enemy, EnemyData *data, int index, bool attackAnimationReset);  // elderBoss.c
void wraithBossUpdate(Enemy *enemy, EnemyData *data, int index, bool attackAnimationReset); // wraithBoss.c
