#pragma once

#include "pd_api.h"
#include "game/player/spells/spells.h"
#include "game/systems/runeManager.h"

#define SHOP_DESC_MAX_WIDTH 145
#define SHOP_DESC_MAX_HEIGHT 50

typedef struct ShopItem {
    LCDBitmap *shopDescBitmapEN;
    int descHeightEN;
    LCDBitmap *shopDescBitmapJP;
    int descHeightJP;
    int iconIndex;
    RuneType runeType;
    int price;
} ShopItem;

void initShop(void);
void updateShop(float deltaTime);
void selectShopItem(ShopItem shopItem);