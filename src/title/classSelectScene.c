#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/sceneManager.h"
#include "title/classSelectScene.h"
#include "game/gameScene.h"
#include "game/player/classes/classData.h"
#include "game/rendering/textures.h"

#define COL_WIDTH 67
#define COL_Y 72
#define TITLE_Y 30
#define HIGHLIGHT_PADDING_X 5
#define HIGHLIGHT_PADDING_Y 3

static int selectedIndex;

void initClassSelectScene(void) {
    selectedIndex = 0;
}

void updateClassSelectScene(float deltaTime) {
    pd->graphics->clear(kColorBlack);

    pd->graphics->setFont(fontTitleEN);
    pd->graphics->setDrawMode(kDrawModeInverted);

    const char *title = "Select Class";
    int titleWidth = pd->graphics->getTextWidth(fontTitleEN, title, strlen(title), kUTF8Encoding, 0);
    pd->graphics->drawText(title, strlen(title), kUTF8Encoding, (200 - titleWidth) / 2, TITLE_Y);

    pd->graphics->setFont(fontSubEN);
    int fontHeight = pd->graphics->getFontHeight(fontSubEN);

    for (int i = 0; i < CLASS_COUNT; i++) {
        const char *name = classData[i].name;
        int nameWidth = pd->graphics->getTextWidth(fontSubEN, name, strlen(name), kUTF8Encoding, 0);
        int cx = 33 + i * COL_WIDTH;
        int textX = cx - nameWidth / 2;
        int textY = COL_Y;

        if (i == selectedIndex) {
            int rectX = textX - HIGHLIGHT_PADDING_X;
            int rectY = textY - HIGHLIGHT_PADDING_Y;
            int rectW = nameWidth + HIGHLIGHT_PADDING_X * 2;
            int rectH = fontHeight + HIGHLIGHT_PADDING_Y * 2;
            pd->graphics->setDrawMode(kDrawModeCopy);
            pd->graphics->drawRect(rectX, rectY, rectW, rectH, kColorWhite);
            pd->graphics->setDrawMode(kDrawModeInverted);
        }
        pd->graphics->drawText(name, strlen(name), kUTF8Encoding, textX, textY);
    }

    pd->graphics->setDrawMode(kDrawModeCopy);

    PDButtons pushed;
    pd->system->getButtonState(NULL, &pushed, NULL);

    if (!isSceneTransitioning()) {
        if (pushed & kButtonLeft) {
            selectedIndex = (selectedIndex - 1 + CLASS_COUNT) % CLASS_COUNT;
        } else if (pushed & kButtonRight) {
            selectedIndex = (selectedIndex + 1) % CLASS_COUNT;
        } else if (pushed & kButtonA) {
            setCurrentClass((ClassType)selectedIndex);
            initRunStart();
            switchScene(SCENE_GAME);
        }
    }
}
