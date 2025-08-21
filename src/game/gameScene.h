#pragma once

#include "pd_api.h"

typedef enum Scene {
	SCENE_GAME,
	SCENE_INVENTORY
} Scene;

extern Scene scene;

void setScene(Scene newScene);
int initGameScene(lua_State* L);
int updateGameScene(lua_State* L);
