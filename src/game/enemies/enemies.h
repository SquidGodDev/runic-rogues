#pragma once

#define MAX_ENEMY_COUNT 25

typedef enum EnemyType
{
	ENEMY_TYPE_BAT,
	ENEMY_TYPE_SLIME,
	ENEMY_TYPE_BABY_SLIME,
	ENEMY_TYPE_SKELETON,
	ENEMY_TYPE_IMP,
	ENEMY_TYPE_COUNT
} EnemyType;

typedef enum EnemyState
{
	ENEMY_STATE_IDLE,
	ENEMY_STATE_ATTACK,
	ENEMY_STATE_ATTACK_END,
	ENEMY_STATE_DEAD
} EnemyState;

typedef struct Enemy {
	float x;
	float y;
	int targetMapX;
	int targetMapY;
	int health;
	float attackCooldown;
	float hitFlashTime;
	float animationTime;
	uint16_t animationFrame;
	uint16_t active;
	int state;
	int type;
} Enemy;

extern Enemy enemies[MAX_ENEMY_COUNT];

void initEnemies(void);
void updateEnemies(float deltaTime);
int checkEnemyCollision(float x, float y, float collisionRange, int damage);