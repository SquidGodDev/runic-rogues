#define TARGET_EXTENSION 1
#include "pd_api.h"

#include <stdio.h>
#include <math.h>
#include <float.h>

#include "globals.h"
#include "helpers.h"
#include "game/entities/entities.h"
#include "game/entities/projectiles.h"
#include "game/entities/pickups.h"
#include "game/entities/interactables.h"
#include "game/rendering/particles.h"
#include "game/rendering/raycasting.h"
#include "game/rendering/textures.h"
#include "game/player/player.h"
#include "game/player/spells.h"
#include "game/UI/UI.h"
#include "game/enemies/enemies.h"
#include "game/map/map.h"
#include "game/systems/audioManager.h"
#include "game/systems/progressionManager.h"
#include "game/systems/upgradeManager.h"
#include "game/settings/settings.h"

PlaydateAPI *pd = NULL;

// Run when game is launched
void initSystems(void)
{
	initTextures();
	initSettings();
	initUpgradeManager();
	initAudioManager();
	initSpellData();
}

// Run at the start of every room
int initGameScene(lua_State* L)
{	
	pd->system->resetElapsedTime();

	// Args 0/1 are the state and an unused scene object
	int currentArea = pd->lua->getArgInt(2) - 1;
	int currentRoom = pd->lua->getArgInt(3) - 1;
	RoomType roomType = pd->lua->getArgInt(4);
	int isGameStart = currentArea == 0 && currentRoom == 0;
	int isBossRoom = roomType == ROOM_TYPE_BOSS;

	// Upgrades
	initUpgradeValues(isGameStart);

	// Progression
	initProgressionManager(currentArea, currentRoom);

	// Spells
	initSpells();
	initPickups();

	// Entities
	initEntities();
	initInteractables();
	initEnemies();
	initProjectiles();
	initParticles();

	// Map
	initMap(roomType);
	initRaycasting();
	spawnEnemies(currentArea, roomType);

	// UI
	initUI(isBossRoom);

	// Player
	initPlayer(isGameStart);

	return 0;
}

int updateGameScene(lua_State* L)
{
	float deltaTime = pd->system->getElapsedTime(); // In seconds
	pd->system->resetElapsedTime();

	updatePickups(deltaTime);
	updateSpells(deltaTime);
	updatePlayer(deltaTime);
	updateEnemies(deltaTime);
	updateProjectiles(deltaTime);
	updateParticles(deltaTime);
	updateInteractables(deltaTime);
	renderScene();
	drawUI(deltaTime);

	pd->system->drawFPS(185, 108);

	pd->lua->pushNil();
	return 1;
}

int eventHandler(PlaydateAPI *playdate, PDSystemEvent event, uint32_t arg)
{
	if (event == kEventInitLua) {
		pd = playdate;
		pd->system->resetElapsedTime();
		pd->display->setScale(400 / SCREEN_WIDTH);
		pd->display->setRefreshRate(TARGET_FPS);

		srand(pd->system->getSecondsSinceEpoch(NULL));

		initSystems();
		pd->lua->addFunction(updateGameScene, "GameScene.update", NULL);
		pd->lua->addFunction(initGameScene, "GameScene.init", NULL);
	}

	return 0;
}
