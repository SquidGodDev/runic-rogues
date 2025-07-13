#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "helpers.h"
#include "game/player/player.h"
#include "game/entities/interactables.h"
#include "game/rendering/textures.h"
#include "game/entities/entities.h"
#include "game/systems/progressionManager.h"

#define CAMPFIRE_HEAL_AMOUNT 2

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
			nextRoom();
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

void addInteractable(int mapX, int mapY, InteractableType type) {
    int newInteractableIndex = -1;
	for (int i = 0; i < MAX_INTERACTABLE_COUNT; i++) {
		if (!INTERACTABLES[i].active) {
			newInteractableIndex = i;
			break;
		}
	}

	if (newInteractableIndex == -1) {
		return;
	}

	InteractableData curInteractableData = interactableData[type];

	INTERACTABLES[newInteractableIndex] = (struct Interactable) {
        .x = mapX * CELL_SIZE + CELL_SIZE / 2,
        .y = mapY * CELL_SIZE + CELL_SIZE / 2,
        .animationFrame = 0,
        .animationTime = randFloat(0, curInteractableData.frameTime),
        .textureIndex = curInteractableData.textureIndex,
        .type = type,
        .active = true
    };
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
        addEntityToRenderQueue(curInteractable.x, curInteractable.y, INTERACTABLE, i, curInteractableData.textureIndex, curInteractable.animationFrame);
    }
}