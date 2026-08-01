#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/localization.h"
#include "globals/sceneManager.h"
#include "game/gameScene.h"
#include "game/UI/quitConfirmDialog.h"
#include "game/UI/gameMenu.h"
#include "game/player/player.h"
#include "game/rendering/textures.h"
#include "game/settings/settings.h"
#include "game/tutorial/tutorialScene.h"

#define DIALOG_WIDTH 170
#define DIALOG_HEIGHT 88
#define DIALOG_X ((SCREEN_WIDTH - DIALOG_WIDTH) / 2)
#define DIALOG_Y ((SCREEN_HEIGHT - DIALOG_HEIGHT) / 2)
#define DIALOG_CORNER_RADIUS 3
#define DIALOG_BORDER_WIDTH 2

#define HEADER_TOP_PADDING 6
#define BODY_TOP_PADDING 10
#define BODY_SIDE_PADDING 6
#define BUTTON_BOTTOM_PADDING 8
#define BUTTON_GAP 28
#define BUTTON_PADDING_X 5
#define BUTTON_PADDING_Y 3

typedef enum {
    CONFIRM_MODE_QUIT,
    CONFIRM_MODE_SKIP_TUTORIAL
} ConfirmDialogMode;

static LCDBitmap *gameScreenshot = NULL;
static GameState previousGameState;
static int selectedButtonIndex;
static bool initPending = false;
static ConfirmDialogMode dialogMode = CONFIRM_MODE_QUIT;

static const char *getHeaderText(void) {
    return getLocString(dialogMode == CONFIRM_MODE_SKIP_TUTORIAL ? STR_SKIP_TUTORIAL_TITLE : STR_QUIT_RUN_TITLE);
}

static const char *getBodyText(void) {
    return getLocString(dialogMode == CONFIRM_MODE_SKIP_TUTORIAL ? STR_SKIP_TUTORIAL_BODY : STR_QUIT_RUN_BODY);
}

static const char *getYesText(void) {
    return getLocString(STR_YES);
}

static const char *getNoText(void) {
    return getLocString(STR_NO);
}

void initQuitConfirmDialog(void) {
    if (gameState == GAME_STATE_QUIT_CONFIRM || initPending) {
        return;
    }
    dialogMode = CONFIRM_MODE_QUIT;
    previousGameState = gameState;
    initPending = true;
}

void initSkipTutorialConfirmDialog(void) {
    if (gameState == GAME_STATE_QUIT_CONFIRM || initPending) {
        return;
    }
    dialogMode = CONFIRM_MODE_SKIP_TUTORIAL;
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

void freeQuitScreenshot(void) {
    if (gameScreenshot != NULL) {
        pd->graphics->freeBitmap(gameScreenshot);
        gameScreenshot = NULL;
    }
}

static void closeDialog(void) {
    freeQuitScreenshot();
    suppressHeldPlayerButtons();
    setGameState(previousGameState);
}

static void confirmSkipTutorial(void) {
    freeQuitScreenshot();
    skipTutorial();
}

static void confirmQuit(void) {
    resetGameMenu();
    removeGameMenuItem();
    if (isTutorialActive()) {
        removeSkipTutorialMenuItem();
        setTutorialActive(false);
    }
    setGameState(GAME_STATE_QUITTING);
    switchScene(SCENE_TITLE);
}

void drawQuitScreenshot(void) {
    if (gameScreenshot != NULL) {
        pd->graphics->drawBitmap(gameScreenshot, 0, 0, kBitmapUnflipped);
    }
}

static void drawDialog(void) {
    if (gameScreenshot != NULL) {
        pd->graphics->drawBitmap(gameScreenshot, 0, 0, kBitmapUnflipped);
    }

    pd->graphics->pushContext(NULL);

    pd->graphics->fillRoundRect(DIALOG_X - 3, DIALOG_Y - 3, DIALOG_WIDTH + 6, DIALOG_HEIGHT + 6, DIALOG_CORNER_RADIUS, kColorBlack);
    pd->graphics->fillRect(DIALOG_X, DIALOG_Y, DIALOG_WIDTH, DIALOG_HEIGHT, kColorBlack);
    pd->graphics->drawRoundRect(DIALOG_X, DIALOG_Y, DIALOG_WIDTH, DIALOG_HEIGHT, DIALOG_CORNER_RADIUS, DIALOG_BORDER_WIDTH, kColorWhite);

    bool isJapanese = SETTINGS.language == LANGUAGE_JP;

    LCDFont *titleFont = isJapanese ? fontTitleJP : fontTitleEN;
    LCDFont *subFont = isJapanese ? fontSubJP : fontSubEN;
    int titleTracking = isJapanese ? 0 : -1;
    int subTracking = isJapanese ? 1 : -2;
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
    int bodyY = headerY + titleHeight + (isJapanese ? BODY_TOP_PADDING / 2 : BODY_TOP_PADDING);
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

    int noX = buttonsStartX;
    int yesX = buttonsStartX + noWidth + BUTTON_GAP;

    pd->graphics->drawText(noText, noLen, kUTF8Encoding, noX, buttonsY);
    pd->graphics->drawText(yesText, yesLen, kUTF8Encoding, yesX, buttonsY);

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
            if (dialogMode == CONFIRM_MODE_SKIP_TUTORIAL) {
                confirmSkipTutorial();
            } else {
                confirmQuit();
            }
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

