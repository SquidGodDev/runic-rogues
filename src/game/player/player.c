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
	// Dynamic values
	int spellIndex;
	float bobCounter;
	float drawX;
	float drawY;
	float targetDrawX;
	float targetDrawY;
	int curFrame;
	float curFrameTime;
} WeaponAnimation;

static PlayerState playerState;
static PlayerMovementStats movementStats;
static WeaponAnimation weaponAnimation;

static bool defaultStrafeSetting = false;
static bool toggleMovementSetting = false;
static bool movementToggled = false;

void initPlayer(void) {
	const ClassData *classData = getCurrentClassData();
	playerHealth = getPlayerMaxHealth();
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

	playerAngle = pd->system->getCrankAngle() * CRANK_SENSITIVITY;
	if (playerAngle < 0) {
		playerAngle += 2 * PI;
	} else if (playerAngle >= 2 * PI) {
		playerAngle -= 2 * PI;
	}
	float oldDirX = dirX;
	dirX = dirX * cosf(playerAngle) - dirY * sinf(playerAngle);
	dirY = oldDirX * sinf(playerAngle) + dirY * cosf(playerAngle);
	float oldPlaneX = planeX;
	planeX = planeX * cosf(playerAngle) - planeY * sinf(playerAngle);
	planeY = oldPlaneX * sinf(playerAngle) + planeY * cosf(playerAngle);

	movementStats = (PlayerMovementStats) {
		.turnAcceleration = 5.0f,
		.turnDrag = 12.5f,
		.turnMaxSpeed = 2.0f,
		.turnInitialSpeed = 0.5f,
		.turnSpeed = 0,
		.initialSpeed = 20.0f,
		.acceleration = 175.0f,
		.drag = 175.0f,
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
		// Dynamic values
		.spellIndex = -1,
		.bobCounter = 0,
		.drawX = weaponBasePositionX,
		.drawY = weaponBasePositionY,
		.targetDrawX = weaponBasePositionX,
		.targetDrawY = weaponBasePositionY,
		.curFrame = 0,
		.curFrameTime = 0
	};
}

void startWeaponAnimation(int spellIndex) {
	weaponAnimation.spellIndex = spellIndex;
	weaponAnimation.curFrameTime = weaponAnimation.frameTime;
	weaponAnimation.curFrame = 1;
}

void handleInput(float deltaTime)
{
	if (!pd->system->isCrankDocked() && !defaultStrafeSetting) {
		defaultStrafeSetting = true;
	}

	int cellSize = CELL_SIZE;
	int mapWidth = MAP_WIDTH;

	PDButtons current;
	PDButtons pushed;
	PDButtons released;
	pd->system->getButtonState(&current, &pushed, &released);

	if (isSceneTransitioning() || playerState.dead) {
		// Disable inputs
		current = 0;
		pushed = 0;
	}

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
	} else if ((current & kButtonA) && !playerState.interactedOnPress) {
		if(spellIsCastable(BASIC_SPELL_INDEX) && weaponAnimation.curFrame == 0 && weaponAnimation.spellIndex != BASIC_SPELL_INDEX) {
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
		if (castSpecialSpell() && getCurrentClassType() == CLASS_MAGE) {
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
	float crankChange = pd->system->getCrankChange();

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

	int curInteractableIndex = getPlayerInteractableIndex();
	if (curInteractableIndex != -1) {
		Interactable curInteractable = INTERACTABLES[curInteractableIndex];
		if (curInteractable.active && curInteractable.type == INTERACTABLE_TYPE_SHOP_ITEM) {
			if (adjustShopDescOffset(crankChange)) {
				crankChange = 0;
			}
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

	if (map[yIndex * mapWidth + lookAheadXIndex] == 0) {
		playerX += movementStats.xVelocity * deltaTime;
	}

	if (map[lookAheadYIndex * mapWidth + xIndex] == 0) {
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

	if (weaponAnimation.curFrame > 0) {
		weaponAnimation.targetDrawX = weaponAnimation.basePositionX + weaponAnimation.activeXOffset;
		weaponAnimation.targetDrawY = weaponAnimation.basePositionY + weaponAnimation.activeYOffset;
	}
}

void handleInteractables(void) {
	int previousInteractableIndex = playerState.interactableIndex;
	playerState.interactableIndex = getInteractableIndexAtPosition(playerX, playerY);

	if (playerState.interactableIndex != previousInteractableIndex) {
		resetShopDescOffset();
	}
}

int getPlayerInteractableIndex(void) {
	return playerState.interactableIndex;
}

void playerDash(void) {
	movementStats.dashAngle = playerAngle;
	movementStats.curDashTime = movementStats.dashTime;
	playerState.hitInvincibility = movementStats.dashTime * 2;
}

void healPlayer(float amount) {
	playerHealth += amount;
	if (playerHealth >= getPlayerMaxHealth()) {
		playerHealth = getPlayerMaxHealth();
	}
}

void damagePlayer(float amount) {
    if (playerState.hitInvincibility > 0 || playerState.dead) {
        return;
    }

	playSound(SFX_PLAYER_HIT);
    screenShake(SCREEN_SHAKE_MEDIUM);
    playerState.hitInvincibility = HIT_INVINCIBILITY_TIME;
    playerHealth -= amount;
    if (playerHealth <= 0) {
        playerHealth = 0;
		playerState.dead = 1;
        pd->graphics->clear(kColorBlack);
        switchScene(SCENE_GAME_OVER);
    }
}

void drawWeapon(float deltaTime) {
	float lerpSpeed = 0.2f;
	weaponAnimation.drawX = lerp(weaponAnimation.drawX, weaponAnimation.targetDrawX, lerpSpeed);
	weaponAnimation.drawY = lerp(weaponAnimation.drawY, weaponAnimation.targetDrawY, lerpSpeed);
	if (weaponAnimation.curFrame > 0 && weaponAnimation.curFrameTime > 0) {
		weaponAnimation.curFrameTime -= deltaTime;
		if (weaponAnimation.curFrameTime <= 0) {
			weaponAnimation.curFrameTime = weaponAnimation.frameTime;
			weaponAnimation.curFrame++;
			if (weaponAnimation.curFrame == weaponAnimation.activeFrame && weaponAnimation.spellIndex == BASIC_SPELL_INDEX) {
				castSpell(BASIC_SPELL_INDEX);
			}
			if (weaponAnimation.curFrame >= weaponAnimation.bitmapFrames) {
				weaponAnimation.curFrame = 0;
				weaponAnimation.spellIndex = -1;
			}
		}
	}

	LCDBitmap *weaponBitmap = pd->graphics->getTableBitmap(weaponAnimation.bitmapTable, weaponAnimation.curFrame);
	pd->graphics->drawBitmap(weaponBitmap, weaponAnimation.drawX, weaponAnimation.drawY, kBitmapUnflipped);
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