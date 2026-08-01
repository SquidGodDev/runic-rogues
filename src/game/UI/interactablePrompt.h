#pragma once

#include "pd_api.h"

// Interactable prompt labels (e.g. "Next Floor") drawn above interactables. The
// bitmaps are generated from the localized prompt strings (using the sub fonts,
// inverted to white text with a black outline) during texture loading, cached
// per language so runtime language switching is instant, and no prompt art
// ships as an asset.
typedef enum InteractablePrompt {
    INTERACTABLE_PROMPT_NEXT_FLOOR,
    INTERACTABLE_PROMPT_COUNT,
} InteractablePrompt;

// Number of generation steps fed into the texture loading bar (one per prompt,
// each building both languages).
int getInteractablePromptAssetCount(void);

// Generates the interactable prompt bitmaps for generation step `step`. Fonts
// must be loaded first.
void buildInteractablePrompt(int step);

// Returns the cached interactable prompt bitmap for the active language.
LCDBitmap *getInteractablePromptBitmap(InteractablePrompt prompt);
