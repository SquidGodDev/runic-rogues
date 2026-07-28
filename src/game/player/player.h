#pragma once
#include "pd_api.h"

extern float playerX, playerY;
extern float playerAngle;

extern float planeX, planeY;
extern float dirX, dirY;

extern float playerHealth;

extern int playerDashCount;

void initPlayer(void);
void notePlayerRunStart(void);
int getPlayerInteractableIndex(void);
void playerDash(void);
void healPlayer(float amount);
void clampPlayerHealth(void);
void applyMaxHealthChange(int oldMaxHealth);
bool playerIsMoving(void);
void damagePlayer(float amount);
void damagePlayerFromDirection(float amount, float sourceX, float sourceY);
void drawWeapon(float deltaTime);
void updatePlayer(float deltaTime);

void setPlayerInputDisabled(bool disabled);
void suppressHeldPlayerButtons(void);

// Tutorial ability gating: selectively disable individual actions while leaving
// movement intact. All default to enabled and reset to enabled in initPlayer().
void setPlayerBasicSpellEnabled(bool enabled);
void setPlayerSpecialSpellEnabled(bool enabled);
void setPlayerDashEnabled(bool enabled);