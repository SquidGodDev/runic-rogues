#pragma once

#include "stdbool.h"

#include "globals/localization.h"

void openTutorialDialogPages(const StringKey *pages, int pageCount);
#define openTutorialDialog(pages) openTutorialDialogPages((pages), (int)(sizeof(pages) / sizeof((pages)[0])))
void updateTutorialDialog(float deltaTime);
void drawTutorialDialog(void);

bool isTutorialDialogActive(void);
bool isTutorialDialogComplete(void);
