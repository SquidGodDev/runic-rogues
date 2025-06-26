#pragma once

extern float playerX, playerY;
extern float playerAngle;

extern float planeX, planeY;
extern float dirX, dirY;

extern int playerHealth;
extern float playerMana;

extern int interactableIndex;

void initPlayer(int isGameStart);
void updatePlayer(float deltaTime);
void healPlayer(int amount);
void damagePlayer(int amount);
void drawWeapon(float deltaTime);