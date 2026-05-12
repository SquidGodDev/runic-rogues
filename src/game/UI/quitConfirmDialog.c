#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/sceneManager.h"
#include "game/gameScene.h"
#include "game/UI/quitConfirmDialog.h"
#include "game/UI/runeInventory.h"
#include "game/player/player.h"
#include "game/rendering/textures.h"
#include "game/settings/settings.h"

#define DIALOG_WIDTH 160
#define DIALOG_HEIGHT 88
#define DIALOG_X ((SCREEN_WIDTH - DIALOG_WIDTH) / 2)
#define DIALOG_Y ((SCREEN_HEIGHT - DIALOG_HEIGHT) / 2)
#define DIALOG_CORNER_RADIUS 3
#define DIALOG_BORDER_WIDTH 2

#define HEADER_TOP_PADDING 6
#define BODY_TOP_PADDING 10
#define BODY_SIDE_PADDING 12
#define BUTTON_BOTTOM_PADDING 8
#define BUTTON_GAP 28
#define BUTTON_PADDING_X 5
#define BUTTON_PADDING_Y 3

static LCDBitmap *gameScreenshot = NULL;
static GameState previousGameState;
static int selectedButtonIndex;
static PDMenuItem *quitMenuItem = NULL;
static bool initPending = false;

static const char *getHeaderText(void) {
    return SETTINGS.language == LANGUAGE_JP ? "ランをやめる？" : "Quit Run?";
}

static const char *getBodyText(void) {
    return SETTINGS.language == LANGUAGE_JP
        ? "本当にやめますか？進行状況はすべて失われます。"
        : "Are you sure? All progress will be lost.";
}

static const char *getYesText(void) {
    return SETTINGS.language == LANGUAGE_JP ? "はい" : "Yes";
}

static const char *getNoText(void) {
    return SETTINGS.language == LANGUAGE_JP ? "いいえ" : "No";
}

void initQuitConfirmDialog(void) {
    if (gameState == GAME_STATE_QUIT_CONFIRM || initPending) {
        return;
    }
    previousGameState = gameState;
    initPending = true;
}

void tryCompleteQuitDialogInit(void) {
    if (!initPending) {
        return;
    }
    initPending = false;

    if (gameScreenshot != NULL) {
        pd->graphics->freeBitmap(gameScreenshot);
    }
    gameScreenshot = pd->graphics->newBitmap(SCREEN_WIDTH, SCREEN_HEIGHT, kColorClear);
    LCDBitmap *displayBuffer = pd->graphics->getDisplayBufferBitmap();
    pd->graphics->pushContext(gameScreenshot);
        pd->graphics->drawBitmap(displayBuffer, 0, 0, kBitmapUnflipped);
    pd->graphics->popContext();

    selectedButtonIndex = 1;
    setGameState(GAME_STATE_QUIT_CONFIRM);
}

static void freeScreenshot(void) {
    if (gameScreenshot != NULL) {
        pd->graphics->freeBitmap(gameScreenshot);
        gameScreenshot = NULL;
    }
}

static void closeDialog(void) {
    freeScreenshot();
    suppressHeldPlayerButtons();
    setGameState(previousGameState);
}

static void confirmQuit(void) {
    freeScreenshot();
    removeRuneInventoryMenuItem();
    removeQuitRunMenuItem();
    setGameState(GAME_STATE_ACTIVE);
    switchScene(SCENE_TITLE);
}

static void drawDialog(void) {
    if (gameScreenshot != NULL) {
        pd->graphics->drawBitmap(gameScreenshot, 0, 0, kBitmapUnflipped);
    }

    pd->graphics->pushContext(NULL);

    pd->graphics->fillRoundRect(DIALOG_X - 3, DIALOG_Y - 3, DIALOG_WIDTH + 6, DIALOG_HEIGHT + 6, DIALOG_CORNER_RADIUS, kColorBlack);
    pd->graphics->fillRect(DIALOG_X, DIALOG_Y, DIALOG_WIDTH, DIALOG_HEIGHT, kColorBlack);
    pd->graphics->drawRoundRect(DIALOG_X, DIALOG_Y, DIALOG_WIDTH, DIALOG_HEIGHT, DIALOG_CORNER_RADIUS, DIALOG_BORDER_WIDTH, kColorWhite);

    LCDFont *titleFont = SETTINGS.language == LANGUAGE_JP ? fontTitleJP : fontTitleEN;
    LCDFont *subFont = SETTINGS.language == LANGUAGE_JP ? fontSubJP : fontSubEN;
    int titleTracking = SETTINGS.language == LANGUAGE_JP ? 0 : -1;
    int subTracking = SETTINGS.language == LANGUAGE_JP ? 1 : -2;
    int subLeading = SETTINGS.language == LANGUAGE_JP ? 2 : 0;

    pd->graphics->setDrawMode(kDrawModeCopy);

    // Header
    const char *header = getHeaderText();
    int headerLen = strlen(header);
    pd->graphics->setFont(titleFont);
    pd->graphics->setTextTracking(titleTracking);
    int titleHeight = pd->graphics->getFontHeight(titleFont);
    int headerWidth = pd->graphics->getTextWidth(titleFont, header, headerLen, kUTF8Encoding, titleTracking);
    int headerX = DIALOG_X + (DIALOG_WIDTH - headerWidth) / 2;
    int headerY = DIALOG_Y + HEADER_TOP_PADDING;
    pd->graphics->drawText(header, headerLen, kUTF8Encoding, headerX, headerY);

    // Body
    const char *body = getBodyText();
    int bodyLen = strlen(body);
    int bodyMaxWidth = DIALOG_WIDTH - BODY_SIDE_PADDING * 2;
    pd->graphics->setFont(subFont);
    pd->graphics->setTextTracking(subTracking);
    pd->graphics->setTextLeading(subLeading);
    int bodyHeight = pd->graphics->getTextHeightForMaxWidth(subFont, body, bodyLen, bodyMaxWidth, kUTF8Encoding, kWrapWord, subTracking, subLeading);
    int bodyX = DIALOG_X + BODY_SIDE_PADDING;
    int bodyY = headerY + titleHeight + BODY_TOP_PADDING;
    pd->graphics->drawTextInRect(body, bodyLen, kUTF8Encoding, bodyX, bodyY, bodyMaxWidth, bodyHeight, kWrapWord, kAlignTextCenter);

    // Buttons
    const char *yesText = getYesText();
    const char *noText = getNoText();
    int yesLen = strlen(yesText);
    int noLen = strlen(noText);
    int yesWidth = pd->graphics->getTextWidth(subFont, yesText, yesLen, kUTF8Encoding, subTracking);
    int noWidth = pd->graphics->getTextWidth(subFont, noText, noLen, kUTF8Encoding, subTracking);
    int subFontHeight = pd->graphics->getFontHeight(subFont);

    int totalButtonsWidth = yesWidth + noWidth + BUTTON_GAP;
    int buttonsStartX = DIALOG_X + (DIALOG_WIDTH - totalButtonsWidth) / 2;
    int buttonsY = DIALOG_Y + DIALOG_HEIGHT - subFontHeight - BUTTON_BOTTOM_PADDING;

    int yesX = buttonsStartX;
    int noX = buttonsStartX + yesWidth + BUTTON_GAP;

    pd->graphics->drawText(yesText, yesLen, kUTF8Encoding, yesX, buttonsY);
    pd->graphics->drawText(noText, noLen, kUTF8Encoding, noX, buttonsY);

    int selX = selectedButtonIndex == 0 ? yesX : noX;
    int selW = selectedButtonIndex == 0 ? yesWidth : noWidth;
    pd->graphics->drawRect(selX - BUTTON_PADDING_X, buttonsY - BUTTON_PADDING_Y, selW + BUTTON_PADDING_X * 2 - 1, subFontHeight + BUTTON_PADDING_Y * 2 - 1, kColorWhite);

    pd->graphics->popContext();
}

void updateQuitConfirmDialog(float deltaTime) {
    drawDialog();

    PDButtons pushed;
    pd->system->getButtonState(NULL, &pushed, NULL);

    if (pushed & (kButtonLeft | kButtonRight)) {
        selectedButtonIndex = 1 - selectedButtonIndex;
    }

    if (pushed & kButtonA) {
        if (selectedButtonIndex == 0) {
            confirmQuit();
        } else {
            closeDialog();
        }
        return;
    }

    if (pushed & kButtonB) {
        closeDialog();
        return;
    }
}

static void onQuitRunMenuItemSelect(void *userdata) {
    if (gameState == GAME_STATE_QUIT_CONFIRM) {
        return;
    }
    initQuitConfirmDialog();
}

void createQuitRunMenuItem(void) {
    if (quitMenuItem != NULL) {
        pd->system->removeMenuItem(quitMenuItem);
    }
    quitMenuItem = pd->system->addMenuItem("Quit Run", onQuitRunMenuItemSelect, NULL);
}

void removeQuitRunMenuItem(void) {
    if (quitMenuItem != NULL) {
        pd->system->removeMenuItem(quitMenuItem);
        quitMenuItem = NULL;
    }
}
