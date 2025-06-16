#pragma once

#include "pd_api.h"

#define MAX_INTERACTABLE_COUNT 15
#define INTERACT_DISTANCE 500

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
	uint16_t animationFrame;
	uint8_t *texture;
    InteractableType type;
	int active;
} Interactable;

extern Interactable INTERACTABLES[MAX_INTERACTABLE_COUNT];

void initInteractables(void);
void interact(int interactableIndex);
void addInteractable(int mapX, int mapY, InteractableType type);
void updateInteractables(float deltaTime);