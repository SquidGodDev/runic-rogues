#pragma once

extern float playerX, playerY;
extern float playerAngle;

extern float planeX, planeY;
extern float dirX, dirY;

extern int playerMaxHealth;
extern int playerHealth;

void initPlayer(int spawnX, int spawnY);
void updatePlayer(float deltaTime);
void damagePlayer(int amount);
void drawWeapon(float deltaTime);