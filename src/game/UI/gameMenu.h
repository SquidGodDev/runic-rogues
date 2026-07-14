#pragma once

void initGameMenu(void);
// Opens the menu from the title screen: hides the rune tab and reduces the exit
// tab to a single "Return to Title" option. Poll gameMenuActive() to detect close.
void initSettingsMenu(void);
bool gameMenuActive(void);
void updateGameMenu(float deltaTime);
void createGameMenuItem(void);
void removeGameMenuItem(void);
