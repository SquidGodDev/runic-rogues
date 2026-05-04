#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "game/rendering/textures.h"
#include "game/tutorial/tutorialObjective.h"

#define OBJ_BOX_W   84
#define OBJ_BOX_H   24
#define OBJ_BOX_X   (SCREEN_WIDTH - 4 - OBJ_BOX_W)
#define OBJ_BOX_Y   4

static bool objActive;
static const char *objTitle;
static bool objHasProgress;
static int objCurrent;
static int objMax;

void setTutorialObjective(const char *title, int current, int max) {
    objActive = true;
    objTitle = title;
    objHasProgress = true;
    objCurrent = current;
    objMax = max;
}

void setTutorialObjectiveSimple(const char *title) {
    objActive = true;
    objTitle = title;
    objHasProgress = false;
    objCurrent = 0;
    objMax = 0;
}

void clearTutorialObjective(void) {
    objActive = false;
}

bool isTutorialObjectiveActive(void) {
    return objActive;
}

void drawTutorialObjective(void) {
    if (!objActive) {
        return;
    }

    pd->graphics->fillRect(OBJ_BOX_X, OBJ_BOX_Y, OBJ_BOX_W, OBJ_BOX_H, kColorBlack);
    pd->graphics->drawRect(OBJ_BOX_X, OBJ_BOX_Y, OBJ_BOX_W, OBJ_BOX_H, kColorWhite);

    pd->graphics->setFont(fontSubEN);
    int titleW = pd->graphics->getTextWidth(fontSubEN, objTitle, strlen(objTitle), kUTF8Encoding, 0);
    int titleX = OBJ_BOX_X + (OBJ_BOX_W - titleW) / 2;
    int fontH = pd->graphics->getFontHeight(fontSubEN);

    if (objHasProgress) {
        pd->graphics->drawText(objTitle, strlen(objTitle), kUTF8Encoding, titleX, OBJ_BOX_Y + 2);
        char buf[16];
        int len = 0;
        int c = objCurrent < 0 ? 0 : (objCurrent > 99 ? 99 : objCurrent);
        int m = objMax < 0 ? 0 : (objMax > 99 ? 99 : objMax);
        if (c >= 10) { buf[len++] = '0' + (c / 10); }
        buf[len++] = '0' + (c % 10);
        buf[len++] = '/';
        if (m >= 10) { buf[len++] = '0' + (m / 10); }
        buf[len++] = '0' + (m % 10);
        buf[len] = '\0';
        int pw = pd->graphics->getTextWidth(fontSubEN, buf, len, kUTF8Encoding, 0);
        int px = OBJ_BOX_X + (OBJ_BOX_W - pw) / 2;
        pd->graphics->drawText(buf, len, kUTF8Encoding, px, OBJ_BOX_Y + 2 + fontH);
    } else {
        int titleY = OBJ_BOX_Y + (OBJ_BOX_H - fontH) / 2;
        pd->graphics->drawText(objTitle, strlen(objTitle), kUTF8Encoding, titleX, titleY);
    }
}
