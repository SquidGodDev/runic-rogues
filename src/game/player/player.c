#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "helpers.h"
#include "game/player/player.h"
#include "game/player/spells.h"
#include "game/UI/UI.h"
#include "game/entities/projectiles.h"
#include "game/entities/interactables.h"
#include "game/rendering/textures.h"
#include "game/map/map.h"
#include "game/systems/audioManager.h"
#include "game/systems/progressionManager.h"
#include "game/systems/upgradeManager.h"

#define CRANK_SENSITIVITY 0.3f
// Spawn the projectile slightly to the right to match hand
#define FIRE_ANGLE_OFFSET PI / 8
#define FIRE_POSITION_OFFSET CELL_SIZE / 2

#define HIT_INVINCIBILITY_TIME 0.5f // Seconds

// === Extern variables ===
float playerX, playerY;
float playerAngle;
float planeX, planeY;
float dirX, dirY;
int playerHealth;
float playerMana;
// ========================

typedef struct PlayerState {
	bool dead;
	float hitInvincibility;
	int interactableIndex;
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
	// Dynamic values
	float curCooldown;
	float bobCounter;
	float drawX;
	float drawY;
	float targetDrawX;
	float targetDrawY;
} WeaponAnimation;

static PlayerState playerState;
static PlayerMovementStats movementStats;
static WeaponAnimation weaponAnimation;

static int defaultStrafeSetting = 0;
static int toggleMovementSetting = 0;
static int movementToggled = 0;

PDMenuItem* toggleDefaultStrafeMenuItem;
PDMenuItem* toggleMovementMenuItem;

void toggleDefaultStrafeCallback(void* userdata) {
	int value = pd->system->getMenuItemValue(toggleDefaultStrafeMenuItem);
	defaultStrafeSetting = value;
}

void toggleMovementCallback(void* userdata) {
	int value = pd->system->getMenuItemValue(toggleMovementMenuItem);
	toggleMovementSetting = value;
	movementToggled = 0;
}

void initPlayerRunStartStats(void) {
	playerHealth = PLAYER_STATS.maxHealth;
}

void initPlayer(void) {
	playerMana = PLAYER_STATS.maxMana;
	playerState = (PlayerState) {
		.dead = 0,
		.hitInvincibility = 0,
		.interactableIndex = -1
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
		.acceleration = 175.0f,
		.drag = 175.0f,
		.xVelocity = 0,
		.yVelocity = 0,
		.speed = 0,
		.strafeSpeed = 0,

		.dashSpeed = 125.0f,
		.dashTime = 0.3f,
		.dashMovementDir = 0,
		.curDashTime = 0
	};

	// Animation
	int weaponBasePositionX = 126;
	int weaponBasePositionY = 75;
	weaponAnimation = (WeaponAnimation) {
		.bobSpeed = 0.15f,
		.basePositionX = weaponBasePositionX,
		.basePositionY = weaponBasePositionY,
		.xOffset = 5,
		.yOffset = 3,
		.activeXOffset = 0,
		.activeYOffset = -7,
		.curCooldown = 0,
		.bobCounter = 0,
		.drawX = weaponBasePositionX,
		.drawY = weaponBasePositionY,
		.targetDrawX = weaponBasePositionX,
		.targetDrawY = weaponBasePositionY
	};

	pd->system->removeAllMenuItems();
	const char *movementOptions[] = {"rotate", "strafe"};
	toggleDefaultStrafeMenuItem = pd->system->addOptionsMenuItem("Default", movementOptions, 2, toggleDefaultStrafeCallback, NULL);
	toggleMovementMenuItem = pd->system->addCheckmarkMenuItem("Toggle", 0, toggleMovementCallback, NULL);
}

void handleInput(float deltaTime)
{
	if (!pd->system->isCrankDocked() && !defaultStrafeSetting) {
		pd->system->setMenuItemValue(toggleDefaultStrafeMenuItem, 1);
		toggleDefaultStrafeCallback(NULL);
	}

	int cellSize = CELL_SIZE;
	int mapWidth = MAP_WIDTH;

	PDButtons current;
	PDButtons pushed;
	pd->system->getButtonState(&current, &pushed, NULL);

	if (isTransitioning() || playerState.dead) {
		// Disable inputs
		current = 0;
		pushed = 0;
	}

	movementStats.curDashTime -= deltaTime;
	if (movementStats.curDashTime < 0) {
		movementStats.curDashTime = 0;
	}
	// if (pushed & kButtonB) {
	// 	castUtilitySpell();
	// }

	if (playerState.interactableIndex != -1 && (pushed & kButtonA)) {
		interact(playerState.interactableIndex);
	} else if (pushed & kButtonA) {
		if(castSpell(0)) {
			weaponAnimation.curCooldown = PLAYER_STATS.weaponCooldown;
		}
	}

	// if (pushed & kButtonB) {
	// 	movementToggled ^= 1;
	// }

	if (current & kButtonB) {
		int spellIndex = 0;
		if (current & kButtonLeft) {
			spellIndex = 1;
		} else if (current & kButtonUp) {
			spellIndex = 2;
		} else if (current & kButtonRight) {
			spellIndex = 3;
		} else if (current & kButtonDown) {
			spellIndex = 4;
		}
		if (spellIndex > 0) {
			if (castSpell(spellIndex)) {
				weaponAnimation.curCooldown = PLAYER_STATS.weaponCooldown;
			}
		}
		current = 0;
		pushed = 0;
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

	float adjustedTurnSpeed = (movementStats.turnSpeed + crankChange * CRANK_SENSITIVITY) * deltaTime;
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

	float maxSpeed = playerIsDashing ? movementStats.dashSpeed : PLAYER_STATS.maxSpeed;
	maxSpeed *= getPlayerSpeedModifier();
	PDButtons movementDir = playerIsDashing ? movementStats.dashMovementDir : current;

	if ((movementDir & kButtonRight) && strafeEnabled) {
		if (movementStats.strafeSpeed < movementStats.initialSpeed) {
			movementStats.strafeSpeed = movementStats.initialSpeed;
		}
		movementStats.strafeSpeed += movementStats.acceleration * deltaTime;
		if (movementStats.strafeSpeed > maxSpeed) {
			movementStats.strafeSpeed = maxSpeed;
		}
	} else if ((movementDir & kButtonLeft) && strafeEnabled) {
		if (movementStats.strafeSpeed > -movementStats.initialSpeed) {
			movementStats.strafeSpeed = -movementStats.initialSpeed;
		}
		movementStats.strafeSpeed -= movementStats.acceleration * deltaTime;
		if (movementStats.strafeSpeed < -maxSpeed) {
			movementStats.strafeSpeed = -maxSpeed;
		}
	} else {
		float dragCutoff = 0.1f * maxSpeed;
		if (movementStats.strafeSpeed < dragCutoff && movementStats.strafeSpeed > -dragCutoff) {
			movementStats.strafeSpeed = 0;
		} else if (movementStats.strafeSpeed > 0) {
			movementStats.strafeSpeed -= movementStats.drag * deltaTime;
		} else if (movementStats.strafeSpeed < 0) {
			movementStats.strafeSpeed += movementStats.drag * deltaTime;
		}
	}

	if (movementDir & kButtonUp) {
		if (movementStats.speed < movementStats.initialSpeed) {
			movementStats.speed = movementStats.initialSpeed;
		}
		movementStats.speed += movementStats.acceleration * deltaTime;
		if (movementStats.speed > maxSpeed) {
			movementStats.speed = maxSpeed;
		}
	} else if (movementDir & kButtonDown) {
		if (movementStats.speed > -movementStats.initialSpeed) {
			movementStats.speed = -movementStats.initialSpeed;
		}
		movementStats.speed -= movementStats.acceleration * deltaTime;
		if (movementStats.speed < -maxSpeed) {
			movementStats.speed = -maxSpeed;
		}
	} else {
		float dragCutoff = 0.1f * maxSpeed;
		if (movementStats.speed < dragCutoff && movementStats.speed > -dragCutoff) {
			movementStats.speed = 0;
		} else if (movementStats.speed > 0) {
			movementStats.speed -= movementStats.drag * deltaTime;
		} else if (movementStats.speed < 0) {
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

	movementStats.xVelocity = cosf(playerAngle) * normalizedSpeed + cosf(playerAngle + PI / 2) * normalizedStrafeSpeed;
	movementStats.yVelocity = sinf(playerAngle) * normalizedSpeed + sinf(playerAngle + PI / 2) * normalizedStrafeSpeed;

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

	if (weaponAnimation.curCooldown > 0) {
		weaponAnimation.targetDrawX = weaponAnimation.basePositionX + weaponAnimation.activeXOffset;
		weaponAnimation.targetDrawY = weaponAnimation.basePositionY + weaponAnimation.activeYOffset;
	}
}

void handleInteractables(void) {
	playerState.interactableIndex = -1;
	for (int i = 0; i < MAX_INTERACTABLE_COUNT; i++) {
		if (!INTERACTABLES[i].active) {
			continue;
		}

		Interactable curInteractable = INTERACTABLES[i];
		if (distanceSquared(playerX, playerY, curInteractable.x, curInteractable.y) <= INTERACT_DISTANCE) {
			playerState.interactableIndex = i;
			break;
		}
	}
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
}

int getPlayerInteractableIndex(void) {
	return playerState.interactableIndex;
}

void playerDash(void) {
	pd->system->getButtonState(&movementStats.dashMovementDir, NULL, NULL);
	movementStats.curDashTime = movementStats.dashTime;
	playerState.hitInvincibility = movementStats.dashTime * 2;
}

void healPlayer(int amount) {
	playerHealth += amount;
	if (playerHealth >= PLAYER_STATS.maxHealth) {
		playerHealth = PLAYER_STATS.maxHealth;
	}
}

void damagePlayer(int amount) {
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
        pd->lua->callFunction("GameOver", 0, NULL);
    }
}

void drawWeapon(float deltaTime) {
	float lerpSpeed = 0.2f;
	weaponAnimation.drawX = lerp(weaponAnimation.drawX, weaponAnimation.targetDrawX, lerpSpeed);
	weaponAnimation.drawY = lerp(weaponAnimation.drawY, weaponAnimation.targetDrawY, lerpSpeed);
	if (weaponAnimation.curCooldown > 0) {
		weaponAnimation.curCooldown -= deltaTime;
		if (weaponAnimation.curCooldown <= 0) {
			weaponAnimation.curCooldown = 0;
		}
		pd->graphics->drawBitmap(BITMAPS[BITMAP_HAND_OPEN], weaponAnimation.drawX, weaponAnimation.drawY, kBitmapUnflipped);
	} else {
		pd->graphics->drawBitmap(BITMAPS[BITMAP_HAND], weaponAnimation.drawX, weaponAnimation.drawY, kBitmapUnflipped);
	}
}