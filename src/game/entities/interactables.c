#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "helpers.h"
#include "game/entities/interactables.h"
#include "game/rendering/textures.h"
#include "game/entities/entities.h"

typedef struct InteractableData {
	uint8_t* texture;
	float frameTime;
    int idleEndFrame;
} InteractableData;

Interactable INTERACTABLES[MAX_INTERACTABLE_COUNT] = { 0 };

InteractableData interactableData[INTERACTABLE_TYPE_COUNT];
void initInteractablesData(void) {
    interactableData[INTERACTABLE_TYPE_EXIT] = (InteractableData){
		.texture = TEXTURES[TEXTURE_PORTAL],
        .frameTime = 0.1f,
		.idleEndFrame = 12
    };
}

void initInteractables(void) {
	initInteractablesData();

	for (int i = 0; i < MAX_INTERACTABLE_COUNT; i++) {
		INTERACTABLES[i].active = 0;
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
			pd->system->logToConsole("Next Level C");
			pd->lua->callFunction("NextLevel", 0, NULL);
			break;
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
        .texture = curInteractableData.texture,
        .type = type,
        .active = 1
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
        addEntityToRenderQueue(curInteractable.x, curInteractable.y, INTERACTABLE, i, curInteractableData.texture, curInteractable.animationFrame);
    }
}