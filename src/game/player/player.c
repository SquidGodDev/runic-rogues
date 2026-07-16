#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/helpers.h"
#include "globals/sceneManager.h"

#include "game/player/player.h"
#include "game/player/playerStats.h"
#include "game/player/spells/spells.h"
#include "game/player/classes/classData.h"
#include "game/UI/UI.h"
#include "game/entities/projectiles.h"
#include "game/entities/interactables.h"
#include "game/rendering/textures.h"
#include "game/map/map.h"
#include "game/systems/audioManager.h"
#include "game/systems/progressionManager.h"
#include "game/systems/runeManager.h"
#include "game/systems/runStats.h"
#include "game/settings/settings.h"

#define CRANK_SENSITIVITY 0.0075f
// Spawn the projectile slightly to the right to match hand
#define FIRE_ANGLE_OFFSET PI / 8
#define FIRE_POSITION_OFFSET CELL_SIZE / 2

#define HIT_INVINCIBILITY_TIME 0.5f // Seconds

#define DOUBLE_TAP_TIME 0.3f // Seconds

// === Extern variables ===
float playerX, playerY;
float playerAngle;
float planeX, planeY;
float dirX, dirY;
float playerHealth;
int playerDashCount;
// ========================

typedef struct PlayerState {
	bool dead;
	float hitInvincibility;
	int interactableIndex;
	bool interactedOnPress;
} PlayerState;

typedef struct PlayerMovementStats {
	// Turning (Constant)
	float turnAcceleration;
	float turnDrag;
	float turnMaxSpeed;
	float turnInitialSpeed;
	// Turning (Dynamic)
	float turnSpeed;

	// Movement speed (Constant)
	float initialSpeed;
	float acceleration;
	float drag;
	// Movement speed (Dynamic)
	float xVelocity;
	float yVelocity;
	float speed;
	float strafeSpeed;

	// Dash (Constant)
	float dashSpeed;
	float dashTime;
	// Dash (Dynamic)
	float dashAngle;
	float dashDoubleTapTime;
	PDButtons dashMovementDir;
	float curDashTime;
} PlayerMovementStats;

typedef struct WeaponAnimation {
	// Fixed constants
	float bobSpeed;
	int basePositionX;
	int basePositionY;
	int xOffset;
	int yOffset;
	int activeXOffset;
	int activeYOffset;
	LCDBitmapTable *bitmapTable;
	int bitmapFrames;
	float frameTime;
	int activeFrame;
	bool isChargeSpell;
	// Dynamic values
	int spellIndex;
	bool spellCasted;
	float bobCounter;
	float drawX;
	float drawY;
	float targetDrawX;
	float targetDrawY;
	int curFrame;
	float curFrameTime;
	int chargeLevel;     // 0 = not drawing the bow; 1..N = current charge level
	float chargeTimer;   // time accumulated toward the next charge level
	float jigglePhase;   // drives the max-charge jiggle
} WeaponAnimation;

static PlayerState playerState;
static PlayerMovementStats movementStats;
static WeaponAnimation weaponAnimation;

// Charge state for a chargeable special spell (the basic spell charges via weaponAnimation).
static int specialChargeLevel;   // 0 = not charging; 1..N = current charge level
static float specialChargeTimer; // time accumulated toward the next charge level

static bool defaultStrafeSetting = false;
static bool toggleMovementSetting = false;
static bool movementToggled = false;

// Continuous (unwrapped) crank angle in degrees, so absolute crank can gear down
// below 1:1 — at 0.5x two revolutions map to a full 360° turn. Seeded to the real
// crank angle so 1x stays identical to reading getCrankAngle() directly.
static float absoluteCrankAccum = 0.0f;
static float prevCrankAngle = 0.0f;
static bool crankAccumInit = false;

static bool inputDisabled = false;
static PDButtons suppressedButtons = 0;

static bool basicSpellEnabled = true;
static bool specialSpellEnabled = true;
static bool dashEnabled = true;

void setPlayerInputDisabled(bool disabled) {
	inputDisabled = disabled;
}

void setPlayerBasicSpellEnabled(bool enabled) {
	basicSpellEnabled = enabled;
}

void setPlayerSpecialSpellEnabled(bool enabled) {
	specialSpellEnabled = enabled;
}

void setPlayerDashEnabled(bool enabled) {
	dashEnabled = enabled;
}

void suppressHeldPlayerButtons(void) {
	PDButtons current;
	pd->system->getButtonState(&current, NULL, NULL);
	suppressedButtons |= current;
}

void initPlayer(void) {
	const ClassData *classData = getCurrentClassData();
	playerHealth = getPlayerMaxHealth();
	playerDashCount = 0;
	basicSpellEnabled = true;
	specialSpellEnabled = true;
	dashEnabled = true;
	playerState = (PlayerState) {
		.dead = 0,
		.hitInvincibility = 0,
		.interactableIndex = -1,
		.interactedOnPress = false
	};

	Point playerSpawnMapPosition = getPlayerSpawnMapPosition();
	playerX = playerSpawnMapPosition.x * CELL_SIZE + CELL_SIZE / 2;
	playerY = playerSpawnMapPosition.y * CELL_SIZE + CELL_SIZE / 2;
	playerAngle = 0;
	planeX = 0;
	planeY = -0.59f; // I think it's supposed to be -tanf(FOV_RAD / 2) ~= -0.65, but adjusted since it was off for some reason
	dirX = 1;
	dirY = 0;

	movementStats = (PlayerMovementStats) {
		.turnAcceleration = 5.0f,
		.turnDrag = 12.5f,
		.turnMaxSpeed = 2.0f,
		.turnInitialSpeed = 0.5f,
		.turnSpeed = 0,
		.initialSpeed = 20.0f,
		.acceleration = 225.0f,
		.drag = 250.0f,
		.xVelocity = 0,
		.yVelocity = 0,
		.speed = 0,
		.strafeSpeed = 0,
		.dashSpeed = 100.0f,
		.dashTime = 0.3f,
		.dashMovementDir = 0,
		.curDashTime = 0
	};

	// Animation
	Spell basicSpell = classData->basicSpell;
	SpellData basicSpellData = spellData[basicSpell];
	WeaponData weaponData = basicSpellData.weaponData;
	int weaponBasePositionX = weaponData.drawX;
	int weaponBasePositionY = weaponData.drawY;
	LCDBitmapTable *bitmapTable = BITMAP_TABLES[weaponData.weaponBitmapTable];
    int frameCount;
    pd->graphics->getBitmapTableInfo(bitmapTable, NULL, &frameCount);
	weaponAnimation = (WeaponAnimation) {
		// Fixed constants
		.bobSpeed = 0.15f,
		.basePositionX = weaponBasePositionX,
		.basePositionY = weaponBasePositionY,
		.xOffset = 5,
		.yOffset = 3,
		.activeXOffset = weaponData.activeOffsetX,
		.activeYOffset = weaponData.activeOffsetY,
		.bitmapTable = bitmapTable,
		.bitmapFrames = frameCount,
		.frameTime = weaponData.frameTime,
		.activeFrame = weaponData.activeFrame,
		.isChargeSpell = spellIsCharged(basicSpell),
		// Dynamic values
		.spellIndex = -1,
		.spellCasted = false,
		.bobCounter = 0,
		.drawX = weaponBasePositionX,
		.drawY = weaponBasePositionY,
		.targetDrawX = weaponBasePositionX,
		.targetDrawY = weaponBasePositionY,
		.curFrame = 0,
		.curFrameTime = 0,
		.chargeLevel = 0,
		.chargeTimer = 0,
		.jigglePhase = 0
	};

	specialChargeLevel = 0;
	specialChargeTimer = 0.0f;
}

void startWeaponAnimation(int spellIndex) {
	weaponAnimation.spellIndex = spellIndex;
	weaponAnimation.spellCasted = false;
	weaponAnimation.curFrameTime = weaponAnimation.frameTime;
	weaponAnimation.curFrame = 1;
}

static void updateBasicSpellCharge(float deltaTime, bool aHeld) {
	bool blocked = playerState.interactableIndex != -1 || playerState.interactedOnPress;
	bool holding = aHeld && !blocked;

	if (weaponAnimation.chargeLevel == 0) {
		if (holding && spellIsCastable(BASIC_SPELL_INDEX)) {
			weaponAnimation.chargeLevel = 1;
			weaponAnimation.chargeTimer = 0.0f;
			weaponAnimation.curFrame = 1;
		}
		return;
	}

	if (holding) {
		const ChargeData *charge = getBasicSpellChargeData();
		weaponAnimation.chargeTimer += deltaTime;
		if (weaponAnimation.chargeLevel < charge->levelCount) {
			if (weaponAnimation.chargeTimer >= charge->timePerLevel) {
				weaponAnimation.chargeTimer -= charge->timePerLevel;
				weaponAnimation.chargeLevel++;
			}
		} else {
			weaponAnimation.chargeTimer = 0.0f;
		}
		weaponAnimation.curFrame = weaponAnimation.chargeLevel;
	} else {
		castBasicSpellCharged(weaponAnimation.chargeLevel);
		weaponAnimation.chargeLevel = 0;
		weaponAnimation.chargeTimer = 0.0f;
		weaponAnimation.curFrame = 0;
	}
}

static float getBasicSpellChargeSpeedModifier(void) {
	if (!weaponAnimation.isChargeSpell || weaponAnimation.chargeLevel == 0) {
		return 1.0f;
	}
	const ChargeData *charge = getBasicSpellChargeData();
	int levelIndex = weaponAnimation.chargeLevel - 1;
	if (levelIndex >= charge->levelCount) {
		levelIndex = charge->levelCount - 1;
	}
	return 1.0f + charge->levels[levelIndex].playerSpeedModifier;
}

static void updateSpecialSpellCharge(float deltaTime, bool bHeld) {
	bool blocked = playerState.interactableIndex != -1 || playerState.interactedOnPress;
	bool holding = bHeld && !blocked;

	if (specialChargeLevel == 0) {
		if (holding && spellIsCastable(SPECIAL_SPELL_INDEX)) {
			specialChargeLevel = 1;
			specialChargeTimer = 0.0f;
		}
		return;
	}

	if (holding) {
		const ChargeData *charge = getSpecialSpellChargeData();
		specialChargeTimer += deltaTime;
		if (specialChargeLevel < charge->levelCount) {
			if (specialChargeTimer >= charge->timePerLevel) {
				specialChargeTimer -= charge->timePerLevel;
				specialChargeLevel++;
			}
		} else {
			specialChargeTimer = 0.0f;
		}
	} else {
		castSpecialSpellCharged(specialChargeLevel);
		specialChargeLevel = 0;
		specialChargeTimer = 0.0f;
	}
}

void handleInput(float deltaTime)
{
	bool crankControl = SETTINGS.controlScheme == CONTROL_SCHEME_CRANK;
	defaultStrafeSetting = crankControl && SETTINGS.dpadStrafe;

	int cellSize = CELL_SIZE;
	int mapWidth = MAP_WIDTH;

	PDButtons current;
	PDButtons pushed;
	PDButtons released;
	pd->system->getButtonState(&current, &pushed, &released);

	if (isSceneTransitioning() || playerState.dead || inputDisabled) {
		// Disable inputs
		current = 0;
		pushed = 0;
	}

	suppressedButtons &= current;
	current &= ~suppressedButtons;
	pushed &= ~suppressedButtons;

	movementStats.curDashTime -= deltaTime;
	if (movementStats.curDashTime < 0) {
		movementStats.curDashTime = 0;
	}

	if (released & kButtonA) {
		playerState.interactedOnPress = false;
	}

	if (playerState.interactableIndex != -1) {
		if (pushed & kButtonA) {
			interact(playerState.interactableIndex);
			playerState.interactedOnPress = true;
		}
	}

	PDButtons basicSpellButton = SETTINGS.spellButtonsReversed ? kButtonB : kButtonA;
	PDButtons specialSpellButton = SETTINGS.spellButtonsReversed ? kButtonA : kButtonB;
	// The A button also interacts, so whichever spell sits on A yields to interactions
	bool interactBlocked = playerState.interactableIndex != -1 || playerState.interactedOnPress;
	bool basicSpellHeld = (current & basicSpellButton) && (basicSpellButton != kButtonA || !interactBlocked);
	bool specialSpellHeld = (current & specialSpellButton) && (specialSpellButton != kButtonA || !interactBlocked);

	if (basicSpellEnabled) {
		if (weaponAnimation.isChargeSpell) {
			updateBasicSpellCharge(deltaTime, current & basicSpellButton);
		} else if (basicSpellHeld) {
			bool animationCancellable = weaponAnimation.curFrame == 0 || weaponAnimation.curFrame > weaponAnimation.activeFrame;
			if(spellIsCastable(BASIC_SPELL_INDEX) && animationCancellable) {
				startWeaponAnimation(BASIC_SPELL_INDEX);
			}
		}
	}

	// Check for double-tap of d-pad direction to dash
	if (movementStats.dashDoubleTapTime > 0) {
		movementStats.dashDoubleTapTime -= deltaTime;
	}

	PDButtons pressedDirection = pushed & (kButtonLeft | kButtonRight | kButtonUp | kButtonDown);
	if (pressedDirection && !(current & specialSpellButton)) {
		if (movementStats.dashDoubleTapTime > 0 && (movementStats.dashMovementDir & pressedDirection)) {
			movementStats.dashDoubleTapTime = 0;
			if (dashEnabled) {
				castUtilitySpell();
			}
		} else {
			movementStats.dashDoubleTapTime = DOUBLE_TAP_TIME;
			movementStats.dashMovementDir = pushed & (kButtonLeft | kButtonRight | kButtonUp | kButtonDown);
		}
	}

	// if (pushed & kButtonB) {
	// 	movementToggled ^= 1;
	// }

	if (specialSpellEnabled) {
		if (spellIsCharged(getSpecialSpell())) {
			updateSpecialSpellCharge(deltaTime, current & specialSpellButton);
		} else if (specialSpellHeld) {
			if (castSpecialSpell() && (getCurrentClassType() == CLASS_MAGE)) {
				startWeaponAnimation(SPECIAL_SPELL_INDEX);
			}
		}
	}

	int playerIsDashing = movementStats.curDashTime > 0;
	int bButtonPressed = 0;
	if (toggleMovementSetting) {
		bButtonPressed = movementToggled;
	}
	int rotationEnabled = (defaultStrafeSetting == bButtonPressed) && !playerIsDashing;
	int strafeEnabled = (defaultStrafeSetting != bButtonPressed) || playerIsDashing;
	float crankChange = (inputDisabled || !crankControl) ? 0.0f : pd->system->getCrankChange();

	// Track the crank's unwrapped angle every frame so absolute crank can gear down.
	float rawCrankAngle = pd->system->getCrankAngle();
	if (!crankAccumInit) {
		prevCrankAngle = rawCrankAngle;
		absoluteCrankAccum = rawCrankAngle;
		crankAccumInit = true;
	}
	float crankAngleDelta = rawCrankAngle - prevCrankAngle;
	while (crankAngleDelta > 180.0f) {
		crankAngleDelta -= 360.0f;
	}
	while (crankAngleDelta < -180.0f) {
		crankAngleDelta += 360.0f;
	}
	absoluteCrankAccum += crankAngleDelta;
	prevCrankAngle = rawCrankAngle;

	PDButtons turnLeftButton = SETTINGS.invertDpad ? kButtonRight : kButtonLeft;
	PDButtons turnRightButton = SETTINGS.invertDpad ? kButtonLeft : kButtonRight;
	float dpadTurnMultiplier = getDpadTurnSpeedMultiplier();
	float turnInitialSpeed = movementStats.turnInitialSpeed * dpadTurnMultiplier;
	float turnAcceleration = movementStats.turnAcceleration * dpadTurnMultiplier;
	float turnMaxSpeed = movementStats.turnMaxSpeed * dpadTurnMultiplier;

	if ((current & turnLeftButton) && rotationEnabled) {
		if (movementStats.turnSpeed > -turnInitialSpeed) {
			movementStats.turnSpeed = -turnInitialSpeed;
		}
		movementStats.turnSpeed -= turnAcceleration * deltaTime;
		if (movementStats.turnSpeed < -turnMaxSpeed) {
			movementStats.turnSpeed = -turnMaxSpeed;
		}
	} else if ((current & turnRightButton) && rotationEnabled) {
		if (movementStats.turnSpeed < turnInitialSpeed) {
			movementStats.turnSpeed = turnInitialSpeed;
		}
		movementStats.turnSpeed += turnAcceleration * deltaTime;
		if (movementStats.turnSpeed > turnMaxSpeed) {
			movementStats.turnSpeed = turnMaxSpeed;
		}
	} else {
		if (movementStats.turnSpeed < 0.5f && movementStats.turnSpeed > -0.5f) {
			movementStats.turnSpeed = 0;
		} else if (movementStats.turnSpeed > 0) {
			movementStats.turnSpeed -= movementStats.turnDrag * deltaTime;
		} else if (movementStats.turnSpeed < 0) {
			movementStats.turnSpeed += movementStats.turnDrag * deltaTime;
		}
	}

	float crankRadians;
	// Absolute crank needs a real crank angle; in d-pad control mode fall back to
	// relative turning so d-pad play isn't stuck facing the last crank angle.
	if (SETTINGS.absoluteCrank && crankControl) {
		if (inputDisabled) {
			crankRadians = 0.0f;
		} else {
			// The crank's angle maps to facing, geared by the crank ratio: at 1x the
			// accumulated angle equals the physical angle (crank up = reference), at
			// 0.5x it takes two revolutions to sweep 360°. The offset rotates the
			// reference. Rotate by the shortest delta to the target for continuity.
			float crankDeg = absoluteCrankAccum * getAbsoluteCrankRatio() + SETTINGS.absoluteCrankOffset;
			float targetAngle = degToRad(SETTINGS.invertCrank ? -crankDeg : crankDeg);
			float delta = targetAngle - playerAngle;
			while (delta > PI) {
				delta -= 2 * PI;
			}
			while (delta < -PI) {
				delta += 2 * PI;
			}
			crankRadians = delta;
		}
	} else {
		crankRadians = crankChange * CRANK_SENSITIVITY * getCrankTurnSpeedMultiplier();
		if (SETTINGS.invertCrank) {
			crankRadians = -crankRadians;
		}
	}
	float adjustedTurnSpeed = movementStats.turnSpeed * deltaTime + crankRadians;
	playerAngle += adjustedTurnSpeed;
	if (playerAngle < 0) {
		playerAngle += 2 * PI;
	} else if (playerAngle > 2 * PI) {
		playerAngle -= 2 * PI;
	}

	float oldDirX = dirX;
	dirX = dirX * cosf(adjustedTurnSpeed) - dirY * sinf(adjustedTurnSpeed);
	dirY = oldDirX * sinf(adjustedTurnSpeed) + dirY * cosf(adjustedTurnSpeed);
	float oldPlaneX = planeX;
	planeX = planeX * cosf(adjustedTurnSpeed) - planeY * sinf(adjustedTurnSpeed);
	planeY = oldPlaneX * sinf(adjustedTurnSpeed) + planeY * cosf(adjustedTurnSpeed);

	float maxSpeed = getPlayerMoveSpeed();
	maxSpeed *= getPlayerSpeedModifier();
	maxSpeed *= getBasicSpellChargeSpeedModifier();
	float acceleration = movementStats.acceleration;
	PDButtons movementDir = playerIsDashing ? movementStats.dashMovementDir : current;

	if ((movementDir & kButtonRight) && strafeEnabled) {
		if (movementStats.strafeSpeed < movementStats.initialSpeed) {
			movementStats.strafeSpeed = movementStats.initialSpeed;
		}
		movementStats.strafeSpeed += acceleration * deltaTime;
		if (movementStats.strafeSpeed > maxSpeed) {
			movementStats.strafeSpeed = maxSpeed;
		}
		if (playerIsDashing) {
			movementStats.strafeSpeed = movementStats.dashSpeed;
		}
	} else if ((movementDir & kButtonLeft) && strafeEnabled) {
		if (movementStats.strafeSpeed > -movementStats.initialSpeed) {
			movementStats.strafeSpeed = -movementStats.initialSpeed;
		}
		movementStats.strafeSpeed -= acceleration * deltaTime;
		if (movementStats.strafeSpeed < -maxSpeed) {
			movementStats.strafeSpeed = -maxSpeed;
		}
		if (playerIsDashing) {
			movementStats.strafeSpeed = -movementStats.dashSpeed;
		}
	} else {
		float dragCutoff = 0.1f * maxSpeed;
		if (movementStats.strafeSpeed < dragCutoff && movementStats.strafeSpeed > -dragCutoff) {
			movementStats.strafeSpeed = 0;
		} else if (movementStats.strafeSpeed > 0) {
			if (movementStats.strafeSpeed > maxSpeed) {
				movementStats.strafeSpeed = maxSpeed;
			}
			movementStats.strafeSpeed -= movementStats.drag * deltaTime;
		} else if (movementStats.strafeSpeed < 0) {
			if (movementStats.strafeSpeed < -maxSpeed) {
				movementStats.strafeSpeed = -maxSpeed;
			}
			movementStats.strafeSpeed += movementStats.drag * deltaTime;
		}
	}

	if (movementDir & kButtonUp) {
		if (movementStats.speed < movementStats.initialSpeed) {
			movementStats.speed = movementStats.initialSpeed;
		}
		movementStats.speed += acceleration * deltaTime;
		if (movementStats.speed > maxSpeed) {
			movementStats.speed = maxSpeed;
		}
		if (playerIsDashing) {
			movementStats.speed = movementStats.dashSpeed;
		}
	} else if (movementDir & kButtonDown) {
		if (movementStats.speed > -movementStats.initialSpeed) {
			movementStats.speed = -movementStats.initialSpeed;
		}
		movementStats.speed -= acceleration * deltaTime;
		if (movementStats.speed < -maxSpeed) {
			movementStats.speed = -maxSpeed;
		}
		if (playerIsDashing) {
			movementStats.speed = -movementStats.dashSpeed;
		}
	} else {
		float dragCutoff = 0.1f * maxSpeed;
		if (movementStats.speed < dragCutoff && movementStats.speed > -dragCutoff) {
			movementStats.speed = 0;
		} else if (movementStats.speed > 0) {
			if (movementStats.speed > maxSpeed) {
				movementStats.speed = maxSpeed;
			}
			movementStats.speed -= movementStats.drag * deltaTime;
		} else if (movementStats.speed < 0) {
			if (movementStats.speed < -maxSpeed) {
				movementStats.speed = -maxSpeed;
			}
			movementStats.speed += movementStats.drag * deltaTime;
		}
	}

	float combinedMagnitude = sqrtf((movementStats.speed * movementStats.speed) + (movementStats.strafeSpeed * movementStats.strafeSpeed));
	float normalizedSpeed = 0;
	float normalizedStrafeSpeed = 0;

	if (combinedMagnitude != 0) {
		normalizedSpeed = movementStats.speed * (fabsf(movementStats.speed) / combinedMagnitude);
		normalizedStrafeSpeed = movementStats.strafeSpeed * (fabsf(movementStats.strafeSpeed) / combinedMagnitude);
	}

	float angle = playerIsDashing ? movementStats.dashAngle : playerAngle;
	movementStats.xVelocity = cosf(angle) * normalizedSpeed + cosf(angle + PI / 2) * normalizedStrafeSpeed;
	movementStats.yVelocity = sinf(angle) * normalizedSpeed + sinf(angle + PI / 2) * normalizedStrafeSpeed;

	int wallCollisionDist = 10;
	int xOffset = 0;
	if (movementStats.xVelocity < 0) {
		xOffset = -wallCollisionDist;
	} else {
		xOffset = wallCollisionDist;
	}

	int yOffset = 0;
	if (movementStats.yVelocity < 0) {
		yOffset = -wallCollisionDist;
	} else {
		yOffset = wallCollisionDist;
	}

	// Player coordinates -> Map coordinates
	int xIndex = playerX / cellSize;
	int lookAheadXIndex = (playerX + xOffset) / cellSize;
	int yIndex = playerY / cellSize;
	int lookAheadYIndex = (playerY + yOffset) / cellSize;

	int curCell = map[yIndex * mapWidth + xIndex];
	int lookCellX = map[yIndex * mapWidth + lookAheadXIndex];
	int lookCellY = map[lookAheadYIndex * mapWidth + xIndex];

	bool canMoveX = (lookCellX == MAP_FLOOR) ||
		(lookCellX == MAP_CHASM && (playerIsDashing || curCell == MAP_CHASM));
	bool canMoveY = (lookCellY == MAP_FLOOR) ||
		(lookCellY == MAP_CHASM && (playerIsDashing || curCell == MAP_CHASM));

	if (canMoveX) {
		playerX += movementStats.xVelocity * deltaTime;
	}

	if (canMoveY) {
		playerY += movementStats.yVelocity * deltaTime;
	}

	// Weapon drawing
	if (current & kButtonLeft) {
		weaponAnimation.targetDrawX = weaponAnimation.basePositionX - weaponAnimation.xOffset;
	} else if (current & kButtonRight) {
		weaponAnimation.targetDrawX = weaponAnimation.basePositionX + weaponAnimation.xOffset;
	} else {
		weaponAnimation.targetDrawX = weaponAnimation.basePositionX;
	}

	if ((current & kButtonUp) || (current & kButtonDown)) {
		weaponAnimation.bobCounter += weaponAnimation.bobSpeed;
		if (weaponAnimation.bobCounter > 2 * PI) {
			weaponAnimation.bobCounter -= 2 * PI;
		}

		weaponAnimation.targetDrawY = weaponAnimation.basePositionY - weaponAnimation.yOffset * sinf(weaponAnimation.bobCounter);
	} else {
		weaponAnimation.bobCounter = 0;
		weaponAnimation.targetDrawY = weaponAnimation.basePositionY;
	}

	if (weaponAnimation.curFrame > 0 && weaponAnimation.chargeLevel == 0) {
		weaponAnimation.targetDrawX = weaponAnimation.basePositionX + weaponAnimation.activeXOffset;
		weaponAnimation.targetDrawY = weaponAnimation.basePositionY + weaponAnimation.activeYOffset;
	}
}

void handleInteractables(void) {
	playerState.interactableIndex = getInteractableIndexAtPosition(playerX, playerY);
}

int getPlayerInteractableIndex(void) {
	return playerState.interactableIndex;
}

void playerDash(void) {
	playerDashCount++;
	movementStats.dashAngle = playerAngle;
	movementStats.curDashTime = movementStats.dashTime;
	playerState.hitInvincibility = movementStats.dashTime * 2;
	signalRuneTrigger(RUNE_TRIGGER_DASH, 1.0f);
}

void healPlayer(float amount) {
	playerHealth += amount;
	if (playerHealth >= getPlayerMaxHealth()) {
		playerHealth = getPlayerMaxHealth();
	}
}

void damagePlayer(float amount) {
    if (playerState.hitInvincibility > 0 || playerState.dead) {
        if (!playerState.dead) {
            runStatsRecordDodge();
        }
        return;
    }

	playSound(SFX_PLAYER_HIT);
    screenShake(SCREEN_SHAKE_MEDIUM);
    flashHealthUI();
    playerState.hitInvincibility = HIT_INVINCIBILITY_TIME;
    signalRuneTrigger(RUNE_TRIGGER_DAMAGED, 1.0f);
    runStatsRecordDamageTaken((int)amount);
    playerHealth -= amount;
    if (playerHealth <= 0) {
        playerHealth = 0;
		playerState.dead = 1;
        pd->graphics->clear(kColorBlack);
        runStatsCaptureFinal(false);
        switchScene(SCENE_GAME_OVER);
    }
}

void damagePlayerFromDirection(float amount, float sourceX, float sourceY) {
    if (playerState.hitInvincibility > 0 || playerState.dead) {
        return;
    }
    addDamageIndicator(sourceX, sourceY);
    damagePlayer(amount);
}

// Continuous side-to-side shake added once the bow is fully charged
#define MAX_CHARGE_JIGGLE_SPEED 35.0f
#define MAX_CHARGE_JIGGLE_AMOUNT 2.0f

void drawWeapon(float deltaTime) {
	float lerpSpeed = 0.2f;
	weaponAnimation.drawX = lerp(weaponAnimation.drawX, weaponAnimation.targetDrawX, lerpSpeed);
	weaponAnimation.drawY = lerp(weaponAnimation.drawY, weaponAnimation.targetDrawY, lerpSpeed);

	bool charging = weaponAnimation.chargeLevel > 0;
	if (!charging) {
		if (weaponAnimation.curFrame > 0 && weaponAnimation.curFrameTime > 0) {
			weaponAnimation.curFrameTime -= deltaTime;
			if (weaponAnimation.curFrameTime <= 0) {
				weaponAnimation.curFrameTime = weaponAnimation.frameTime;
				weaponAnimation.curFrame++;
				if (weaponAnimation.curFrame >= weaponAnimation.bitmapFrames) {
					weaponAnimation.curFrame = 0;
					weaponAnimation.spellIndex = -1;
				}
			}
		}

		if (!weaponAnimation.spellCasted && weaponAnimation.curFrame >= weaponAnimation.activeFrame && weaponAnimation.spellIndex == BASIC_SPELL_INDEX) {
			castSpell(BASIC_SPELL_INDEX);
			weaponAnimation.spellCasted = true;
		}
	}

	float drawX = weaponAnimation.drawX;
	float drawY = weaponAnimation.drawY;
	const ChargeData *charge = getBasicSpellChargeData();
	bool fullyCharged = charging && weaponAnimation.chargeLevel >= charge->levelCount;
	if (fullyCharged && charge->jiggleAtMax) {
		weaponAnimation.jigglePhase += deltaTime * MAX_CHARGE_JIGGLE_SPEED;
		drawX += sinf(weaponAnimation.jigglePhase) * MAX_CHARGE_JIGGLE_AMOUNT;
	} else {
		weaponAnimation.jigglePhase = 0.0f;
	}

	LCDBitmap *weaponBitmap = pd->graphics->getTableBitmap(weaponAnimation.bitmapTable, weaponAnimation.curFrame);
	pd->graphics->drawBitmap(weaponBitmap, drawX, drawY, kBitmapUnflipped);
}

void updatePlayer(float deltaTime) {
	if (playerState.hitInvincibility > 0) {
        playerState.hitInvincibility -= deltaTime;
        if (playerState.hitInvincibility < 0) {
            playerState.hitInvincibility = 0;
        }
    }
	handleInput(deltaTime);
	handleInteractables();
	healPlayer(getPlayerHealthRegenRate() * deltaTime);
}