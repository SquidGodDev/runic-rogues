#pragma once

#include "pd_api.h"

typedef enum PopupType {
    POPUP_ROOM_CLEARED,
    POPUP_AREA_CLEARED,
    POPUP_ELDER_BOSS,
    POPUP_WRAITH_BOSS,
    POPUP_TYPE_COUNT,
} PopupType;

typedef enum PopupPosition {
    POPUP_POSITION_TOP,
    POPUP_POSITION_CENTER,
    POPUP_POSITION_BOTTOM,
} PopupPosition;

#define POPUP_FADE_IN_FRAMES   4
#define POPUP_HOLD_FRAMES      15
#define POPUP_FADE_OUT_FRAMES  4
#define POPUP_MAX_FRAME        (POPUP_FADE_IN_FRAMES + POPUP_HOLD_FRAMES + POPUP_FADE_OUT_FRAMES)
#define POPUP_FRAME_TIME       0.05f

#define POPUP_BITMAP_COUNT     (POPUP_FADE_IN_FRAMES + 1 + POPUP_FADE_OUT_FRAMES)

void initPopups(void);
LCDBitmap *getPopupBitmap(PopupType type, int curFrame);
