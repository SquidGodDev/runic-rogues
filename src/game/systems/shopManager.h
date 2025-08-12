#pragma once

#include "pd_api.h"
#include "game/player/spells.h"
#include "game/systems/runeManager.h"

typedef struct ShopItem {
    LCDBitmap *shopPageBitmapEN;
    LCDBitmap *shopPageBitmapJP;
    int textureIndex;
    Spell spell;
    RuneType runeType;
    int price;
} ShopItem;

void initShop(void);
void selectShopItem(ShopItem shopItem);