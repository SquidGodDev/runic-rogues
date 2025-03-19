#define TARGET_EXTENSION 1
#include "pd_api.h"

#include <stdio.h>
#include <math.h>
#include <float.h>

#include "globals.h"
#include "helpers.h"
#include "game/rendering/drawables.h"
#include "game/rendering/raycasting.h"
#include "game/rendering/textures.h"
#include "game/player/player.h"
#include "game/player/projectiles.h"
#include "game/enemies/enemies.h"
#include "game/map/map.h"

PlaydateAPI *pd = NULL;

void gameInit(void)
{	
	// Textures
	initTextures();

	// Map
	initMap();

	// Drawables
	initDrawables();
	initEnemies();
	initProjectiles();

	// Player
	initPlayer();
}

int gameUpdate(lua_State* L)
{
	float deltaTime = pd->system->getElapsedTime();
	pd->system->resetElapsedTime();

	pd->display->setRefreshRate(50);

	handleInput(deltaTime);
	updateEnemies(deltaTime);
	updateProjectiles(deltaTime);
	renderScene();
	drawWeapon(deltaTime);

	pd->system->drawFPS(0, 0);

	pd->lua->pushNil();
	return 1;
}

int eventHandler(PlaydateAPI *playdate, PDSystemEvent event, uint32_t arg)
{
	if (event == kEventInitLua) {
		pd = playdate;
		pd->system->resetElapsedTime();
		pd->display->setScale(400 / SCREEN_WIDTH);

		gameInit();

		pd->lua->addFunction(gameUpdate, "game.update", NULL);
	}

	return 0;
}
