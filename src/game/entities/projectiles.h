#pragma once

#define MAX_PROJECTILE_COUNT 20

typedef struct Projectile {
	float x;
	float y;
	float deltaX;
	float deltaY;
	float speed;
	int hitCount;
	uint8_t damage;
	uint8_t *texture;
	uint16_t isEnemy;
	uint16_t active;
} Projectile;

extern Projectile projectiles[MAX_PROJECTILE_COUNT];

void initProjectiles(void);
void fireProjectile(float x, float y, float deltaX, float deltaY, float speed, int damage, uint8_t *texture, int isEnemy, int hitCount);
void firePlayerProjectile(float x, float y, float deltaX, float deltaY, float speed, int damage, int hitCount);
void fireEnemyProjectile(float x, float y, float deltaX, float deltaY, float speed, int damage);
void updateProjectiles(float deltaTime);