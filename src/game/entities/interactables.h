#pragma once

#include "pd_api.h"
#include "game/player/spells/spells.h"
#include "game/systems/shopManager.h"

#define MAX_INTERACTABLE_COUNT 15

typedef enum InteractableType
{
	INTERACTABLE_TYPE_EXIT,
	INTERACTABLE_TYPE_CAMPFIRE,
	INTERACTABLE_TYPE_EXTINGUISHED_CAMPFIRE,
	INTERACTABLE_TYPE_COUNT
} InteractableType;

typedef struct Interactable {
	float x;
	float y;
	float animationTime;
	int animationFrame;
	int textureIndex;
    InteractableType type;
	bool active;
} Interactable;

extern Interactable INTERACTABLES[MAX_INTERACTABLE_COUNT];

void initInteractables(void);
int getInteractableIndexAtPosition(float x, float y);
void interact(int interactableIndex);
int addInteractable(int mapX, int mapY, InteractableType type);
void updateInteractables(float deltaTime);