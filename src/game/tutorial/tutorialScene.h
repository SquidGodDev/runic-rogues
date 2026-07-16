#pragma once

#include "globals/localization.h"

void initTutorialScene(void);
void updateTutorialScene(float deltaTime);
bool isTutorialActive(void);
void setTutorialActive(bool active);
void removeSkipTutorialMenuItem(void);
void refreshSkipTutorialMenuItemTitle(void);
// Returns the current tutorial objective for the HUD. Writes the format string
// key and the current/target counts, or returns false when no objective applies.
bool getTutorialObjectiveText(StringKey *outKey, int *outCurrent, int *outTarget);
