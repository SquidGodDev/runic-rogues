#define TARGET_EXTENSION 1
#include "pd_api.h"

#include <math.h>

#include "globals/globals.h"
#include "globals/helpers.h"
#include "globals/sceneManager.h"
#include "game/gameScene.h"
#include "game/player/player.h"
#include "game/enemies/enemies.h"
#include "game/UI/UI.h"
#include "game/UI/popups.h"
#include "game/systems/bossCinematic.h"
#include "game/systems/progressionManager.h"
#include "game/systems/spawnManager.h"
#include "game/systems/audioManager.h"
#include "game/systems/runStats.h"

// === Tunable cinematic parameters ===
#define BOSS_CINEMATIC_DISTANCE_CELLS 2.0f
#define BOSS_CINEMATIC_ZOOM_IN_TIME   1.5f
#define BOSS_CINEMATIC_ZOOM_OUT_TIME  1.0f
#define BOSS_CINEMATIC_HOLD_TIME      2.0f
#define BOSS_CINEMATIC_ROTATE_TIME    5.0f
#define BOSS_CINEMATIC_EASE           easeInOutQuad
#define BOSS_CINEMATIC_ROTATION_DIR   1.0f

#define BOSS_DEATH_ZOOM_IN_TIME       1.5f
#define BOSS_DEATH_ZOOM_OUT_TIME      1.5f
#define BOSS_DEATH_HOLD_TIME          1.0f
#define BOSS_DEATH_SHAKE_DURATION     0.6f  // matches screenShake(MEDIUM) duration
#define BOSS_DEATH_SHAKE_END_MAG      7.5f  // matches screenShake(MEDIUM) magnitude
// ====================================

// Plane vector magnitude — must match initPlayer in player.c
#define CAMERA_PLANE_MAGNITUDE 0.59f

typedef enum {
	BOSS_CINEMATIC_INACTIVE,
	BOSS_CINEMATIC_ZOOM_IN,
	BOSS_CINEMATIC_MIDDLE,
	BOSS_CINEMATIC_ZOOM_OUT,
	BOSS_CINEMATIC_DEATH_ZOOM_IN,
	BOSS_CINEMATIC_DEATH_HOLD,
	BOSS_CINEMATIC_DEATH_ZOOM_OUT,
} BossCinematicPhase;

static BossCinematicPhase phase = BOSS_CINEMATIC_INACTIVE;
static float phaseTimer;

static float savedX, savedY, savedAngle;
static float bossX, bossY;
static float orbitStartAngle;
static int bossEnemyIndex = -1;

static float lerpAngleShortest(float a, float b, float t) {
	float diff = fmodf(b - a + PI, 2.0f * PI);
	if (diff < 0) {
		diff += 2.0f * PI;
	}
	diff -= PI;
	return a + diff * t;
}

static void setCameraGlobals(float x, float y, float angle) {
	playerX = x;
	playerY = y;
	playerAngle = angle;
	dirX = cosf(angle);
	dirY = sinf(angle);
	planeX = sinf(angle) * CAMERA_PLANE_MAGNITUDE;
	planeY = -cosf(angle) * CAMERA_PLANE_MAGNITUDE;
}

bool isBossCinematicActive(void) {
	return phase != BOSS_CINEMATIC_INACTIVE;
}

bool isBossDeathCinematicActive(void) {
	return phase == BOSS_CINEMATIC_DEATH_ZOOM_IN
		|| phase == BOSS_CINEMATIC_DEATH_HOLD
		|| phase == BOSS_CINEMATIC_DEATH_ZOOM_OUT;
}

float getBossCinematicProgress(void) {
	if (phase == BOSS_CINEMATIC_ZOOM_IN) {
		return 0.0f;
	} else if (phase == BOSS_CINEMATIC_MIDDLE) {
		return clampFloat(phaseTimer / BOSS_CINEMATIC_HOLD_TIME, 0.0f, 1.0f);
	}
	return 1.0f;
}

void startBossCinematic(int bossIndex) {
	bossEnemyIndex = bossIndex;
	savedX = playerX;
	savedY = playerY;
	savedAngle = playerAngle;

	Enemy *boss = &enemies[bossIndex];
	bossX = boss->x;
	bossY = boss->y;

	orbitStartAngle = atan2f(savedY - bossY, savedX - bossX);

	transitionToState(boss, ENEMY_STATE_IDLE);

	playSound(SFX_CINEMATIC_SWEEP_IN);
	phase = BOSS_CINEMATIC_ZOOM_IN;
	phaseTimer = 0;
	setGameState(GAME_STATE_BOSS_CINEMATIC);
}

void startBossDeathCinematic(int bossIndex) {
	setBossActive(false);
	bossEnemyIndex = bossIndex;
	savedX = playerX;
	savedY = playerY;
	savedAngle = playerAngle;

	Enemy *boss = &enemies[bossIndex];
	bossX = boss->x;
	bossY = boss->y;

	orbitStartAngle = atan2f(savedY - bossY, savedX - bossX);

	stopSong();

	playSound(SFX_CINEMATIC_DEFEATED);

	phase = BOSS_CINEMATIC_DEATH_ZOOM_IN;
	phaseTimer = 0;
	setGameState(GAME_STATE_BOSS_CINEMATIC);
}

void updateBossCinematic(float deltaTime) {
	if (phase == BOSS_CINEMATIC_INACTIVE) {
		return;
	}

	phaseTimer += deltaTime;

	float radius = BOSS_CINEMATIC_DISTANCE_CELLS * CELL_SIZE;
	float orbitEndX = bossX + radius * cosf(orbitStartAngle);
	float orbitEndY = bossY + radius * sinf(orbitStartAngle);
	float faceBossAngleAtStart = orbitStartAngle + PI;

	if (phase == BOSS_CINEMATIC_ZOOM_IN) {
		float t = clampFloat(phaseTimer / BOSS_CINEMATIC_ZOOM_IN_TIME, 0, 1);
		float e = BOSS_CINEMATIC_EASE(t);
		float camX = savedX + (orbitEndX - savedX) * e;
		float camY = savedY + (orbitEndY - savedY) * e;
		float camAngle = lerpAngleShortest(savedAngle, faceBossAngleAtStart, e);
		setCameraGlobals(camX, camY, camAngle);
		if (phaseTimer >= BOSS_CINEMATIC_ZOOM_IN_TIME) {
			screenShake(SCREEN_SHAKE_MEDIUM);
			Enemy *boss = &enemies[bossEnemyIndex];
			transitionToState(boss, ENEMY_STATE_PREP);
			// Phase-transition re-entry keeps its brief pause; the wraith routes back through idle for it
			if (boss->phase != 0 && boss->type == ENEMY_TYPE_WRAITH_BOSS) {
				boss->nextState = ENEMY_STATE_IDLE;
			}
			phase = BOSS_CINEMATIC_MIDDLE;
			phaseTimer = 0;
			if (bossEnemyIndex >= 0) {
				if (enemies[bossEnemyIndex].phase == 0) {
					PopupType bossPopup;
					switch (enemies[bossEnemyIndex].type) {
						case ENEMY_TYPE_WRAITH_BOSS:
							playSound(SFX_BOSS_SHOUT_WRAITH);
							bossPopup = POPUP_WRAITH_BOSS;
							break;
						case ENEMY_TYPE_DRAGON_BOSS:
							playSound(SFX_BOSS_SHOUT_DRAGON);
							bossPopup = POPUP_DRAGON_BOSS;
							break;
						default:
							playSound(SFX_BOSS_SHOUT_ELDER);
							bossPopup = POPUP_ELDER_BOSS;
							break;
					}
					showPopup(bossPopup, POPUP_POSITION_TOP);
				} else {
					playSound(SFX_CINEMATIC_HEAL);
				}
			}
		}
	} else if (phase == BOSS_CINEMATIC_MIDDLE) {
		// == For old rotation around boss ==
		// float t = clampFloat(phaseTimer / BOSS_CINEMATIC_ROTATE_TIME, 0, 1);
		// float orbitAngle = orbitStartAngle + BOSS_CINEMATIC_ROTATION_DIR * 2.0f * PI * t;
		// float camX = bossX + radius * cosf(orbitAngle);
		// float camY = bossY + radius * sinf(orbitAngle);
		// float camAngle = orbitAngle + PI;
		// setCameraGlobals(camX, camY, camAngle);
		// if (phaseTimer >= BOSS_CINEMATIC_ROTATE_TIME) {
		// 	phase = BOSS_CINEMATIC_ZOOM_OUT;
		// 	phaseTimer = 0;
		// }
		setCameraGlobals(orbitEndX, orbitEndY, faceBossAngleAtStart);
		if (phaseTimer >= BOSS_CINEMATIC_HOLD_TIME) {
			playSound(SFX_CINEMATIC_SWEEP_OUT);
			phase = BOSS_CINEMATIC_ZOOM_OUT;
			phaseTimer = 0;
		}
	} else if (phase == BOSS_CINEMATIC_ZOOM_OUT) {
		float t = clampFloat(phaseTimer / BOSS_CINEMATIC_ZOOM_OUT_TIME, 0, 1);
		float e = BOSS_CINEMATIC_EASE(t);
		float camX = orbitEndX + (savedX - orbitEndX) * e;
		float camY = orbitEndY + (savedY - orbitEndY) * e;
		float camAngle = lerpAngleShortest(faceBossAngleAtStart, savedAngle, e);
		setCameraGlobals(camX, camY, camAngle);
		if (phaseTimer >= BOSS_CINEMATIC_ZOOM_OUT_TIME) {
			setCameraGlobals(savedX, savedY, savedAngle);
			if (bossEnemyIndex >= 0) {
				Enemy *boss = &enemies[bossEnemyIndex];
				// On initial room entry (phase 0) drop straight into idle so the boss attacks immediately;
				// phase-transition re-entry stays in PREP so its handler restores the usual brief pause.
				if (boss->phase == 0) {
					transitionToState(boss, ENEMY_STATE_IDLE);
				}
				boss->stateTimer = 0;
			}
			phase = BOSS_CINEMATIC_INACTIVE;
			setBossActive(true);
			setGameState(GAME_STATE_ACTIVE);
			if (DEBUG.instantKillBoss) {
				damageEnemy(bossEnemyIndex, 9999);
			}
		}
	} else if (phase == BOSS_CINEMATIC_DEATH_ZOOM_IN) {
		float t = clampFloat(phaseTimer / BOSS_DEATH_ZOOM_IN_TIME, 0, 1);
		float e = easeOutQuint(t);
		float camX = savedX + (orbitEndX - savedX) * e;
		float camY = savedY + (orbitEndY - savedY) * e;
		float camAngle = lerpAngleShortest(savedAngle, faceBossAngleAtStart, e);
		setCameraGlobals(camX, camY, camAngle);

		float shakeMag = t * BOSS_DEATH_SHAKE_END_MAG;
		screenShakeCustom(BOSS_DEATH_SHAKE_DURATION, shakeMag);

		if (phaseTimer >= BOSS_DEATH_ZOOM_IN_TIME) {
			phase = BOSS_CINEMATIC_DEATH_HOLD;
			phaseTimer = 0;
		}
	} else if (phase == BOSS_CINEMATIC_DEATH_HOLD) {
		setCameraGlobals(orbitEndX, orbitEndY, faceBossAngleAtStart);
		if (phaseTimer >= BOSS_DEATH_HOLD_TIME) {
			phase = BOSS_CINEMATIC_DEATH_ZOOM_OUT;
			phaseTimer = 0;
		}
	} else if (phase == BOSS_CINEMATIC_DEATH_ZOOM_OUT) {
		float t = clampFloat(phaseTimer / BOSS_DEATH_ZOOM_OUT_TIME, 0, 1);
		float e = BOSS_CINEMATIC_EASE(t);
		float camX = orbitEndX + (savedX - orbitEndX) * e;
		float camY = orbitEndY + (savedY - orbitEndY) * e;
		float camAngle = lerpAngleShortest(faceBossAngleAtStart, savedAngle, e);
		setCameraGlobals(camX, camY, camAngle);
		if (phaseTimer >= BOSS_DEATH_ZOOM_OUT_TIME) {
			setCameraGlobals(savedX, savedY, savedAngle);
			phase = BOSS_CINEMATIC_INACTIVE;
			onBossDefeated();
			setGameState(GAME_STATE_ACTIVE);
		}
	}
}
