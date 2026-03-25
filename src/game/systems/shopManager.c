#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/helpers.h"
#include "game/gameScene.h"
#include "game/systems/shopManager.h"
#include "game/systems/progressionManager.h"
#include "game/systems/runeManager.h"
#include "game/settings/settings.h"
#include "game/rendering/textures.h"
#include "game/entities/interactables.h"
#include "game/player/spells/spells.h"
#include "game/map/map.h"

typedef enum ShopState {
    SHOP_STATE_ENTERING,
    SHOP_STATE_ACTIVE,
    SHOP_STATE_EXITING
} ShopState;

typedef struct ShopPageData {
    LCDBitmap *shopDescBitmap;
    int descHeight;
} ShopPageData;

#define SHOP_OPTION_COUNT 3

#define SHOP_PAGE_X 18
#define SHOP_PAGE_Y 50
#define SHOP_DESC_X SHOP_PAGE_X + 5
#define SHOP_DESC_Y SHOP_PAGE_Y + 6
#define SHOP_ICON_BASE_X 15
#define SHOP_ICON_BASE_Y 12
#define SHOP_ICON_SPACING 71

#define SHOP_TITLE_DESC_SPACING 6

#define SHOP_TRANSITION_IN_DURATION 0.5f
#define SHOP_TRANSITION_OUT_DURATION 0.4f

static LCDBitmap *gameScreenshot;
static ShopItem shopItems[SHOP_OPTION_COUNT];
static ShopState shopState;
static float shopTransitionTime;

static int shopDescOffset;
static int maxDescOffset;
static int selectedShopOptionIndex;

static const char *ROMAN_NUMERALS[] = { "", "I", "II", "III", "IV", "V" };

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

    int titleFontHeight = pd->graphics->getFontHeight(fontTitle);
    int descHeight = pd->graphics->getTextHeightForMaxWidth(fontSub, desc, strlen(desc), SHOP_DESC_MAX_WIDTH, kUTF8Encoding, kWrapWord, tracking, leading);
    int totalHeight = titleFontHeight + SHOP_TITLE_DESC_SPACING + descHeight;
    LCDBitmap *shopDescBitmap = pd->graphics->newBitmap(SHOP_DESC_MAX_WIDTH, totalHeight, kColorClear);
    pd->graphics->pushContext(shopDescBitmap);
        // Title
        pd->graphics->setFont(fontTitle);
        pd->graphics->setTextTracking(titleTracking);
        pd->graphics->drawText(name, strlen(name), kUTF8Encoding, 0, 0);
        // Description
        pd->graphics->setDrawMode(drawMode);
        pd->graphics->setFont(fontSub);
        pd->graphics->setTextTracking(tracking);
        pd->graphics->setTextLeading(leading);
        pd->graphics->drawTextInRect(desc, strlen(desc), kUTF8Encoding, 0, titleFontHeight + SHOP_TITLE_DESC_SPACING, SHOP_DESC_MAX_WIDTH, descHeight, kWrapWord, kAlignTextLeft);
    pd->graphics->popContext();

    return (ShopPageData){
        .shopDescBitmap = shopDescBitmap,
        .descHeight = totalHeight
    };
}

ShopPageData createRuneShopPage(RuneType runeType, Language language) {
    char *name = getRuneName(runeType, language);
    char *type = language == LANGUAGE_JP ? "ルーン" : "Rune";
    int nextLevel = getRuneLevel(runeType) + 1;
    char *desc = getRuneDescription(runeType, language, nextLevel);
    char nameWithLevel[64];
    snprintf(nameWithLevel, sizeof(nameWithLevel), "%s %s", name, ROMAN_NUMERALS[nextLevel]);
    return createShopPage(nameWithLevel, desc, type, BITMAPS[getRuneIconIndex(runeType)], language);
}

void createShopOptions(void) {
    Stack optionsStack;
    stackInit(&optionsStack);
    ClassType currentClass = getCurrentClassType();
    for (int i = RUNE_TYPE_NONE + 1; i < RUNE_TYPE_COUNT; i++) {
        if (runeIsMaxLevel((RuneType)i)) {
            continue;
        }
        RuneClassFilter filter = getRuneClassFilter((RuneType)i);
        if (filter != 0 && !(filter & (1 << currentClass))) {
            continue;
        }
        stackPush(&optionsStack, i);
    }
    stackShuffle(&optionsStack);
    
    for (int i = 0; i < SHOP_OPTION_COUNT; i++) {
        ShopPageData shopPageDataEN;
        ShopPageData shopPageDataJP;
        RuneType shopRune = RUNE_TYPE_VAMPIRISM; // Default rune if no options available
        if (!stackIsEmpty(&optionsStack)) {
            shopRune = (RuneType)stackPop(&optionsStack);
        }
        shopPageDataEN = createRuneShopPage(shopRune, LANGUAGE_EN);
        shopPageDataJP = createRuneShopPage(shopRune, LANGUAGE_JP);
        int iconIndex = getRuneIconIndex(shopRune);

        ShopItem shopItem = {
            .shopDescBitmapEN = shopPageDataEN.shopDescBitmap,
            .descHeightEN = shopPageDataEN.descHeight,
            .shopDescBitmapJP = shopPageDataJP.shopDescBitmap,
            .descHeightJP = shopPageDataJP.descHeight,
            .iconIndex = iconIndex,
            .runeType = shopRune
        };
        shopItems[i] = shopItem;
    }
}

bool adjustShopDescOffset(int delta) {
    shopDescOffset += delta;
    if (shopDescOffset < 0) {
        shopDescOffset = 0;
    } else if (shopDescOffset > maxDescOffset) {
        shopDescOffset = maxDescOffset;
    }

    return maxDescOffset > 0;
}

void resetShopDescOffset(void) {
    shopDescOffset = 0;

    ShopItem curShopItem = shopItems[selectedShopOptionIndex];
    int descHeight = SETTINGS.language == LANGUAGE_JP ? curShopItem.descHeightJP : curShopItem.descHeightEN;
    maxDescOffset = descHeight - SHOP_DESC_MAX_HEIGHT > 0 ? descHeight - SHOP_DESC_MAX_HEIGHT : 0;
}

void initShop(void) {
    shopState = SHOP_STATE_ENTERING;
    shopTransitionTime = SHOP_TRANSITION_IN_DURATION;
    if (gameScreenshot != NULL) {
        pd->graphics->freeBitmap(gameScreenshot);
    }
    gameScreenshot = pd->graphics->newBitmap(SCREEN_WIDTH, SCREEN_HEIGHT, kColorClear);
    LCDBitmap *displayBuffer = pd->graphics->getDisplayBufferBitmap();
    pd->graphics->pushContext(gameScreenshot);
        pd->graphics->drawBitmap(displayBuffer, 0, 0, kBitmapUnflipped);
    pd->graphics->popContext();
    createShopOptions();
    selectedShopOptionIndex = 1;
    resetShopDescOffset();
    setGameState(GAME_STATE_SHOP);
}

void selectShopItem(ShopItem shopItem) {
    if (shopItem.runeType != RUNE_TYPE_NONE) {
        addRune(shopItem.runeType);
    }
}

void drawShop(float deltaTime) {
    if (gameScreenshot != NULL) {
        pd->graphics->drawBitmap(gameScreenshot, 0, 0, kBitmapUnflipped);
    }

    if (shopTransitionTime > 0) {
        float transitionProgress;
        float easedTransitionProgress;
        if (shopState == SHOP_STATE_ENTERING) {
            transitionProgress = 1.0f - (shopTransitionTime / SHOP_TRANSITION_IN_DURATION);
            easedTransitionProgress = easeOutCubic(transitionProgress);
        } else {
            transitionProgress = 1.0f - (shopTransitionTime / SHOP_TRANSITION_OUT_DURATION);
            easedTransitionProgress = easeInCubic(transitionProgress);
        }
        int transitionOffset = (int)(easedTransitionProgress * SCREEN_HEIGHT);

        int frameCount;
        LCDBitmapTable *ditherBitmapTable = BITMAP_TABLES[BITMAP_TABLE_SHOP_DITHER_BACKGROUND];
        pd->graphics->getBitmapTableInfo(ditherBitmapTable, NULL, &frameCount);

        if (shopState == SHOP_STATE_ENTERING) {
            pd->graphics->setDrawOffset(0, 0);
            LCDBitmap *ditherBitmap = pd->graphics->getTableBitmap(ditherBitmapTable, (int)(transitionProgress * frameCount));
            pd->graphics->drawBitmap(ditherBitmap, 0, 0, kBitmapUnflipped);
            pd->graphics->setDrawOffset(0, SCREEN_HEIGHT - transitionOffset);
        } else if (shopState == SHOP_STATE_EXITING) {
            pd->graphics->setDrawOffset(0, 0);
            LCDBitmap *ditherBitmap = pd->graphics->getTableBitmap(ditherBitmapTable, frameCount - 1 - (int)(transitionProgress * frameCount));
            pd->graphics->drawBitmap(ditherBitmap, 0, 0, kBitmapUnflipped);
            pd->graphics->setDrawOffset(0, transitionOffset);
        }
    } else if (shopState != SHOP_STATE_EXITING) {
        LCDBitmapTable *ditherBitmapTable = BITMAP_TABLES[BITMAP_TABLE_SHOP_DITHER_BACKGROUND];
        int frameCount;
        pd->graphics->getBitmapTableInfo(ditherBitmapTable, NULL, &frameCount);
        LCDBitmap *ditherBitmap = pd->graphics->getTableBitmap(ditherBitmapTable, frameCount - 1);
        pd->graphics->drawBitmap(ditherBitmap, 0, 0, kBitmapUnflipped);
        pd->graphics->setDrawOffset(0, 0);
    }

    for (int i = 0; i < SHOP_OPTION_COUNT; i++) {
        int optionIconX = SHOP_ICON_BASE_X + i * SHOP_ICON_SPACING;
        int optionIconY = SHOP_ICON_BASE_Y;
        ShopItem shopItem = shopItems[i];
        pd->graphics->drawBitmap(BITMAPS[shopItem.iconIndex], optionIconX, optionIconY, kBitmapUnflipped);
        if (i == selectedShopOptionIndex) {
            pd->graphics->drawBitmap(BITMAPS[BITMAP_SHOP_CURSOR], optionIconX - 4, optionIconY - 4, kBitmapUnflipped);
        }
    }

    ShopItem curShopItem = shopItems[selectedShopOptionIndex];
    LCDBitmap *shopDescBitmap;
    int descHeight;
    pd->graphics->drawBitmap(BITMAPS[BITMAP_SHOP_POPUP_BACKGROUND], SHOP_PAGE_X, SHOP_PAGE_Y, kBitmapUnflipped);
    if (SETTINGS.language == LANGUAGE_JP) {
        shopDescBitmap = curShopItem.shopDescBitmapJP;
        descHeight = curShopItem.descHeightJP;
    } else {
        shopDescBitmap = curShopItem.shopDescBitmapEN;
        descHeight = curShopItem.descHeightEN;
    }
    pd->graphics->setClipRect(SHOP_DESC_X, SHOP_DESC_Y, SHOP_DESC_MAX_WIDTH, SHOP_DESC_MAX_HEIGHT);
    pd->graphics->drawBitmap(shopDescBitmap, SHOP_DESC_X, SHOP_DESC_Y - shopDescOffset, kBitmapUnflipped);
    pd->graphics->clearClipRect();

    // Scrollbar
    if (maxDescOffset > 0) {
        int scrollbarX = 173;
        int scrollbarY = 54;
        int scrollbarWidth = 5;
        int scrollbarHeight = 55;
        pd->graphics->drawRoundRect(scrollbarX, scrollbarY, scrollbarWidth, scrollbarHeight, 1, 1, kColorWhite);
        float scrollbarPercentage = (float)SHOP_DESC_MAX_HEIGHT / descHeight;
        int scrollbarMaxLen = scrollbarHeight - 6;
        int scrollbarLength = scrollbarMaxLen * scrollbarPercentage;
        int scrollbarOffset = scrollbarMaxLen * ((float)shopDescOffset / descHeight);
        int scrollbarLineX = scrollbarX + 2;
        int scrollbarLineY = scrollbarY + 3 + scrollbarOffset;
        pd->graphics->drawLine(scrollbarLineX, scrollbarLineY, scrollbarLineX, scrollbarLineY + scrollbarLength, 1, kColorWhite);
    }

    pd->graphics->setDrawOffset(0, 0);
}

void updateShop(float deltaTime) {
    if (shopState == SHOP_STATE_ENTERING) {
        if (shopTransitionTime > 0) {
            shopTransitionTime -= deltaTime;
            if (shopTransitionTime < 0) {
                shopTransitionTime = 0;
                shopState = SHOP_STATE_ACTIVE;
            }
        }
    } else if (shopState == SHOP_STATE_EXITING) {
        shopTransitionTime -= deltaTime;
        if (shopTransitionTime <= 0) {
            shopTransitionTime = 0;
            setGameState(GAME_STATE_ACTIVE);
            for (int i = 0; i < SHOP_OPTION_COUNT; i++) {
                pd->graphics->freeBitmap(shopItems[i].shopDescBitmapEN);
                pd->graphics->freeBitmap(shopItems[i].shopDescBitmapJP);
            }
            return;
        }
    }

    drawShop(deltaTime);

    if (shopState != SHOP_STATE_ACTIVE) {
        return;
    }

    PDButtons pushed;
    PDButtons current;
	pd->system->getButtonState(&current, &pushed, NULL);
    if (pushed & kButtonLeft) {
        if (selectedShopOptionIndex > 0) {
            selectedShopOptionIndex--;
            resetShopDescOffset();
        }
    } else if (pushed & kButtonRight) {
        if (selectedShopOptionIndex < SHOP_OPTION_COUNT - 1) {
            selectedShopOptionIndex++;
            resetShopDescOffset();
        }
    }

    if (current & kButtonUp) {
        adjustShopDescOffset(-1);
    } else if (current & kButtonDown) {
        adjustShopDescOffset(1);
    }

    float crankChange = pd->system->getCrankChange();
    if (crankChange != 0) {
        adjustShopDescOffset(crankChange);
    }

    if (pushed & kButtonA) {
        selectShopItem(shopItems[selectedShopOptionIndex]);
        shopState = SHOP_STATE_EXITING;
        shopTransitionTime = SHOP_TRANSITION_OUT_DURATION;
    }
}