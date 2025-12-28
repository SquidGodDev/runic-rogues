#pragma once

#include "pd_api.h"
#include "game/player/spells.h"
#include "game/systems/runeManager.h"

#define SHOP_DESC_MAX_WIDTH 150
#define SHOP_DESC_MAX_HEIGHT 50

typedef struct ShopItem {
    LCDBitmap *shopPageBitmapEN;
    LCDBitmap *shopDescBitmapEN;
    int descHeightEN;
    LCDBitmap *shopPageBitmapJP;
    LCDBitmap *shopDescBitmapJP;
    int descHeightJP;
    int textureIndex;
    Spell spell;
    RuneType runeType;
    int price;
} ShopItem;

void initShop(void);
void selectShopItem(ShopItem shopItem);