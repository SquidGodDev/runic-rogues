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

static bool defaultStrafeSetting = false;
static bool toggleMovementSetting = false;
static bool movementToggled = false;

static bool inputDisabled = false;
static PDButtons suppressedButtons = 0;

void setPlayerInputDisabled(bool disabled) {
	inputDisabled = disabled;
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

void handleInput(float deltaTime)
{
	defaultStrafeSetting = !pd->system->isCrankDocked();

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

	if (weaponAnimation.isChargeSpell) {
		updateBasicSpellCharge(deltaTime, current & kButtonA);
	} else if (playerState.interactableIndex == -1 && (current & kButtonA) && !playerState.interactedOnPress) {
		bool animationCancellable = weaponAnimation.curFrame == 0 || weaponAnimation.curFrame > weaponAnimation.activeFrame;
		if(spellIsCastable(BASIC_SPELL_INDEX) && animationCancellable) {
			startWeaponAnimation(BASIC_SPELL_INDEX);
		}
	}

	// Check for double-tap of d-pad direction to dash
	if (movementStats.dashDoubleTapTime > 0) {
		movementStats.dashDoubleTapTime -= deltaTime;
	}

	PDButtons pressedDirection = pushed & (kButtonLeft | kButtonRight | kButtonUp | kButtonDown);
	if (pressedDirection && !(current & kButtonB)) {
		if (movementStats.dashDoubleTapTime > 0 && (movementStats.dashMovementDir & pressedDirection)) {
			movementStats.dashDoubleTapTime = 0;
			castUtilitySpell();
		} else {
			movementStats.dashDoubleTapTime = DOUBLE_TAP_TIME;
			movementStats.dashMovementDir = pushed & (kButtonLeft | kButtonRight | kButtonUp | kButtonDown);
		}
	}

	// if (pushed & kButtonB) {
	// 	movementToggled ^= 1;
	// }

	if (current & kButtonB) {
		if (castSpecialSpell() && (getCurrentClassType() == CLASS_MAGE || getCurrentClassType() == CLASS_RANGER)) {
			startWeaponAnimation(SPECIAL_SPELL_INDEX);
		}
	}

	int playerIsDashing = movementStats.curDashTime > 0;
	int bButtonPressed = 0;
	if (toggleMovementSetting) {
		bButtonPressed = movementToggled;
	}
	int rotationEnabled = (defaultStrafeSetting == bButtonPressed) && !playerIsDashing;
	int strafeEnabled = (defaultStrafeSetting != bButtonPressed) || playerIsDashing;
	float crankChange = inputDisabled ? 0.0f : pd->system->getCrankChange();

	if ((current & kButtonLeft) && rotationEnabled) {
		if (movementStats.turnSpeed > -movementStats.turnInitialSpeed) {
			movementStats.turnSpeed = -movementStats.turnInitialSpeed;
		}
		movementStats.turnSpeed -= movementStats.turnAcceleration * deltaTime; 
		if (movementStats.turnSpeed < -movementStats.turnMaxSpeed) {
			movementStats.turnSpeed = -movementStats.turnMaxSpeed;
		}
	} else if ((current & kButtonRight) && rotationEnabled) {
		if (movementStats.turnSpeed < movementStats.turnInitialSpeed) {
			movementStats.turnSpeed = movementStats.turnInitialSpeed;
		}
		movementStats.turnSpeed += movementStats.turnAcceleration * deltaTime; 
		if (movementStats.turnSpeed > movementStats.turnMaxSpeed) {
			movementStats.turnSpeed = movementStats.turnMaxSpeed;
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

	float adjustedTurnSpeed = movementStats.turnSpeed * deltaTime + crankChange * CRANK_SENSITIVITY;
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