#pragma once

#include <stdbool.h>

typedef enum RuneInventoryFocus {
    RUNE_INV_FOCUS_NONE,     // draw only, no input (tab selection state)
    RUNE_INV_FOCUS_RUNES,    // cursor shown, left/right switches runes, crank scrolls
    RUNE_INV_FOCUS_SCROLLBAR // scrollbar solid, up/down scrolls
} RuneInventoryFocus;

void initRuneInventoryTab(void);
void updateRuneInventoryTab(float deltaTime, RuneInventoryFocus focus);
// Debug only: cycles the selected rune's level (unowned -> I -> ... -> max -> unowned).
void runeInventoryDebugCycleSelected(void);
bool runeInventoryHasScrollbar(void);
bool runeInventoryAtScrollTop(void);
// True once per left/right rune switch, so the menu can drop scrollbar focus.
bool runeInventoryConsumeNavigated(void);
void notifyEnterRuneScrollbar(void);
void freeRuneInventoryCache(void);
