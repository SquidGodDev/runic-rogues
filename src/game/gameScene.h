#pragma once

#include "pd_api.h"

typedef enum GameState {
	GAME_STATE_ACTIVE,
	GAME_STATE_INVENTORY
} GameState;

extern GameState gameState;

void setGameState(GameState newState);
void initGameScene(void);
void updateGameScene(float deltaTime);
