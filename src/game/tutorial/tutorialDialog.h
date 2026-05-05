#pragma once

#include "stdbool.h"

void openTutorialDialogPages(const char *const *pages, int pageCount);
#define openTutorialDialog(pages) openTutorialDialogPages((pages), (int)(sizeof(pages) / sizeof((pages)[0])))
void updateTutorialDialog(float deltaTime);
void drawTutorialDialog(void);

bool isTutorialDialogActive(void);
bool isTutorialDialogComplete(void);
