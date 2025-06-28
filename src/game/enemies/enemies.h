#pragma once

#define MAX_ENEMY_COUNT 25

typedef enum EnemyType
{
	ENEMY_TYPE_TEST,
	ENEMY_TYPE_BAT,
	ENEMY_TYPE_SLIME,
	ENEMY_TYPE_BABY_SLIME,
	ENEMY_TYPE_SKELETON,
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
	float attackCooldown;
	float abilityCooldown;
	float hitFlashTime;
	float animationTime;
	uint16_t animationFrame;
	uint16_t active;
	int state;
	int type;
} Enemy;

extern Enemy enemies[MAX_ENEMY_COUNT];

void initEnemies(void);
void spawnEnemies(int currentArea, RoomType roomType);
void updateEnemies(float deltaTime);
float getBossHealthPercentage(void);
int checkEnemyCollision(float x, float y, float collisionRange, int damage);