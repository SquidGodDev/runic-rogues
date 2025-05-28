#define TARGET_EXTENSION 1
#include "pd_api.h"

#include <stdio.h>
#include <math.h>
#include <float.h>

#include "globals.h"
#include "helpers.h"
#include "game/entities/entities.h"
#include "game/entities/projectiles.h"
#include "game/entities/interactables.h"
#include "game/rendering/particles.h"
#include "game/rendering/raycasting.h"
#include "game/rendering/textures.h"
#include "game/player/player.h"
#include "game/UI/UI.h"
#include "game/enemies/enemies.h"
#include "game/map/map.h"
#include "game/systems/progressionManager.h"

PlaydateAPI *pd = NULL;

int initGame(lua_State* L)
{	
	// Textures
	initTextures();

	// Progression
	initProgressionManager();

	// Entities
	initEntities();
	initInteractables();
	initEnemies();
	initProjectiles();
	initParticles();

	// Map
	int playerSpawnX, playerSpawnY;
	initMap(&playerSpawnX, &playerSpawnY);
	initRaycasting();

	spawnEnemies();

	// UI
	initUI();

	// Player
	initPlayer(playerSpawnX, playerSpawnY);

	return 0;
}

int gameUpdate(lua_State* L)
{
	float deltaTime = pd->system->getElapsedTime(); // In seconds
	pd->system->resetElapsedTime();

	updatePlayer(deltaTime);
	updateEnemies(deltaTime);
	updateProjectiles(deltaTime);
	updateParticles(deltaTime);
	updateInteractables(deltaTime);
	renderScene();
	drawUI(deltaTime);

	pd->system->drawFPS(0, 108);

	pd->lua->pushNil();
	return 1;
}

int eventHandler(PlaydateAPI *playdate, PDSystemEvent event, uint32_t arg)
{
	if (event == kEventInitLua) {
		pd = playdate;
		pd->system->resetElapsedTime();
		pd->display->setScale(400 / SCREEN_WIDTH);
		pd->display->setRefreshRate(50);

		srand(pd->system->getSecondsSinceEpoch(NULL));

		initGame(NULL);

		pd->lua->addFunction(gameUpdate, "game.update", NULL);
		pd->lua->addFunction(initGame, "game.init", NULL);
	}

	return 0;
}
