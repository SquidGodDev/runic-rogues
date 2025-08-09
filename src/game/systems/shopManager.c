#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "helpers.h"
#include "game/systems/shopManager.h"
#include "game/systems/progressionManager.h"
#include "game/settings/settings.h"
#include "game/rendering/textures.h"
#include "game/entities/interactables.h"
#include "game/player/spells.h"

#define SHOP_OPTION_COUNT 3

int shopInteractableIndexes[SHOP_OPTION_COUNT];

#define SHOP_ITEM_SPACING 1

#define DESC_MAX_WIDTH 150
#define DESC_MAX_HEIGHT 50

#define SHOP_PAGE_ICON_X 6
#define SHOP_PAGE_ICON_Y 6
#define SHOP_PAGE_TITLE_X 35
#define SHOP_PAGE_TITLE_Y 8
#define SHOP_PAGE_TYPE_X 36
#define SHOP_PAGE_TYPE_Y 26
#define SHOP_PAGE_DESC_X 8
#define SHOP_PAGE_DESC_Y 45

LCDBitmap *createShopPage(Spell spell, Language language) {
    SpellData curSpellData = spellData[spell];
    char *name = curSpellData.name;
    char *desc = curSpellData.description;
    char *type = "Spell";
    LCDFont *fontTitle = fontTitleEN;
    LCDFont *fontSub = fontSubEN;
    LCDBitmapDrawMode drawMode = kDrawModeCopy;
    int tracking = -2;
    int leading = 0;
    if (language == LANGUAGE_JP) {
        name = curSpellData.name_jp;
        desc = curSpellData.description_jp;
        type = "スペル";
        fontTitle = fontTitleJP;
        fontSub = fontSubJP;
        drawMode = kDrawModeFillWhite;
        tracking = 1;
        leading = 2;
    }

    LCDBitmap *shopPageBitmap = pd->graphics->copyBitmap(BITMAPS[BITMAP_SHOP_POPUP_BACKGROUND]);
    LCDBitmap *spellIcon = BITMAPS[curSpellData.spellIconIndex];
    pd->graphics->pushContext(shopPageBitmap);
        pd->graphics->drawScaledBitmap(spellIcon, SHOP_PAGE_ICON_X, SHOP_PAGE_ICON_Y, 2.0f, 2.0f);
        pd->graphics->setDrawMode(drawMode);
        pd->graphics->setFont(fontTitle);
        pd->graphics->drawText(name, strlen(name), kUTF8Encoding, SHOP_PAGE_TITLE_X, SHOP_PAGE_TITLE_Y);
        pd->graphics->setFont(fontSub);
        pd->graphics->setTextTracking(tracking);
        pd->graphics->setTextLeading(leading);
        pd->graphics->drawText(type, strlen(type), kUTF8Encoding, SHOP_PAGE_TYPE_X, SHOP_PAGE_TYPE_Y);
        pd->graphics->drawTextInRect(desc, strlen(desc), kUTF8Encoding, SHOP_PAGE_DESC_X, SHOP_PAGE_DESC_Y, DESC_MAX_WIDTH, DESC_MAX_HEIGHT, kWrapWord, kAlignTextLeft);
    pd->graphics->popContext();

    return shopPageBitmap;
}

void createShopInteractables(void) {
    Stack spellOptionsStack;
    stackInit(&spellOptionsStack);
    for (int i = 1; i < SPELL_COUNT; i++) {
        if (spellInDeck((Spell)i)) {
            continue;
        }
        stackPush(&spellOptionsStack, i);
    }
    stackShuffle(&spellOptionsStack);

    for (int i = 0; i < SHOP_OPTION_COUNT; i++) {
        Spell spell = SPELL_FIREBALL; // Default spell if no options available
        if (!stackIsEmpty(&spellOptionsStack)) {
            spell = (Spell)stackPop(&spellOptionsStack);
        }

        int mapX = MAP_WIDTH / 2;
        int mapY = MAP_HEIGHT / 2 - SHOP_ITEM_SPACING + (i * SHOP_ITEM_SPACING);

        LCDBitmap *shopPageBitmapEN = createShopPage(spell, LANGUAGE_EN);
        LCDBitmap *shopPageBitmapJP = createShopPage(spell, LANGUAGE_JP);

        ShopItem shopItem = {
            .shopPageBitmapEN = shopPageBitmapEN,
            .shopPageBitmapJP = shopPageBitmapJP,
            .spell = spell,
            .price = 20
        };
        int interactableIndex = addShopInteractable(mapX, mapY, shopItem);
        shopInteractableIndexes[i] = interactableIndex;
    }
}

void selectShopItem(ShopItem shopItem) {
    for (int i = 0; i < SHOP_OPTION_COUNT; i++) {
        if (shopInteractableIndexes[i] < 0) {
            continue;
        }
        Interactable *interactable = &INTERACTABLES[shopInteractableIndexes[i]];
        interactable->active = false;
    }

    addSpellToDeck(shopItem.spell);

    spawnExit();
}