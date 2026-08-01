#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/sceneManager.h"
#include "globals/saveSystem.h"
#include "title/languageSelectScene.h"
#include "game/gameScene.h"
#include "game/player/classes/classData.h"
#include "game/rendering/textures.h"
#include "game/systems/audioManager.h"
#include "game/settings/settings.h"

#define TITLE_Y 34
#define OPTIONS_TOP_Y 66
#define BUTTON_PADDING_X 6
#define BUTTON_PADDING_Y 4
#define OPTION_GAP 20
#define TRACKING_EN -1
#define TRACKING_JP 0
#define OPTION_COUNT 2

// 0 = English, 1 = 日本語. Maps directly onto the Language enum.
static int selectedIndex;

void initLanguageSelectScene(void) {
    // Default the cursor to the system-detected language so the first press of A
    // keeps that language if the player agrees with the auto-detection.
    selectedIndex = (SETTINGS.language == LANGUAGE_JP) ? 1 : 0;
    playSong(MUSIC_TITLE);
}

// Draws label centered horizontally within a fixed-height row, with a selection
// rectangle border matching the title screen buttons. Returns the box width so
// callers can lay the two options out side by side.
static void drawOption(const char *label, LCDFont *font, int tracking, int boxX, int rowTop, int rowHeight, int yNudge, bool selected) {
    pd->graphics->setFont(font);
    pd->graphics->setTextTracking(tracking);
    int fontHeight = pd->graphics->getFontHeight(font);
    int labelLen = strlen(label);
    int labelWidth = pd->graphics->getTextWidth(font, label, labelLen, kUTF8Encoding, tracking) - 1;

    int textX = boxX + BUTTON_PADDING_X;
    int textY = rowTop + BUTTON_PADDING_Y + (rowHeight - fontHeight) / 2;
    pd->graphics->drawText(label, labelLen, kUTF8Encoding, textX, textY + yNudge);

    if (selected) {
        int rectW = labelWidth + BUTTON_PADDING_X * 2;
        int rectH = rowHeight + BUTTON_PADDING_Y * 2;
        pd->graphics->drawRect(boxX, rowTop, rectW, rectH, kColorWhite);
    }
}

static int optionBoxWidth(const char *label, LCDFont *font, int tracking) {
    int labelWidth = pd->graphics->getTextWidth(font, label, strlen(label), kUTF8Encoding, tracking) - 1;
    return labelWidth + BUTTON_PADDING_X * 2;
}

void updateLanguageSelectScene(float deltaTime) {
    (void)deltaTime;

    setMusicPaused(false);

    pd->graphics->clear(kColorBlack);
    pd->graphics->setDrawMode(kDrawModeCopy);

    bool japaneseHovered = false; // (selectedIndex == 1);

    LCDFont *titleFont = japaneseHovered ? fontTitleJP : fontTitleEN;
    const char *titleText = japaneseHovered ? "言語" : "Language";
    pd->graphics->setFont(titleFont);
    pd->graphics->setTextTracking(0);
    int titleWidth = pd->graphics->getTextWidth(titleFont, titleText, strlen(titleText), kUTF8Encoding, 0);
    pd->graphics->drawText(titleText, strlen(titleText), kUTF8Encoding, (SCREEN_WIDTH - titleWidth) / 2, TITLE_Y + (japaneseHovered ? -1 : 0));

    // The two options render in their own language's font regardless of which is
    // selected, laid out side by side and centered as a group.
    int enHeight = pd->graphics->getFontHeight(fontSubEN);
    int jpHeight = pd->graphics->getFontHeight(fontSubJP);
    int rowHeight = enHeight > jpHeight ? enHeight : jpHeight;

    int enBox = optionBoxWidth("English", fontSubEN, TRACKING_EN);
    int jpBox = optionBoxWidth("日本語", fontSubJP, TRACKING_JP);
    int totalWidth = enBox + OPTION_GAP + jpBox;
    int startX = (SCREEN_WIDTH - totalWidth) / 2;

    drawOption("English", fontSubEN, TRACKING_EN, startX, OPTIONS_TOP_Y, rowHeight, 0, selectedIndex == 0);
    drawOption("日本語", fontSubJP, TRACKING_JP, startX + enBox + OPTION_GAP, OPTIONS_TOP_Y, rowHeight, -1, selectedIndex == 1);

    if (isSceneTransitioning()) {
        return;
    }

    PDButtons pushed;
    pd->system->getButtonState(NULL, &pushed, NULL);

    if (pushed & kButtonLeft) {
        if (selectedIndex > 0) {
            playSound(SFX_UI_RUNE_SLIDE);
            selectedIndex--;
        }
    } else if (pushed & kButtonRight) {
        if (selectedIndex < OPTION_COUNT - 1) {
            playSound(SFX_UI_RUNE_SLIDE);
            selectedIndex++;
        }
    } else if (pushed & kButtonA) {
        playSound(SFX_CLASS_SELECT);
        SETTINGS.language = (selectedIndex == 1) ? LANGUAGE_JP : LANGUAGE_EN;
        saveGame();
        setCurrentClass(CLASS_MAGE);
        initRunStart();
        switchScene(SCENE_TUTORIAL);
    }
}
