#pragma once

#include "stdbool.h"
#include "helpers.h"

#define MAX_ENEMY_COUNT 50

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
	ENEMY_TYPE_COUNT
} EnemyType;

typedef enum EnemyState
{
	ENEMY_STATE_IDLE,
	ENEMY_STATE_PREP,
	ENEMY_STATE_ATTACK_1,
	ENEMY_STATE_ATTACK_2,
	ENEMY_STATE_ATTACK_3,
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
	float knockbackMaxTime;
	float knockbackTime;
	float knockbackAmount;
} Enemy;

extern Enemy enemies[MAX_ENEMY_COUNT];

void initEnemies(void);
int getEnemiesDefeated(void);
int getActiveEnemyCount(void);
int spawnEnemy(EnemyType enemyType, int x, int y, bool isMinion);
void spawnEnemies(int currentArea, RoomType roomType);
void updateEnemies(float deltaTime);
float getBossHealthPercentage(void);
bool damageEnemy(int index, int damage, float knockbackTime, float knockbackAmount);
void damageEnemiesAtMapPosition(int mapX, int mapY, int damage);
void damageEnemiesInArea(float x, float y, int radius, int damage, float knockbackTime, float knockbackAmount);
bool checkEnemyCollision(float x, float y, Set *hitEnemies, float collisionRange, int damage, float slowTime, float slowAmount);