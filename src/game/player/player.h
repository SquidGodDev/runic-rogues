#pragma once

extern float playerX, playerY;
extern float playerAngle;

extern float planeX, planeY;
extern float dirX, dirY;

extern int playerHealth;
extern int playerShield;
extern float playerMana;

void initPlayerRunStartStats(void);
void initPlayer(void);
void updatePlayer(float deltaTime);
float getShieldRegenPercentage(void);
int getPlayerInteractableIndex(void);
void playerDash(void);
void healPlayer(int amount);
void damagePlayer(int amount);
void drawWeapon(float deltaTime);