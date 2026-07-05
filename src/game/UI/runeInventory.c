#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/helpers.h"
#include "game/UI/runeInventory.h"
#include "game/systems/runeManager.h"
#include "game/rendering/textures.h"
#include "game/settings/settings.h"

// Row layout
#define RUNE_ROW_CENTER_X       100
#define RUNE_ROW_CENTER_Y       40
#define RUNE_ROW_STRIDE         32
#define RUNE_ROW_ICON_SIZE      28
#define RUNE_ROW_SELECTED_LIFT  5
#define RUNE_ROW_CLIP_TOP       16
#define RUNE_ROW_CLIP_BOTTOM    56

// Description layout
#define RUNE_DESC_X             4
#define RUNE_DESC_Y             60
#define RUNE_DESC_WIDTH         185
#define RUNE_DESC_HEIGHT        (SCREEN_HEIGHT - RUNE_DESC_Y - 2)
#define RUNE_SCROLLBAR_X        193
#define RUNE_SCROLLBAR_Y        61
#define RUNE_SCROLLBAR_TRACK_H  57

#define RUNE_SCROLL_SENSITIVITY 0.2f
#define RUNE_SCROLL_BUTTON_SPEED 120.0f

typedef struct RunePageData {
    LCDBitmap *pageBitmapEN;
    int pageHeightEN;
    LCDBitmap *pageBitmapJP;
    int pageHeightJP;
} RunePageData;

static RuneType ownedRunesList[RUNE_TYPE_COUNT];
static int ownedRuneCount;
static int cursorIndex;
static float animCursorIndex;

static RunePageData runePageCache[RUNE_TYPE_COUNT];
static bool runePageCached[RUNE_TYPE_COUNT];

static int descScrollOffset;
static int maxDescScrollOffset;
static float scrollAccum;
static Language lastPageLanguage;  // locale the scroll bounds were last computed for

#define HOLD_DELAY      0.3f
#define HOLD_REPEAT     0.2f

static PDButtons heldDir;
static float holdTimer;

// When entering the scrollbar via a held Down press, ignore scroll input until the
// player releases Down and presses it again.
static bool scrollInputSuppressed;

static const char *ROMAN_NUMERALS[] = { "", "I", "II", "III", "IV", "V" };

static RunePageData getRunePage(RuneType runeType) {
    if (runePageCached[runeType]) {
        return runePageCache[runeType];
    }

    int level = getRuneLevel(runeType);
    char *nameEN = NULL;
    char *nameJP = NULL;
    pd->system->formatString(&nameEN, "%s %s", getRuneName(runeType, LANGUAGE_EN), ROMAN_NUMERALS[level]);
    pd->system->formatString(&nameJP, "%s %s", getRuneName(runeType, LANGUAGE_JP), ROMAN_NUMERALS[level]);
    char *descEN = getRuneDescription(runeType, LANGUAGE_EN, level);
    char *descJP = getRuneDescription(runeType, LANGUAGE_JP, level);

    // EN bitmap
    {
        int titleTracking = -1;
        int tracking = -2;
        int leading = 0;
        int titleH = pd->graphics->getFontHeight(fontTitleEN);
        int descH = pd->graphics->getTextHeightForMaxWidth(fontSubEN, descEN, strlen(descEN),
                        RUNE_DESC_WIDTH, kUTF8Encoding, kWrapWord, tracking, leading);
        int totalH = titleH + 6 + descH;
        LCDBitmap *bmp = pd->graphics->newBitmap(RUNE_DESC_WIDTH, totalH, kColorClear);
        pd->graphics->pushContext(bmp);
            pd->graphics->setFont(fontTitleEN);
            pd->graphics->setTextTracking(titleTracking);
            pd->graphics->drawText(nameEN, strlen(nameEN), kUTF8Encoding, 0, 0);
            pd->graphics->setFont(fontSubEN);
            pd->graphics->setTextTracking(tracking);
            pd->graphics->setTextLeading(leading);
            pd->graphics->setDrawMode(kDrawModeCopy);
            pd->graphics->drawTextInRect(descEN, strlen(descEN), kUTF8Encoding,
                                         0, titleH + 6, RUNE_DESC_WIDTH, descH,
                                         kWrapWord, kAlignTextLeft);
        pd->graphics->popContext();
        pd->system->realloc(nameEN, 0);
        runePageCache[runeType].pageBitmapEN = bmp;
        runePageCache[runeType].pageHeightEN = totalH;
    }

    // JP bitmap
    {
        int titleTracking = -4;
        int tracking = 1;
        int leading = 2;
        int titleH = pd->graphics->getFontHeight(fontTitleJP);
        int descH = pd->graphics->getTextHeightForMaxWidth(fontSubJP, descJP, strlen(descJP),
                        RUNE_DESC_WIDTH, kUTF8Encoding, kWrapWord, tracking, leading);
        int totalH = titleH + 6 + descH;
        LCDBitmap *bmp = pd->graphics->newBitmap(RUNE_DESC_WIDTH, totalH, kColorClear);
        pd->graphics->pushContext(bmp);
            pd->graphics->setFont(fontTitleJP);
            pd->graphics->setTextTracking(titleTracking);
            pd->graphics->drawText(nameJP, strlen(nameJP), kUTF8Encoding, 0, 0);
            pd->graphics->setFont(fontSubJP);
            pd->graphics->setTextTracking(tracking);
            pd->graphics->setTextLeading(leading);
            pd->graphics->setDrawMode(kDrawModeCopy);
            pd->graphics->drawTextInRect(descJP, strlen(descJP), kUTF8Encoding,
                                         0, titleH + 6, RUNE_DESC_WIDTH, descH,
                                         kWrapWord, kAlignTextLeft);
        pd->graphics->popContext();
        pd->system->realloc(nameJP, 0);
        runePageCache[runeType].pageBitmapJP = bmp;
        runePageCache[runeType].pageHeightJP = totalH;
    }

    runePageCached[runeType] = true;
    return runePageCache[runeType];
}

static void resetDescScrollOffset(void);
static void updateMaxDescScrollOffset(void);

static void drawRuneInventoryTab(RuneInventoryFocus focus) {
    if (ownedRuneCount > 0) {
        pd->graphics->setClipRect(0, RUNE_ROW_CLIP_TOP, SCREEN_WIDTH, RUNE_ROW_CLIP_BOTTOM - RUNE_ROW_CLIP_TOP + 1);
        for (int i = ownedRuneCount - 1; i >= 0; i--) {
            float iconCenterX = RUNE_ROW_CENTER_X + (i - animCursorIndex) * RUNE_ROW_STRIDE;
            if (iconCenterX < -RUNE_ROW_ICON_SIZE || iconCenterX > SCREEN_WIDTH + RUNE_ROW_ICON_SIZE) {
                continue;
            }

            float distFromCenter = fabsf(iconCenterX - RUNE_ROW_CENTER_X);
            float liftFactor = clampFloat(1.0f - distFromCenter / RUNE_ROW_STRIDE, 0.0f, 1.0f);
            int iconCenterY = RUNE_ROW_CENTER_Y - (int)(liftFactor * RUNE_ROW_SELECTED_LIFT);

            int drawX = (int)iconCenterX - RUNE_ROW_ICON_SIZE / 2;
            int drawY = iconCenterY - RUNE_ROW_ICON_SIZE / 2;

            int bitmapIndex = getRuneIconIndex(ownedRunesList[i]);
            pd->graphics->setClipRect(drawX, drawY, RUNE_ROW_ICON_SIZE, RUNE_ROW_ICON_SIZE);
            pd->graphics->drawBitmap(BITMAPS[bitmapIndex], drawX, drawY, kBitmapUnflipped);
            pd->graphics->setClipRect(0, RUNE_ROW_CLIP_TOP, SCREEN_WIDTH, RUNE_ROW_CLIP_BOTTOM - RUNE_ROW_CLIP_TOP + 1);

            if (i == cursorIndex && focus == RUNE_INV_FOCUS_RUNES) {
                pd->graphics->drawBitmap(BITMAPS[BITMAP_MENU_RUNE_SELECT_CURSOR], drawX - 1, drawY - 1, kBitmapUnflipped);
            }
        }
        pd->graphics->clearClipRect();
    }

    pd->graphics->drawLine(0, RUNE_ROW_CLIP_BOTTOM + 1, SCREEN_WIDTH, RUNE_ROW_CLIP_BOTTOM + 1, 1, kColorWhite);

    if (ownedRuneCount > 0) {
        RunePageData page = getRunePage(ownedRunesList[cursorIndex]);
        LCDBitmap *pageBitmap;
        int pageHeight;
        if (SETTINGS.language == LANGUAGE_JP) {
            pageBitmap = page.pageBitmapJP;
            pageHeight = page.pageHeightJP;
        } else {
            pageBitmap = page.pageBitmapEN;
            pageHeight = page.pageHeightEN;
        }

        pd->graphics->setClipRect(RUNE_DESC_X, RUNE_DESC_Y, RUNE_DESC_WIDTH, RUNE_DESC_HEIGHT);
        pd->graphics->drawBitmap(pageBitmap, RUNE_DESC_X, RUNE_DESC_Y - descScrollOffset, kBitmapUnflipped);
        pd->graphics->clearClipRect();

        if (maxDescScrollOffset > 0) {
            float viewRatio = (float)RUNE_DESC_HEIGHT / pageHeight;
            int thumbH = (int)(RUNE_SCROLLBAR_TRACK_H * viewRatio) - 4;
            int thumbY = RUNE_SCROLLBAR_Y + (int)(RUNE_SCROLLBAR_TRACK_H * ((float)descScrollOffset / pageHeight)) + 1;
            pd->graphics->drawRoundRect(RUNE_SCROLLBAR_X, RUNE_SCROLLBAR_Y, 5, RUNE_SCROLLBAR_TRACK_H, 2, 1, kColorWhite);
            if (focus == RUNE_INV_FOCUS_SCROLLBAR) {
                pd->graphics->drawLine(RUNE_SCROLLBAR_X + 2, thumbY + 2, RUNE_SCROLLBAR_X + 2, thumbY + thumbH, 1, kColorWhite);
            } else {
                for (int dotY = thumbY + 2; dotY <= thumbY + thumbH; dotY += 2) {
                    pd->graphics->fillRect(RUNE_SCROLLBAR_X + 2, dotY, 1, 1, kColorWhite);
                }
            }
        }
    }
}

static void navigate(PDButtons dir) {
    if (dir & kButtonLeft) {
        if (cursorIndex == 0) {
            cursorIndex = ownedRuneCount - 1;
        } else {
            cursorIndex--;
        }
        resetDescScrollOffset();
    } else if (dir & kButtonRight) {
        if (cursorIndex == ownedRuneCount - 1) {
            cursorIndex = 0;
        } else {
            cursorIndex++;
        }
        resetDescScrollOffset();
    }
}

static void applyScroll(float amount) {
    scrollAccum += amount;
    int step = (int)scrollAccum;
    scrollAccum -= step;
    descScrollOffset += step;
    if (descScrollOffset < 0) {
        descScrollOffset = 0;
    } else if (descScrollOffset > maxDescScrollOffset) {
        descScrollOffset = maxDescScrollOffset;
    }
}

void updateRuneInventoryTab(float deltaTime, RuneInventoryFocus focus) {
    if (SETTINGS.language != lastPageLanguage) {
        lastPageLanguage = SETTINGS.language;
        updateMaxDescScrollOffset();
        if (descScrollOffset > maxDescScrollOffset) {
            descScrollOffset = maxDescScrollOffset;
        }
    }

    animCursorIndex = lerp(animCursorIndex, (float)cursorIndex, 0.2f);

    drawRuneInventoryTab(focus);

    if (ownedRuneCount == 0 || focus == RUNE_INV_FOCUS_NONE) {
        return;
    }

    PDButtons current, pushed;
    pd->system->getButtonState(&current, &pushed, NULL);

    if (focus == RUNE_INV_FOCUS_RUNES) {
        PDButtons dirCurrent = current & (kButtonLeft | kButtonRight);
        PDButtons dirPushed = pushed & (kButtonLeft | kButtonRight);

        if (dirPushed) {
            heldDir = dirPushed;
            holdTimer = 0.0f;
            navigate(dirPushed);
        } else if (dirCurrent && dirCurrent == heldDir) {
            float prevTimer = holdTimer;
            holdTimer += deltaTime;
            if (holdTimer >= HOLD_DELAY) {
                float phase = holdTimer - HOLD_DELAY;
                float prevPhase = prevTimer >= HOLD_DELAY ? prevTimer - HOLD_DELAY : 0.0f;
                if ((int)(phase / HOLD_REPEAT) > (int)(prevPhase / HOLD_REPEAT)) {
                    navigate(heldDir);
                }
            }
        } else {
            heldDir = 0;
            holdTimer = 0.0f;
        }
    } else if (focus == RUNE_INV_FOCUS_SCROLLBAR) {
        if (scrollInputSuppressed) {
            if (!(current & kButtonDown)) {
                scrollInputSuppressed = false;
            }
        } else {
            if (current & kButtonUp) {
                applyScroll(-RUNE_SCROLL_BUTTON_SPEED * deltaTime);
            }
            if (current & kButtonDown) {
                applyScroll(RUNE_SCROLL_BUTTON_SPEED * deltaTime);
            }
        }
    }

    float crankChange = pd->system->getCrankChange() * RUNE_SCROLL_SENSITIVITY;
    if (crankChange != 0) {
        applyScroll(crankChange);
    }
}

static void updateMaxDescScrollOffset(void) {
    if (ownedRuneCount == 0) {
        maxDescScrollOffset = 0;
        return;
    }
    RunePageData page = getRunePage(ownedRunesList[cursorIndex]);
    int pageHeight = SETTINGS.language == LANGUAGE_JP ? page.pageHeightJP : page.pageHeightEN;
    int overflow = pageHeight - RUNE_DESC_HEIGHT;
    maxDescScrollOffset = overflow > 0 ? overflow : 0;
}

static void resetDescScrollOffset(void) {
    descScrollOffset = 0;
    scrollAccum = 0.0f;
    lastPageLanguage = SETTINGS.language;
    updateMaxDescScrollOffset();
}

bool runeInventoryHasScrollbar(void) {
    if (ownedRuneCount == 0) {
        return false;
    }
    RunePageData page = getRunePage(ownedRunesList[cursorIndex]);
    int pageHeight = SETTINGS.language == LANGUAGE_JP ? page.pageHeightJP : page.pageHeightEN;
    return pageHeight > RUNE_DESC_HEIGHT;
}

bool runeInventoryAtScrollTop(void) {
    return descScrollOffset <= 0;
}

void notifyEnterRuneScrollbar(void) {
    scrollInputSuppressed = true;
}

void initRuneInventoryTab(void) {
    ownedRuneCount = 0;
    for (int i = RUNE_TYPE_NONE + 1; i < RUNE_TYPE_COUNT; i++) {
        if (runeIsOwned((RuneType)i)) {
            ownedRunesList[ownedRuneCount] = (RuneType)i;
            ownedRuneCount++;
        }
    }

    cursorIndex = 0;
    animCursorIndex = 0.0f;
    heldDir = 0;
    holdTimer = 0.0f;
    scrollInputSuppressed = false;
    resetDescScrollOffset();
}

void freeRuneInventoryCache(void) {
    for (int i = 0; i < RUNE_TYPE_COUNT; i++) {
        if (runePageCached[i]) {
            if (runePageCache[i].pageBitmapEN != NULL) {
                pd->graphics->freeBitmap(runePageCache[i].pageBitmapEN);
            }
            if (runePageCache[i].pageBitmapJP != NULL) {
                pd->graphics->freeBitmap(runePageCache[i].pageBitmapJP);
            }
            runePageCache[i] = (RunePageData){0};
            runePageCached[i] = false;
        }
    }
}
