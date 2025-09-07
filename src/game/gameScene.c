#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/helpers.h"
#include "game/gameScene.h"
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
#include "game/UI/inventory.h"
#include "game/enemies/enemies.h"
#include "game/map/map.h"
#include "game/systems/audioManager.h"
#include "game/systems/progressionManager.h"
#include "game/systems/runeManager.h"
#include "game/systems/shopManager.h"
#include "game/systems/waveManager.h"
#include "game/settings/settings.h"

PlaydateAPI *pd = NULL;

Scene scene;
void setScene(Scene newScene) {
	scene = newScene;
}

// Run when game is launched
void initSystems(void)
{
	initTextures();
	initSettings();
	initAudioManager();
}

// Run at the start of every new run
void initRunStart(void) {
	setScene(SCENE_GAME);
	initDeck();
	initRunes();
}

// Run at the start of every room
int initGameScene(lua_State* L)
{	
    createInventoryMenuItem();
	setScene(SCENE_GAME);
	pd->system->resetElapsedTime();

	// Args 0/1 are the state and an unused scene object
	int currentArea = pd->lua->getArgInt(2) - 1;
	int currentRoom = pd->lua->getArgInt(3) - 1;
	RoomType roomType = pd->lua->getArgInt(4);
	bool isBossRoom = roomType == ROOM_TYPE_BOSS;

	// Progression
	initProgressionManager(currentArea, currentRoom, roomType);

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

	// Wave Manager
	initWaveManager();

	// UI
	initUI(isBossRoom);

	// Player
	initPlayer();

	return 0;
}

int updateGameScene(lua_State* L)
{
	float deltaTime = pd->system->getElapsedTime(); // In seconds
	pd->system->resetElapsedTime();
	if (scene == SCENE_GAME) {
		updatePickups(deltaTime);
		updateSpells(deltaTime);
		updatePlayer(deltaTime);
		updateEnemies(deltaTime);
		updateProjectiles(deltaTime);
		updateParticles(deltaTime);
		updateInteractables(deltaTime);
		updateWaveManager(deltaTime);
		renderScene();
		drawUI(deltaTime);

		pd->system->drawFPS(185, 108);
	} else if (scene == SCENE_INVENTORY) {
		updateInventory(deltaTime);
	}

	pd->lua->pushNil();
	return 1;
}
