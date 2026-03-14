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
#include "game/player/spells/spells.h"
#include "game/UI/UI.h"
#include "game/enemies/enemies.h"
#include "game/map/map.h"
#include "game/systems/audioManager.h"
#include "game/systems/progressionManager.h"
#include "game/systems/runeManager.h"
#include "game/systems/shopManager.h"
#include "game/systems/timerManager.h"
#include "game/systems/waveManager.h"
#include "game/settings/settings.h"

PlaydateAPI *pd = NULL;

GameState gameState;
void setGameState(GameState newState) {
	gameState = newState;
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
	setGameState(GAME_STATE_ACTIVE);
	initSpells();
	initRunes();
	initProgressionManager();
	initTimers();
}

// Run at the start of every room
void initGameScene(void)
{	
	setGameState(GAME_STATE_ACTIVE);
	pd->system->resetElapsedTime();

	// Progression
	resetProgressionManagerRoomState();

	// Spells
	resetSpellState();
	initPickups();

	// Entities
	initEntities();
	initInteractables();
	initEnemies();
	initProjectiles();
	initParticles();

	// Map
	initMap();
	initRaycasting();

	// Wave Manager
	initWaveManager();

	// UI
	initUI();

	// Player
	initPlayer();
}

void updateGameScene(float deltaTime)
{
	if (gameState == GAME_STATE_ACTIVE) {
		updatePickups(deltaTime);
		updateSpells(deltaTime);
		updatePlayer(deltaTime);
		updateEnemies(deltaTime);
		updateProjectiles(deltaTime);
		updateParticles(deltaTime);
		updateInteractables(deltaTime);
		updateWaveManager(deltaTime);
		updateTimers(deltaTime);
		renderScene();
		drawUI(deltaTime);
	} else if (gameState == GAME_STATE_SHOP) {
		updateShop(deltaTime);
	}
	
	pd->system->drawFPS(185, 108);
	pd->lua->pushNil();
}
