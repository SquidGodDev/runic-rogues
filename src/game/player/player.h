#pragma once

extern float playerX, playerY;
extern float playerAngle;

extern float planeX, planeY;
extern float dirX, dirY;

extern float playerHealth;
extern float playerMana;

void initPlayer(void);
int getPlayerInteractableIndex(void);
void playerDash(void);
float getPlayerMana(void);
void healPlayer(float amount);
void damagePlayer(float amount);
void drawWeapon(float deltaTime);
void updatePlayer(float deltaTime);