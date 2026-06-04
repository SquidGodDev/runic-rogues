#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/helpers.h"
#include "globals/sceneManager.h"

#include "game/player/player.h"
#include "game/entities/interactables.h"
#include "game/rendering/textures.h"
#include "game/entities/entities.h"
#include "game/systems/progressionManager.h"
#include "game/systems/shopManager.h"
#include "game/systems/runStats.h"

#define CAMPFIRE_HEAL_AMOUNT 2
#define INTERACT_DISTANCE 500

typedef struct InteractableData {
	int textureIndex;
	float frameTime;
    int idleEndFrame;
} InteractableData;

Interactable INTERACTABLES[MAX_INTERACTABLE_COUNT] = { 0 };

InteractableData interactableData[INTERACTABLE_TYPE_COUNT] = {
	[INTERACTABLE_TYPE_EXIT] = {
		.textureIndex = TEXTURE_PORTAL,
		.frameTime = 0.1f,
		.idleEndFrame = 12
	},
	[INTERACTABLE_TYPE_CAMPFIRE] = {
		.textureIndex = TEXTURE_CAMPFIRE,
		.frameTime = 0.15f,
		.idleEndFrame = 5
	},
	[INTERACTABLE_TYPE_EXTINGUISHED_CAMPFIRE] = {
		.textureIndex = TEXTURE_EXTINGUISHED_CAMPFIRE,
		.frameTime = 0.1f,
		.idleEndFrame = 1
	}
};

void initInteractables(void) {
	for (int i = 0; i < MAX_INTERACTABLE_COUNT; i++) {
		INTERACTABLES[i].active = false;
	}
}

int getInteractableIndexAtPosition(float x, float y) {
	for (int i = 0; i < MAX_INTERACTABLE_COUNT; i++) {
		if (!INTERACTABLES[i].active) {
			continue;
		}

		Interactable curInteractable = INTERACTABLES[i];
		if (distanceSquared(x, y, curInteractable.x, curInteractable.y) <= INTERACT_DISTANCE) {
			return i;
		}
	}
	return -1;
}

void interact(int interactableIndex) {
	if (interactableIndex < 0 || interactableIndex >= MAX_INTERACTABLE_COUNT) {
		return;
	}

	Interactable curInteractable = INTERACTABLES[interactableIndex];
	if (!curInteractable.active) {
		return;
	}

	switch (curInteractable.type) {
		case INTERACTABLE_TYPE_EXIT:
			if (getCurrentArea() == AREA_ECHOING_CAVERNS && isAreaCompleted()) {
				runStatsCaptureFinal(true);
				switchScene(SCENE_GAME_OVER);
			} else {
				incrementToNextRoom();
				switchScene(SCENE_GAME);
			}
			break;
		case INTERACTABLE_TYPE_CAMPFIRE:
			healPlayer(CAMPFIRE_HEAL_AMOUNT);
			INTERACTABLES[interactableIndex].active = false;
			int campfireMapX = (INTERACTABLES[interactableIndex].x - CELL_SIZE / 2) / CELL_SIZE;
			int campfireMapY = (INTERACTABLES[interactableIndex].y - CELL_SIZE / 2) / CELL_SIZE;
			addInteractable(campfireMapX, campfireMapY, INTERACTABLE_TYPE_EXTINGUISHED_CAMPFIRE);
		default:
			break;
	}
}

int addInteractable(int mapX, int mapY, InteractableType type) {
    int newInteractableIndex = -1;
	for (int i = 0; i < MAX_INTERACTABLE_COUNT; i++) {
		if (!INTERACTABLES[i].active) {
			newInteractableIndex = i;
			break;
		}
	}

	if (newInteractableIndex == -1) {
		return -1;
	}

	InteractableData curInteractableData = interactableData[type];
	int textureIndex = curInteractableData.textureIndex;

	INTERACTABLES[newInteractableIndex] = (struct Interactable) {
        .x = mapX * CELL_SIZE + CELL_SIZE / 2,
        .y = mapY * CELL_SIZE + CELL_SIZE / 2,
        .animationFrame = randInt(0, curInteractableData.idleEndFrame - 1),
        .animationTime = randFloat(0, curInteractableData.frameTime),
        .textureIndex = textureIndex,
        .type = type,
        .active = true
    };

	return newInteractableIndex;
}

void updateInteractables(float deltaTime) {
    for (int i = 0; i < MAX_INTERACTABLE_COUNT; i++) {
		if (!INTERACTABLES[i].active) {
			continue;
		}

		Interactable curInteractable = INTERACTABLES[i];
		InteractableData curInteractableData = interactableData[curInteractable.type];

        curInteractable.animationTime -= deltaTime;
		if (curInteractable.animationTime <= 0) {
			curInteractable.animationTime = curInteractableData.frameTime;
			curInteractable.animationFrame += 1;
            if (curInteractable.animationFrame >= curInteractableData.idleEndFrame) {
                curInteractable.animationFrame = 0;
            }
        }

        INTERACTABLES[i] = curInteractable;
        addEntityToRenderQueue(curInteractable.x, curInteractable.y, INTERACTABLE, i, curInteractable.textureIndex, curInteractable.animationFrame);
    }
}