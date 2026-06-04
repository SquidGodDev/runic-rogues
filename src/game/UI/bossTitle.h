#pragma once

#include "pd_api.h"

// Boss name titles drawn above the boss health bar. The bitmaps are generated
// from the localized boss name strings (using the sub fonts) during texture
// loading, cached per language so runtime language switching is instant, and no
// name art ships as an asset.
typedef enum BossTitle {
    BOSS_TITLE_ELDER,
    BOSS_TITLE_WRAITH,
    BOSS_TITLE_DRAGON,
    BOSS_TITLE_COUNT,
} BossTitle;

// Number of generation steps fed into the texture loading bar (one per title,
// each building both languages).
int getBossTitleAssetCount(void);

// Generates the boss title bitmaps for generation step `step`. Fonts must be
// loaded first.
void buildBossTitle(int step);

// Returns the cached boss title bitmap for the active language.
LCDBitmap *getBossTitleBitmap(BossTitle title);
