#pragma once

#include "pd_api.h"

typedef enum GameState {
	GAME_STATE_ACTIVE,
	GAME_STATE_SHOP,
	GAME_STATE_RUNE_INVENTORY,
	GAME_STATE_BOSS_CINEMATIC
} GameState;

extern GameState gameState;

void setGameState(GameState newState);
void initRunStart(void);
void initGameScene(void);
void updateGameScene(float deltaTime);
