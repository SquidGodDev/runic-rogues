#pragma once

#include "stdbool.h"

// Open a dialog sequence. Strings are NOT copied; pass string literals or otherwise
// guarantee the page array and its strings outlive the dialog.
void openTutorialDialog(const char *const *pages, int pageCount);
void updateTutorialDialog(float deltaTime);
void drawTutorialDialog(void);

bool isTutorialDialogActive(void);
bool isTutorialDialogComplete(void);
