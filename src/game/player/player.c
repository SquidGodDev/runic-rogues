#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "helpers.h"
#include "game/player/player.h"
#include "game/player/projectiles.h"
#include "game/rendering/textures.h"
#include "game/map/map.h"

// Extern variables
float playerX, playerY;
float playerAngle;
float planeX, planeY;
float dirX, dirY;

int playerRadius;
float playerTurnSpeed;
float playerTurnAccel;
float playerTurnDrag;
float playerTurnMaxSpeed;
float playerTurnInitialSpeed;

float playerXVelocity, playerYVelocity;
float playerSpeed;
float playerInitialSpeed;
float playerAccel;
float playerDrag;
float playerMaxSpeed;
float playerStrafeSpeed;

float weaponCooldown = 0.3f; // Seconds
float weaponCurCooldown = 0;
float weaponBobCounter = 0;
float weaponBobSpeed = 0.15f;
int weaponBasePositionX = 126;
int weaponBasePositionY = 75;
int weaponXOffset = 5;
int weaponYOffset = 3;
int weaponActiveXOffset = -4;
int weaponActiveYOffset = -7;
float weaponTargetDrawX = 0;
float weaponTargetDrawY = 0;
float weaponDrawX = 0;
float weaponDrawY = 0;

int defaultStrafeSetting = 0;
int toggleMovementSetting = 0;
int movementToggled = 0;

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

void initPlayer(void) {
    // Player
	playerX = 100;
	playerY = 120;
	playerRadius = 6;
	playerAngle = 0;
	playerTurnSpeed = 0;
	playerTurnMaxSpeed = 0.06f * TARGET_FPS;
	playerTurnInitialSpeed = 0.01f * TARGET_FPS;
	playerTurnAccel = 0.2f * TARGET_FPS;
	playerTurnDrag = 0.4f * TARGET_FPS;
	playerMaxSpeed = 1.4f * TARGET_FPS;
	playerInitialSpeed = 0.7f * TARGET_FPS;
	playerSpeed = 0;
	playerStrafeSpeed = 0;
	playerAccel = 6.0f * TARGET_FPS;
	playerDrag = 6.0f * TARGET_FPS;
	playerXVelocity = 0;
	playerYVelocity = 0;
	planeX = 0;
	planeY = -0.59f; // Related to fov somehow - not sure so I just tweaked it until it looked right
	dirX = 1;
	dirY = 0;

	// Animation
	weaponTargetDrawX = weaponBasePositionX;
	weaponTargetDrawY = weaponBasePositionY;
	weaponDrawX = weaponTargetDrawX;
	weaponDrawY = weaponTargetDrawY;

	const char *movementOptions[] = {"rotate", "strafe"};
	toggleDefaultStrafeMenuItem = pd->system->addOptionsMenuItem("Default", movementOptions, 2, toggleDefaultStrafeCallback, NULL);
	toggleMovementMenuItem = pd->system->addCheckmarkMenuItem("Toggle", 0, toggleMovementCallback, NULL);
}

void handleInput(float deltaTime)
{
	int cellSize = CELL_SIZE;
	int mapWidth = MAP_WIDTH;

	PDButtons current;
	PDButtons pushed;
	pd->system->getButtonState(&current, &pushed, NULL);

	if (pushed & kButtonA) {
		if (weaponCurCooldown <= 0) {
			weaponCurCooldown = weaponCooldown;
			fireProjectile();
		}
	}

	if (pushed & kButtonB) {
		movementToggled ^= 1;
	}

	int bButtonPressed = (current & kButtonB) != 0;
	if (toggleMovementSetting) {
		bButtonPressed = movementToggled;
	}
	int rotationEnabled = (defaultStrafeSetting == bButtonPressed);
	int strafeEnabled = (defaultStrafeSetting != bButtonPressed);

	if ((current & kButtonLeft) && rotationEnabled) {
		if (playerTurnSpeed > -playerTurnInitialSpeed) {
			playerTurnSpeed = -playerTurnInitialSpeed;
		}
		playerTurnSpeed -= playerTurnAccel * deltaTime; 
		if (playerTurnSpeed < -playerTurnMaxSpeed) {
			playerTurnSpeed = -playerTurnMaxSpeed;
		}
	} else if ((current & kButtonRight) && rotationEnabled) {
		if (playerTurnSpeed < playerTurnInitialSpeed) {
			playerTurnSpeed = playerTurnInitialSpeed;
		}
		playerTurnSpeed += playerTurnAccel * deltaTime; 
		if (playerTurnSpeed > playerTurnMaxSpeed) {
			playerTurnSpeed = playerTurnMaxSpeed;
		}
	} else {
		if (playerTurnSpeed < 0.5f && playerTurnSpeed > -0.5f) {
			playerTurnSpeed = 0;
		} else if (playerTurnSpeed > 0) {
			playerTurnSpeed -= playerTurnDrag * deltaTime;
		} else if (playerTurnSpeed < 0) {
			playerTurnSpeed += playerTurnDrag * deltaTime;
		}
	}

	playerAngle += playerTurnSpeed * deltaTime;
	if (playerAngle < 0) {
		playerAngle += 2 * PI;
	} else if (playerAngle > 2 * PI) {
		playerAngle -= 2 * PI;
	}

	float adjustedTurnSpeed = playerTurnSpeed * deltaTime;
	float oldDirX = dirX;
	dirX = dirX * cosf(adjustedTurnSpeed) - dirY * sinf(adjustedTurnSpeed);
	dirY = oldDirX * sinf(adjustedTurnSpeed) + dirY * cosf(adjustedTurnSpeed);
	float oldPlaneX = planeX;
	planeX = planeX * cosf(adjustedTurnSpeed) - planeY * sinf(adjustedTurnSpeed);
	planeY = oldPlaneX * sinf(adjustedTurnSpeed) + planeY * cosf(adjustedTurnSpeed);

	if ((current & kButtonRight) && strafeEnabled) {
		if (playerStrafeSpeed < playerInitialSpeed) {
			playerStrafeSpeed = playerInitialSpeed;
		}
		playerStrafeSpeed += playerAccel * deltaTime;
		if (playerStrafeSpeed > playerMaxSpeed) {
			playerStrafeSpeed = playerMaxSpeed;
		}
	} else if ((current & kButtonLeft) && strafeEnabled) {
		if (playerStrafeSpeed > -playerInitialSpeed) {
			playerStrafeSpeed = -playerInitialSpeed;
		}
		playerStrafeSpeed -= playerAccel * deltaTime;
		if (playerStrafeSpeed < -playerMaxSpeed) {
			playerStrafeSpeed = -playerMaxSpeed;
		}
	} else {
		float dragCutoff = 0.1f * playerMaxSpeed;
		if (playerStrafeSpeed < dragCutoff && playerStrafeSpeed > -dragCutoff) {
			playerStrafeSpeed = 0;
		} else if (playerStrafeSpeed > 0) {
			playerStrafeSpeed -= playerDrag * deltaTime;
		} else if (playerStrafeSpeed < 0) {
			playerStrafeSpeed += playerDrag * deltaTime;
		}
	}

	if (current & kButtonUp) {
		if (playerSpeed < playerInitialSpeed) {
			playerSpeed = playerInitialSpeed;
		}
		playerSpeed += playerAccel * deltaTime;
		if (playerSpeed > playerMaxSpeed) {
			playerSpeed = playerMaxSpeed;
		}
	} else if (current & kButtonDown) {
		if (playerSpeed > -playerInitialSpeed) {
			playerSpeed = -playerInitialSpeed;
		}
		playerSpeed -= playerAccel * deltaTime;
		if (playerSpeed < -playerMaxSpeed) {
			playerSpeed = -playerMaxSpeed;
		}
	} else {
		float dragCutoff = 0.1f * playerMaxSpeed;
		if (playerSpeed < dragCutoff && playerSpeed > -dragCutoff) {
			playerSpeed = 0;
		} else if (playerSpeed > 0) {
			playerSpeed -= playerDrag * deltaTime;
		} else if (playerSpeed < 0) {
			playerSpeed += playerDrag * deltaTime;
		}
	}

	float combinedMagnitude = sqrtf((playerSpeed * playerSpeed) + (playerStrafeSpeed * playerStrafeSpeed));
	float normalizedSpeed = 0;
	float normalizedStrafeSpeed = 0;

	if (combinedMagnitude != 0) {
		normalizedSpeed = playerSpeed * (fabsf(playerSpeed) / combinedMagnitude);
		normalizedStrafeSpeed = playerStrafeSpeed * (fabsf(playerStrafeSpeed) / combinedMagnitude);
	}

	playerXVelocity = cosf(playerAngle) * normalizedSpeed + cosf(playerAngle + PI / 2) * normalizedStrafeSpeed;
	playerYVelocity = sinf(playerAngle) * normalizedSpeed + sinf(playerAngle + PI / 2) * normalizedStrafeSpeed;

	int wallCollisionDist = 10;
	int xOffset = 0;
	if (playerXVelocity < 0) {
		xOffset = -wallCollisionDist;
	} else {
		xOffset = wallCollisionDist;
	}

	int yOffset = 0;
	if (playerYVelocity < 0) {
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
		playerX += playerXVelocity * deltaTime;
	}

	if (map[lookAheadYIndex * mapWidth + xIndex] == 0) {
		playerY += playerYVelocity * deltaTime;
	}

	// Weapon drawing
	if (current & kButtonLeft) {
		weaponTargetDrawX = weaponBasePositionX - weaponXOffset;
	} else if (current & kButtonRight) {
		weaponTargetDrawX = weaponBasePositionX + weaponXOffset;
	} else {
		weaponTargetDrawX = weaponBasePositionX;
	}

	if ((current & kButtonUp) || (current & kButtonDown)) {
		weaponBobCounter += weaponBobSpeed;
		if (weaponBobCounter > 2 * PI) {
			weaponBobCounter -= 2 * PI;
		}

		weaponTargetDrawY = weaponBasePositionY - weaponYOffset * sinf(weaponBobCounter);
	} else {
		weaponBobCounter = 0;
		weaponTargetDrawY = weaponBasePositionY;
	}

	if (weaponCurCooldown > 0) {
		weaponTargetDrawX = weaponBasePositionX;
		weaponTargetDrawY = weaponBasePositionY + weaponActiveYOffset;
	}
}

void drawWeapon(float deltaTime) {
	float lerpSpeed = 0.2f;
	weaponDrawX = lerp(weaponDrawX, weaponTargetDrawX, lerpSpeed);
	weaponDrawY = lerp(weaponDrawY, weaponTargetDrawY, lerpSpeed);
	if (weaponCurCooldown > 0) {
		weaponCurCooldown -= deltaTime;
		if (weaponCurCooldown <= 0) {
			weaponCurCooldown = 0;
		}
		pd->graphics->drawBitmap(handOpenBitmap, weaponDrawX, weaponDrawY, kBitmapUnflipped);
	} else {
		pd->graphics->drawBitmap(handBitmap, weaponDrawX, weaponDrawY, kBitmapUnflipped);
	}
}