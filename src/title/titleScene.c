#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/localization.h"
#include "globals/sceneManager.h"
#include "globals/saveSystem.h"

#include "title/titleScene.h"
#include "game/gameScene.h"
#include "game/player/classes/classData.h"
#include "game/rendering/textures.h"
#include "game/systems/audioManager.h"
#include "game/UI/gameMenu.h"

#define TITLE_TOP_Y 0
#define TITLE_TO_BUTTONS_GAP 33
#define TITLE_TO_BUTTONS_GAP_JP 30
#define BUTTON_PADDING_X 6
#define BUTTON_PADDING_Y 2
#define BUTTON_SPACING 1
#define BUTTON_SPACING_JP 0
#define BUTTON_COUNT 3
#define BUTTON_TRACKING -1
#define BUTTON_TRACKING_JP 0

static int selectedIndex;
static bool settingsOpen;

// Secret "RUBDUBDUB" code (right-up-b-down-up-b-down-up-b) toggles debug mode.
static const PDButtons DEBUG_CODE[] = {
    kButtonRight, kButtonUp, kButtonB,
    kButtonDown, kButtonUp, kButtonB,
    kButtonDown, kButtonUp, kButtonB,
};
#define DEBUG_CODE_LENGTH ((int)(sizeof(DEBUG_CODE) / sizeof(DEBUG_CODE[0])))
static int debugCodeProgress;

// Advances the debug-code matcher by one button press. A press matching the next
// expected button advances; any other press restarts (from 1 if it's the code's
// first button, else 0). Completing the sequence toggles and persists debug mode.
static void updateDebugCode(PDButtons pushed) {
    PDButtons codeButtons = pushed & (kButtonUp | kButtonDown | kButtonLeft | kButtonRight | kButtonA | kButtonB);
    if (codeButtons == 0) {
        return;
    }
    if (codeButtons == DEBUG_CODE[debugCodeProgress]) {
        debugCodeProgress++;
        if (debugCodeProgress == DEBUG_CODE_LENGTH) {
            debugCodeProgress = 0;
            DEBUG.enabled = !DEBUG.enabled;
            saveGame();
            playSound(DEBUG.enabled ? SFX_UI_CHECKBOX_CHECKED : SFX_UI_CHECKBOX_UNCHECKED);
        }
    } else if (codeButtons == DEBUG_CODE[0]) {
        debugCodeProgress = 1;
    } else {
        debugCodeProgress = 0;
    }
}

static LCDFont *buttonFont(void) {
    return (SETTINGS.language == LANGUAGE_JP) ? fontSubJP : fontSubEN;
}

void initTitleScene(void) {
    selectedIndex = 0;
    settingsOpen = false;
    debugCodeProgress = 0;
    playSong(MUSIC_TITLE);
}

void updateTitleScene(float deltaTime) {
    if (settingsOpen) {
        setMusicPausedWithFade(true);
        updateGameMenu(deltaTime);
        if (!gameMenuActive()) {
            settingsOpen = false;
        }
        return;
    }

    setMusicPaused(false);

    pd->graphics->clear(kColorBlack);

    bool isJapanese = (SETTINGS.language == LANGUAGE_JP);

    int width, height;
    pd->graphics->getBitmapData(BITMAPS[BITMAP_TITLE_TEXT], &width, &height, NULL, NULL, NULL);
    pd->graphics->drawBitmap(BITMAPS[BITMAP_TITLE_TEXT], (SCREEN_WIDTH - width) / 2, TITLE_TOP_Y, kBitmapUnflipped);
    
    LCDFont *subFont = buttonFont();
    pd->graphics->setFont(subFont);
    int tracking = isJapanese ? BUTTON_TRACKING_JP : BUTTON_TRACKING;
    pd->graphics->setTextTracking(tracking);
    int subFontHeight = pd->graphics->getFontHeight(subFont);
    
    int titleFontHeight = pd->graphics->getFontHeight(fontTitleEN);
    const char *labels[BUTTON_COUNT] = { getLocString(STR_TITLE_BUTTON_START), getLocString(STR_TITLE_BUTTON_TUTORIAL), getLocString(STR_TITLE_BUTTON_SETTINGS) };
    int buttonsStartY = TITLE_TOP_Y + titleFontHeight * 2 + (isJapanese ? TITLE_TO_BUTTONS_GAP_JP : TITLE_TO_BUTTONS_GAP);
    int buttonStride = subFontHeight + BUTTON_PADDING_Y * 2 + (isJapanese ? BUTTON_SPACING_JP : BUTTON_SPACING);

    for (int i = 0; i < BUTTON_COUNT; i++) {
        const char *label = labels[i];
        int labelLen = strlen(label);
        int labelWidth = pd->graphics->getTextWidth(subFont, label, labelLen, kUTF8Encoding, tracking) - 1;
        int textX = (SCREEN_WIDTH - labelWidth) / 2;
        int textY = buttonsStartY + i * buttonStride;

        pd->graphics->drawText(label, labelLen, kUTF8Encoding, textX, textY + (isJapanese ? -1 : 0));

        if (i == selectedIndex) {
            int rectX = textX - BUTTON_PADDING_X;
            int rectY = textY - BUTTON_PADDING_Y - 1;
            int rectW = labelWidth + BUTTON_PADDING_X * 2;
            int rectH = subFontHeight + BUTTON_PADDING_Y * 2;
            pd->graphics->drawRect(rectX, rectY, rectW, rectH, kColorWhite);
        }
    }

    if (isSceneTransitioning()) {
        return;
    }

    PDButtons pushed;
    pd->system->getButtonState(NULL, &pushed, NULL);

    updateDebugCode(pushed);

    if (pushed & kButtonUp) {
        playSound(SFX_UI_RUNE_SLIDE);
        selectedIndex = (selectedIndex - 1 + BUTTON_COUNT) % BUTTON_COUNT;
    } else if (pushed & kButtonDown) {
        playSound(SFX_UI_RUNE_SLIDE);
        selectedIndex = (selectedIndex + 1) % BUTTON_COUNT;
    } else if (pushed & kButtonA) {
        playSound(SFX_CLASS_SELECT);
        if (selectedIndex == 0) {
            switchScene(SCENE_CLASS_SELECT);
        } else if (selectedIndex == 1) {
            setCurrentClass(CLASS_MAGE);
            initRunStart();
            switchScene(SCENE_TUTORIAL);
        } else {
            initSettingsMenu();
            settingsOpen = true;
        }
    }
}
