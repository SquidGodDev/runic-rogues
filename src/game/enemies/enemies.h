#pragma once

#define MAX_ENEMY_COUNT 10

typedef struct Enemy {
	float x;
	float y;
	float direction;
	float speed;
	float hitFlashTime;
	float animationTime;
	uint16_t animationFrame;
	uint16_t active;
} Enemy;

extern Enemy enemies[MAX_ENEMY_COUNT];

void initEnemies(void);
void updateEnemies(float deltaTime);
int checkEnemyCollision(float x, float y, float collisionRange);