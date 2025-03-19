#pragma once

extern float playerX, playerY;
extern float playerAngle;

extern float planeX, planeY;
extern float dirX, dirY;

void initPlayer(void);
void handleInput(float deltaTime);
void drawWeapon(float deltaTime);