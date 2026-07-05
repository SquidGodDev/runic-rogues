#pragma once

#include <stdbool.h>

typedef enum RuneInventoryFocus {
    RUNE_INV_FOCUS_NONE,     // draw only, no input (tab selection state)
    RUNE_INV_FOCUS_RUNES,    // cursor shown, left/right switches runes, crank scrolls
    RUNE_INV_FOCUS_SCROLLBAR // scrollbar solid, up/down scrolls
} RuneInventoryFocus;

void initRuneInventoryTab(void);
void updateRuneInventoryTab(float deltaTime, RuneInventoryFocus focus);
bool runeInventoryHasScrollbar(void);
void freeRuneInventoryCache(void);
