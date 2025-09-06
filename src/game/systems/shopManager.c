#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "helpers.h"
#include "game/systems/shopManager.h"
#include "game/systems/progressionManager.h"
#include "game/systems/runeManager.h"
#include "game/settings/settings.h"
#include "game/rendering/textures.h"
#include "game/entities/interactables.h"
#include "game/player/spells.h"
#include "game/map/map.h"

typedef struct ShopPageData {
    LCDBitmap *shopPageBitmap;
    LCDBitmap *shopDescBitmap;
    int descHeight;
} ShopPageData;

#define SHOP_OPTION_COUNT 3

int shopInteractableIndexes[SHOP_OPTION_COUNT];

#define SHOP_ITEM_SPACING 1

#define SHOP_PAGE_ICON_X 12
#define SHOP_PAGE_ICON_Y 10
#define SHOP_PAGE_TITLE_X 40
#define SHOP_PAGE_TITLE_Y 12
#define SHOP_PAGE_TYPE_X 41
#define SHOP_PAGE_TYPE_Y 30

ShopPageData createShopPage(char *name, char *desc, char* type, LCDBitmap *icon, Language language) {
    LCDFont *fontTitle = fontTitleEN;
    LCDFont *fontSub = fontSubEN;
    LCDBitmapDrawMode drawMode = kDrawModeCopy;
    int titleTracking = -1;
    int tracking = -2;
    int leading = 0;
    if (language == LANGUAGE_JP) {
        fontTitle = fontTitleJP;
        fontSub = fontSubJP;
        drawMode = kDrawModeFillWhite;
        tracking = 1;
        leading = 2;
    }

    LCDBitmap *shopPageBitmap = pd->graphics->copyBitmap(BITMAPS[BITMAP_SHOP_POPUP_BACKGROUND]);
    pd->graphics->pushContext(shopPageBitmap);
        pd->graphics->drawScaledBitmap(icon, SHOP_PAGE_ICON_X, SHOP_PAGE_ICON_Y, 2.0f, 2.0f);
        pd->graphics->setDrawMode(drawMode);
        pd->graphics->setFont(fontTitle);
        pd->graphics->setTextTracking(titleTracking);
        pd->graphics->drawText(name, strlen(name), kUTF8Encoding, SHOP_PAGE_TITLE_X, SHOP_PAGE_TITLE_Y);
        pd->graphics->setFont(fontSub);
        pd->graphics->setTextTracking(tracking);
        pd->graphics->setTextLeading(leading);
        pd->graphics->drawText(type, strlen(type), kUTF8Encoding, SHOP_PAGE_TYPE_X, SHOP_PAGE_TYPE_Y);
    pd->graphics->popContext();

    int descHeight = pd->graphics->getTextHeightForMaxWidth(fontSub, desc, strlen(desc), SHOP_DESC_MAX_WIDTH, kUTF8Encoding, kWrapWord, tracking, leading);
    LCDBitmap *shopDescBitmap = pd->graphics->newBitmap(SHOP_DESC_MAX_WIDTH, descHeight, kColorClear);
    pd->graphics->pushContext(shopDescBitmap);
        pd->graphics->setDrawMode(drawMode);
        pd->graphics->setFont(fontSub);
        pd->graphics->setTextTracking(tracking);
        pd->graphics->setTextLeading(leading);
        pd->graphics->drawTextInRect(desc, strlen(desc), kUTF8Encoding, 0, 0, SHOP_DESC_MAX_WIDTH, descHeight, kWrapWord, kAlignTextLeft);
    pd->graphics->popContext();

    return (ShopPageData){
        .shopPageBitmap = shopPageBitmap,
        .shopDescBitmap = shopDescBitmap,
        .descHeight = descHeight
    };
}

ShopPageData createSpellShopPage(Spell spell, Language language) {
    SpellData curSpellData = spellData[spell];
    char *name = curSpellData.name;
    char *desc = curSpellData.description;
    char *type = "Spell";
    if (language == LANGUAGE_JP) {
        name = curSpellData.name_jp;
        desc = curSpellData.description_jp;
        type = "スペル";
    }
    return createShopPage(name, desc, type, BITMAPS[curSpellData.spellIconIndex], language);
}

ShopPageData createRuneShopPage(RuneType runeType, Language language) {
    char *name = getRuneName(runeType, language);
    char *desc = getRuneDescription(runeType, language);
    char *type = language == LANGUAGE_JP ? "ルーン" : "Rune";
    return createShopPage(name, desc, type, BITMAPS[getRuneIconIndex(runeType)], language);
}

void createShopInteractables(void) {
    Stack optionsStack;
    stackInit(&optionsStack);
    bool isSpellShop = getCurrentRoom() % 2 == 1;
    if (isSpellShop) {
        for (int i = SPELL_NONE + 1; i < SPELL_COUNT; i++) {
            if (spellInDeck((Spell)i)) {
                continue;
            }
            stackPush(&optionsStack, i);
        }
    } else {
        for (int i = RUNE_TYPE_NONE + 1; i < RUNE_TYPE_COUNT; i++) {
            if (runeIsOwned((RuneType)i)) {
                continue;
            }
            stackPush(&optionsStack, i);
        }
    }
    stackShuffle(&optionsStack);

    Point shopMapPosition = getShopMapPosition();
    for (int i = 0; i < SHOP_OPTION_COUNT; i++) {
        ShopPageData shopPageDataEN;
        ShopPageData shopPageDataJP;
        Spell shopSpell = SPELL_NONE;
        RuneType shopRune = RUNE_TYPE_NONE;
        int textureIndex = 0;
        
        if (isSpellShop) {
            shopSpell = SPELL_FIREBALL; // Default spell if no options available
            if (!stackIsEmpty(&optionsStack)) {
                shopSpell = (Spell)stackPop(&optionsStack);
            }
            shopPageDataEN = createSpellShopPage(shopSpell, LANGUAGE_EN);
            shopPageDataJP = createSpellShopPage(shopSpell, LANGUAGE_JP);
            textureIndex = getSpellPickupTextureIndex(shopSpell);
        } else {
            shopRune = RUNE_TYPE_VAMPIRISM; // Default rune if no options available
            if (!stackIsEmpty(&optionsStack)) {
                shopRune = (RuneType)stackPop(&optionsStack);
            }
            shopPageDataEN = createRuneShopPage(shopRune, LANGUAGE_EN);
            shopPageDataJP = createRuneShopPage(shopRune, LANGUAGE_JP);
            textureIndex = getRuneTextureIndex(shopRune);
        }
        
        int mapX = shopMapPosition.x - SHOP_ITEM_SPACING + (i * SHOP_ITEM_SPACING);
        int mapY = shopMapPosition.y;
        ShopItem shopItem = {
            .shopPageBitmapEN = shopPageDataEN.shopPageBitmap,
            .shopDescBitmapEN = shopPageDataEN.shopDescBitmap,
            .descHeightEN = shopPageDataEN.descHeight,
            .shopPageBitmapJP = shopPageDataJP.shopPageBitmap,
            .shopDescBitmapJP = shopPageDataJP.shopDescBitmap,
            .descHeightJP = shopPageDataJP.descHeight,
            .textureIndex = textureIndex,
            .spell = shopSpell,
            .runeType = shopRune,
            .price = 20
        };
        int interactableIndex = addShopInteractable(mapX, mapY, shopItem);
        shopInteractableIndexes[i] = interactableIndex;
    }
}

void initShop(void) {
    createShopInteractables();
    carveOutShopArea();
    Point exitMapPosition = getExitMapPosition();
    setExitArrowPosition(exitMapPosition.x * CELL_SIZE + CELL_SIZE / 2, exitMapPosition.y * CELL_SIZE + CELL_SIZE / 2);
}

void selectShopItem(ShopItem shopItem) {
    for (int i = 0; i < SHOP_OPTION_COUNT; i++) {
        if (shopInteractableIndexes[i] < 0) {
            continue;
        }
        Interactable *interactable = &INTERACTABLES[shopInteractableIndexes[i]];
        pd->graphics->freeBitmap(interactable->shopItem.shopPageBitmapEN);
        pd->graphics->freeBitmap(interactable->shopItem.shopDescBitmapEN);
        pd->graphics->freeBitmap(interactable->shopItem.shopPageBitmapJP);
        pd->graphics->freeBitmap(interactable->shopItem.shopDescBitmapJP);
        interactable->active = false;
    }

    if (shopItem.spell != SPELL_NONE) {
        addSpellToDeck(shopItem.spell);
    } else if (shopItem.runeType != RUNE_TYPE_NONE) {
        addRune(shopItem.runeType);
    }

    spawnExit();
}