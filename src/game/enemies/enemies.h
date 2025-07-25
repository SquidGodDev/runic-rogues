#pragma once

#include "stdbool.h"
#include "helpers.h"

#define MAX_ENEMY_COUNT 25

typedef enum EnemyType
{
	ENEMY_TYPE_TEST,
	ENEMY_TYPE_BAT,
	ENEMY_TYPE_SLIME,
	ENEMY_TYPE_BABY_SLIME,
	ENEMY_TYPE_SKELETON,
	ENEMY_TYPE_ELDER_BOSS,
	ENEMY_TYPE_IMP,
	ENEMY_TYPE_FLYING_TOME,
	ENEMY_TYPE_LIVING_ARMOR,
	ENEMY_TYPE_WRAITH_BOSS,
	ENEMY_TYPE_WRAITH,
	ENEMY_TYPE_COUNT
} EnemyType;

typedef enum EnemyState
{
	ENEMY_STATE_IDLE,
	ENEMY_STATE_PREP,
	ENEMY_STATE_ATTACK,
	ENEMY_STATE_ATTACK_END,
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
	int type;
	float slowTime;
	float slowAmount;
	float knockbackMaxTime;
	float knockbackTime;
	float knockbackAmount;
} Enemy;

extern Enemy enemies[MAX_ENEMY_COUNT];

void initEnemies(void);
void spawnEnemies(int currentArea, RoomType roomType);
void updateEnemies(float deltaTime);
float getBossHealthPercentage(void);
bool damageEnemy(int index, int damage, float knockbackTime, float knockbackAmount);
void damageEnemiesAtMapPosition(int mapX, int mapY, int damage);
void damageEnemiesInArea(float x, float y, int radiusSquared, int damage, float knockbackTime, float knockbackAmount);
bool checkEnemyCollision(float x, float y, Set *hitEnemies, float collisionRange, int damage, float slowTime, float slowAmount);