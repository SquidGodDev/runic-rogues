#pragma once

#include "pd_api.h"
#include "game/player/spells.h"

typedef struct ShopItem {
    LCDBitmap *shopPageBitmapEN;
    LCDBitmap *shopPageBitmapJP;
    Spell spell;
    int price;
} ShopItem;

void createShopInteractables(void);
void selectShopItem(ShopItem shopItem);