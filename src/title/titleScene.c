#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/localization.h"
#include "globals/sceneManager.h"

#include "title/titleScene.h"
#include "game/gameScene.h"
#include "game/player/classes/classData.h"
#include "game/rendering/textures.h"

#define TITLE_TOP_Y 5
#define TITLE_TO_BUTTONS_GAP 39
#define BUTTON_PADDING_X 6
#define BUTTON_PADDING_Y 2
#define BUTTON_SPACING 4
#define BUTTON_COUNT 2

static int selectedIndex;

void initTitleScene(void) {
    selectedIndex = 0;
}

void updateTitleScene(float deltaTime) {
    pd->graphics->clear(kColorBlack);

    int width, height;
    pd->graphics->getBitmapData(BITMAPS[BITMAP_TITLE_TEXT], &width, &height, NULL, NULL, NULL);
    pd->graphics->drawBitmap(BITMAPS[BITMAP_TITLE_TEXT], (SCREEN_WIDTH - width) / 2, TITLE_TOP_Y, kBitmapUnflipped);
    
    pd->graphics->setFont(fontSubEN);
    int subFontHeight = pd->graphics->getFontHeight(fontSubEN);
    
    int titleFontHeight = pd->graphics->getFontHeight(fontTitleEN);
    const char *labels[BUTTON_COUNT] = { getLocString(STR_TITLE_BUTTON_START), getLocString(STR_TITLE_BUTTON_TUTORIAL) };
    int buttonsStartY = TITLE_TOP_Y + titleFontHeight * 2 + TITLE_TO_BUTTONS_GAP;
    int buttonStride = subFontHeight + BUTTON_PADDING_Y * 2 + BUTTON_SPACING;

    for (int i = 0; i < BUTTON_COUNT; i++) {
        const char *label = labels[i];
        int labelLen = strlen(label);
        int labelWidth = pd->graphics->getTextWidth(fontSubEN, label, labelLen, kUTF8Encoding, 0);
        int textX = (SCREEN_WIDTH - labelWidth) / 2;
        int textY = buttonsStartY + i * buttonStride;

        pd->graphics->drawText(label, labelLen, kUTF8Encoding, textX, textY);

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

    if (pushed & kButtonUp) {
        selectedIndex = (selectedIndex - 1 + BUTTON_COUNT) % BUTTON_COUNT;
    } else if (pushed & kButtonDown) {
        selectedIndex = (selectedIndex + 1) % BUTTON_COUNT;
    } else if (pushed & kButtonA) {
        if (selectedIndex == 0) {
            switchScene(SCENE_CLASS_SELECT);
        } else {
            setCurrentClass(CLASS_MAGE);
            initRunStart();
            switchScene(SCENE_TUTORIAL);
        }
    }
}
