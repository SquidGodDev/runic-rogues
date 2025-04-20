#pragma once

#define MAX_ENEMY_COUNT 10

#define ENEMY_TYPE_COUNT 3
typedef enum EnemyType
{
	Bat,
	Slime,
	Skeleton
} EnemyType;

typedef struct Enemy {
	float x;
	float y;
	int targetMapX;
	int targetMapY;
	int health;
	float attackTime;
	float hitFlashTime;
	float animationTime;
	uint16_t animationFrame;
	uint16_t active;
	int type;
} Enemy;

extern Enemy enemies[MAX_ENEMY_COUNT];

void initEnemies(void);
void updateEnemies(float deltaTime);
int checkEnemyCollision(float x, float y, float collisionRange, int damage);