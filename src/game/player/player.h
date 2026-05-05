#pragma once
#include "pd_api.h"

extern float playerX, playerY;
extern float playerAngle;

extern float planeX, planeY;
extern float dirX, dirY;

extern float playerHealth;

extern int playerDashCount;

void initPlayer(void);
int getPlayerInteractableIndex(void);
void playerDash(void);
void healPlayer(float amount);
void damagePlayer(float amount);
void damagePlayerFromDirection(float amount, float sourceX, float sourceY);
void drawWeapon(float deltaTime);
void updatePlayer(float deltaTime);

void setPlayerInputDisabled(bool disabled);
void suppressHeldPlayerButtons(void);