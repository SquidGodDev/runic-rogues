#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "helpers.h"
#include "game/player/player.h"
#include "game/UI/UI.h"
#include "game/entities/projectiles.h"
#include "game/entities/interactables.h"
#include "game/rendering/textures.h"
#include "game/map/map.h"

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
int playerMaxHealth;
int playerHealth;

int interactableIndex;
// ========================

static float playerHitInvincibility;

static int playerRadius;
static float playerTurnSpeed;
static float playerTurnAccel;
static float playerTurnDrag;
static float playerTurnMaxSpeed;
static float playerTurnInitialSpeed;

static float playerXVelocity, playerYVelocity;
static float playerSpeed;
static float playerInitialSpeed;
static float playerAccel;
static float playerDrag;
static float playerMaxSpeed;
static float playerStrafeSpeed;

static int projectileDamage = 1;
static float projectileSpeed = 4.0f * TARGET_FPS;
static float weaponCooldown = 0.3f; // Seconds
static float weaponCurCooldown = 0;
static float weaponBobCounter = 0;
static float weaponBobSpeed = 0.15f;
static int weaponBasePositionX = 126;
static int weaponBasePositionY = 75;
static int weaponXOffset = 5;
static int weaponYOffset = 3;
static int weaponActiveXOffset = 0;
static int weaponActiveYOffset = -7;
static float weaponTargetDrawX;
static float weaponTargetDrawY;
static float weaponDrawX;
static float weaponDrawY;

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

void initPlayer(void) {
    // Player
	playerMaxHealth = 8;
    playerHealth = playerMaxHealth;
	playerHitInvincibility = 0;
	playerX = PLAYER_SPAWN_MAP_X * CELL_SIZE + CELL_SIZE / 2;
	playerY = PLAYER_SPAWN_MAP_Y * CELL_SIZE + CELL_SIZE / 2;
	playerRadius = 6;
	playerAngle = 0;
	playerTurnSpeed = 0;
	playerTurnMaxSpeed = 0.04f * TARGET_FPS;
	playerTurnInitialSpeed = 0.01f * TARGET_FPS;
	playerTurnAccel = 0.1f * TARGET_FPS;
	playerTurnDrag = 0.25f * TARGET_FPS;
	playerMaxSpeed = 0.8f * TARGET_FPS;
	playerInitialSpeed = 0.4f * TARGET_FPS;
	playerSpeed = 0;
	playerStrafeSpeed = 0;
	playerAccel = 3.5f * TARGET_FPS;
	playerDrag = 3.5f * TARGET_FPS;
	playerXVelocity = 0;
	playerYVelocity = 0;
	planeX = 0;
	planeY = -0.59f; // I think it's supposed to be -tanf(FOV_RAD / 2) ~= -0.65, but adjusted since it was off for some reason
	dirX = 1;
	dirY = 0;

	// Animation
	weaponTargetDrawX = weaponBasePositionX;
	weaponTargetDrawY = weaponBasePositionY;
	weaponDrawX = weaponTargetDrawX;
	weaponDrawY = weaponTargetDrawY;

	// State
	interactableIndex = -1;

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

	if (pushed & kButtonA) {
		if (interactableIndex != -1) {
			interact(interactableIndex);
		} else if (weaponCurCooldown <= 0) {
			weaponCurCooldown = weaponCooldown;
			float projectileX = playerX + cosf(playerAngle + FIRE_ANGLE_OFFSET) * FIRE_POSITION_OFFSET;
			float projectileY = playerY + sinf(playerAngle + FIRE_ANGLE_OFFSET) * FIRE_POSITION_OFFSET;
			float deltaX = cosf(playerAngle);
			float deltaY = sinf(playerAngle);	
			fireProjectile(projectileX, projectileY, deltaX, deltaY, projectileSpeed, projectileDamage, TEXTURES[TEXTURE_PROJECTILE], 0 /*isEnemy*/);
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
	float crankChange = pd->system->getCrankChange();

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

	float adjustedTurnSpeed = (playerTurnSpeed + crankChange * CRANK_SENSITIVITY) * deltaTime;
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
		weaponTargetDrawX = weaponBasePositionX + weaponActiveXOffset;
		weaponTargetDrawY = weaponBasePositionY + weaponActiveYOffset;
	}
}

void handleInteractables(void) {
	interactableIndex = -1;
	for (int i = 0; i < MAX_INTERACTABLE_COUNT; i++) {
		if (!INTERACTABLES[i].active) {
			continue;
		}

		Interactable curInteractable = INTERACTABLES[i];
		if (distanceSquared(playerX, playerY, curInteractable.x, curInteractable.y) <= INTERACT_DISTANCE) {
			interactableIndex = i;
			break;
		}
	}
}

void updatePlayer(float deltaTime) {
	if (playerHitInvincibility > 0) {
        playerHitInvincibility -= deltaTime;
        if (playerHitInvincibility < 0) {
            playerHitInvincibility = 0;
        }
    }
	handleInput(deltaTime);
	handleInteractables();
}

void damagePlayer(int amount) {
    if (playerHitInvincibility > 0) {
        return;
    }

    screenShake(SCREEN_SHAKE_MEDIUM);
    playerHitInvincibility = HIT_INVINCIBILITY_TIME;
    playerHealth -= amount;
    if (playerHealth <= 0) {
        playerHealth = 0;
        pd->graphics->clear(kColorBlack);
        pd->lua->callFunction("GameOver", 0, NULL);
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
		pd->graphics->drawBitmap(BITMAPS[BITMAP_HAND_OPEN], weaponDrawX, weaponDrawY, kBitmapUnflipped);
	} else {
		pd->graphics->drawBitmap(BITMAPS[BITMAP_HAND], weaponDrawX, weaponDrawY, kBitmapUnflipped);
	}
}