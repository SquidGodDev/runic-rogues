#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "popups.h"

#include "globals/globals.h"
#include "game/rendering/textures.h"
#include "game/settings/settings.h"

typedef struct PopupDef {
    const char *textEN;
    const char *textJP;
} PopupDef;

static const PopupDef POPUP_DEFS[POPUP_TYPE_COUNT] = {
    [POPUP_ROOM_CLEARED] = { "Room Cleared", "部屋クリア" },
    [POPUP_AREA_CLEARED] = { "Floor Cleared", "フロア クリア" },
    [POPUP_ELDER_BOSS] = { "The Elder", "エルダー" },
    [POPUP_WRAITH_BOSS] = { "The Wraith", "レイス" },
};

static const uint8_t POPUP_FADE_PATTERNS[POPUP_FADE_IN_FRAMES][8] = {
    {0x77, 0xFF, 0x55, 0xFF, 0x77, 0xFF, 0x55, 0xFF},
    {0x55, 0xBB, 0x55, 0xAA, 0x55, 0xBB, 0x55, 0xAA},
    {0x55, 0x22, 0x55, 0x22, 0x55, 0x22, 0x55, 0x22},
    {0x11, 0x00, 0x44, 0x00, 0x11, 0x00, 0x44, 0x00},
};

static LCDBitmap *popupBitmaps[POPUP_TYPE_COUNT][2][POPUP_BITMAP_COUNT];
static LCDBitmap *fadeStencils[POPUP_FADE_IN_FRAMES];

static void buildFadeStencils(void) {
    for (int i = 0; i < POPUP_FADE_IN_FRAMES; i++) {
        LCDBitmap *bmp = pd->graphics->newBitmap(32, 8, kColorWhite);
        int bw, bh, rowbytes;
        uint8_t *mask = NULL;
        uint8_t *data = NULL;
        pd->graphics->getBitmapData(bmp, &bw, &bh, &rowbytes, &mask, &data);
        for (int y = 0; y < 8; y++) {
            uint8_t byte = POPUP_FADE_PATTERNS[i][y];
            for (int b = 0; b < rowbytes; b++) {
                data[y * rowbytes + b] = ~byte;
            }
        }
        fadeStencils[i] = bmp;
    }
}

static void buildPopupTable(PopupType type, Language language) {
    const PopupDef *def = &POPUP_DEFS[type];
    const char *text = (language == LANGUAGE_JP) ? def->textJP : def->textEN;
    LCDFont *font = (language == LANGUAGE_JP) ? fontTitleJP : fontTitleEN;
    int len = (int)strlen(text);

    int tracking = 0;
    int width = pd->graphics->getTextWidth(font, text, len, kUTF8Encoding, tracking);
    int height = pd->graphics->getFontHeight(font);

    for (int i = 0; i < POPUP_BITMAP_COUNT; i++) {
        LCDBitmap *slot = pd->graphics->newBitmap(width, height, kColorClear);
        pd->graphics->pushContext(slot);
            if (i < POPUP_FADE_IN_FRAMES) {
                pd->graphics->setStencilImage(fadeStencils[i], 1);
            } else if (i > POPUP_FADE_IN_FRAMES) {
                int reverseIdx = POPUP_BITMAP_COUNT - 1 - i;
                pd->graphics->setStencilImage(fadeStencils[reverseIdx], 1);
            }
            pd->graphics->setFont(font);
            pd->graphics->setTextTracking(tracking);
            pd->graphics->setDrawMode(kDrawModeCopy);
            pd->graphics->drawText(text, len, kUTF8Encoding, 0, 0);
        pd->graphics->popContext();
        popupBitmaps[type][language][i] = slot;
    }
}

void initPopups(void) {
    buildFadeStencils();
    for (int type = 0; type < POPUP_TYPE_COUNT; type++) {
        buildPopupTable((PopupType)type, LANGUAGE_EN);
        buildPopupTable((PopupType)type, LANGUAGE_JP);
    }
}

LCDBitmap *getPopupBitmap(PopupType type, int curFrame) {
    int slot;
    if (curFrame < POPUP_FADE_IN_FRAMES) {
        slot = curFrame;
    } else if (curFrame < POPUP_FADE_IN_FRAMES + POPUP_HOLD_FRAMES) {
        slot = POPUP_FADE_IN_FRAMES;
    } else {
        slot = POPUP_FADE_IN_FRAMES + 1 + (curFrame - POPUP_FADE_IN_FRAMES - POPUP_HOLD_FRAMES);
    }
    return popupBitmaps[type][SETTINGS.language][slot];
}
