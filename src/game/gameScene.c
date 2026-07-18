#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/helpers.h"
#include "game/gameScene.h"
#include "game/entities/entities.h"
#include "game/entities/projectiles.h"
#include "game/entities/pickups.h"
#include "game/entities/interactables.h"
#include "game/entities/turret.h"
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
#include "game/UI/runeInventory.h"
#include "game/UI/gameMenu.h"
#include "game/UI/quitConfirmDialog.h"
#include "game/systems/timerManager.h"
#include "game/systems/spawnManager.h"
#include "game/systems/bossCinematic.h"
#include "game/systems/runStats.h"
#include "game/settings/settings.h"
#include "game/tutorial/tutorialScene.h"

PlaydateAPI *pd = NULL;

GameState gameState;
void setGameState(GameState newState) {
	gameState = newState;
}

static void showAreaEnterPopupCallback(int repeatCount, void *data) {
	showAreaEnterPopup();
}

// Run at the start of every new run
void initRunStart(void) {
	setGameState(GAME_STATE_ACTIVE);
	freeQuitScreenshot();
	setTutorialActive(false);
	initSpells();
	initRunes();
	RuneType starterRune = (RuneType)getCurrentClassData()->starterRune;
	if (starterRune != RUNE_TYPE_NONE) {
		addRune(starterRune);
	}
	freeRuneInventoryCache();
	initProgressionManager();
	initTimers();
	runStatsResetForNewRun();
	resetMusicManager();
}

// Run at the start of every floor
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
	initTurrets();

	// Map
	initMap();
	initRaycasting();

	// UI
	initUI();
	if (getCurrentRoom() == 0 && getCurrentArea() != AREA_TUTORIAL) {
		addTimer(0.7f, 1, showAreaEnterPopupCallback, NULL);
	}

	if (getCurrentRoom() == 0) {
		Music areaMusic = getCurrentAreaMusic();
		if (areaMusic == MUSIC_NONE) {
			stopSong();
		} else {
			playSong(areaMusic);
		}
	}

	// Player
	initPlayer();

	// Spawn Manager
	initSpawnManager();
}

void updateGameplaySubsystems(float deltaTime)
{
	signalRuneTrigger(RUNE_TRIGGER_TIME, deltaTime);
	updatePickups(deltaTime);
	updateSpells(deltaTime);
	updatePlayer(deltaTime);
	updateEnemies(deltaTime);
	updateTurrets(deltaTime);
	updateProjectiles(deltaTime);
	updateParticles(deltaTime);
	updateInteractables(deltaTime);
	updateTimers(deltaTime);
	renderScene();
	drawUI(deltaTime, false /*duringCinematic*/);
}

void updateGameScene(float deltaTime)
{
	tryCompleteQuitDialogInit();

	// Run timer counts gameplay, shop, and cutscene time but pauses while the
	// settings/inventory menu overlay is open.
	if (gameState != GAME_STATE_MENU) {
		runStatsTick(deltaTime);
	}

	if (gameState == GAME_STATE_BOSS_CINEMATIC) {
		if (!isBossDeathCinematicActive()) {
			setMusicPausedWithFade(true);
		}
	} else if (gameState == GAME_STATE_MENU) {
		setMusicPausedWithFade(true);
	} else if (gameState == GAME_STATE_QUIT_CONFIRM) {
		setMusicPaused(true);
	} else {
		setMusicPaused(false);
	}

	if (gameState == GAME_STATE_ACTIVE) {
		updateSpawnManager();
		updateGameplaySubsystems(deltaTime);
		if (SETTINGS.drawFPS) {
			pd->graphics->fillRect(183, 106, 17, 14, kColorBlack);
			pd->system->drawFPS(185, 108);
		}
	} else if (gameState == GAME_STATE_SHOP) {
		updateShop(deltaTime);
	} else if (gameState == GAME_STATE_MENU) {
		updateGameMenu(deltaTime);
	} else if (gameState == GAME_STATE_BOSS_CINEMATIC) {
		updateBossCinematic(deltaTime);
		updateEnemies(deltaTime);
		renderScene();
		drawUI(deltaTime, true /*duringCinematic*/);
		drawPopup(deltaTime);
	} else if (gameState == GAME_STATE_QUIT_CONFIRM) {
		updateQuitConfirmDialog(deltaTime);
	} else if (gameState == GAME_STATE_QUITTING) {
		drawQuitScreenshot();
	}

	pd->lua->pushNil();
}
