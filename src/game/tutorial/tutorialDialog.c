#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/helpers.h"
#include "game/rendering/textures.h"
#include "game/tutorial/tutorialDialog.h"

#define TD_BOX_MARGIN       6
#define TD_BOX_PADDING_X    6
#define TD_BOX_PADDING_Y    5
#define TD_BOX_HEIGHT       42
#define TD_BOX_X            TD_BOX_MARGIN
#define TD_BOX_W            (SCREEN_WIDTH - TD_BOX_MARGIN * 2)
#define TD_BOX_Y_BASE       (SCREEN_HEIGHT - TD_BOX_MARGIN - TD_BOX_HEIGHT)
#define TD_TEXT_X           (TD_BOX_X + TD_BOX_PADDING_X)
#define TD_TEXT_MAX_W       (TD_BOX_W - TD_BOX_PADDING_X * 2)
#define TD_LINE_LEADING     0
#define TD_MAX_LINES        4
#define TD_MAX_LINE_LEN     128

#define TD_CHAR_TIME        0.05f
#define TD_IN_DURATION      0.70f
#define TD_OUT_DURATION     0.5f

typedef enum {
    TD_HIDDEN,
    TD_TRANSITION_IN,
    TD_TYPING,
    TD_AWAIT_ADVANCE,
    TD_TRANSITION_OUT
} TDState;

static TDState dState = TD_HIDDEN;
static const char *const *dPages;
static int dPageCount;
static int dPageIndex;
static char dLines[TD_MAX_LINES][TD_MAX_LINE_LEN];
static int dLineCharCounts[TD_MAX_LINES];
static int dLineCount;
static int dPageTotalChars;
static int dRevealedChars;
static float dCharAccumulator;
static float dTransitionTime;
static bool dCompletedFlag;

static int utf8NumChars(const char *s) {
    int n = 0;
    for (int i = 0; s[i]; i++) {
        if (((unsigned char)s[i] & 0xC0) != 0x80) {
            n++;
        }
    }
    return n;
}

static int utf8ByteOffsetForChar(const char *s, int charCount) {
    if (charCount <= 0) {
        return 0;
    }
    int ci = 0;
    int i = 0;
    while (s[i]) {
        unsigned char c = (unsigned char)s[i];
        if ((c & 0xC0) != 0x80) {
            if (ci == charCount) {
                return i;
            }
            ci++;
        }
        i++;
    }
    return i;
}

static void wrapPage(const char *text) {
    dLineCount = 0;
    dPageTotalChars = 0;
    char cur[TD_MAX_LINE_LEN];
    cur[0] = '\0';
    int curLen = 0;

    int wordStart = 0;
    int i = 0;
    while (1) {
        char c = text[i];
        if (c == ' ' || c == '\0') {
            int wordLen = i - wordStart;
            if (wordLen > 0) {
                char trial[TD_MAX_LINE_LEN];
                int trialLen;
                if (curLen == 0) {
                    if (wordLen >= TD_MAX_LINE_LEN) {
                        wordLen = TD_MAX_LINE_LEN - 1;
                    }
                    memcpy(trial, text + wordStart, wordLen);
                    trial[wordLen] = '\0';
                    trialLen = wordLen;
                } else {
                    trialLen = curLen + 1 + wordLen;
                    if (trialLen >= TD_MAX_LINE_LEN) {
                        trialLen = TD_MAX_LINE_LEN - 1;
                        wordLen = trialLen - curLen - 1;
                        if (wordLen < 0) {
                            wordLen = 0;
                        }
                    }
                    memcpy(trial, cur, curLen);
                    trial[curLen] = ' ';
                    memcpy(trial + curLen + 1, text + wordStart, wordLen);
                    trial[trialLen] = '\0';
                }

                int width = pd->graphics->getTextWidth(fontSubEN, trial, trialLen, kUTF8Encoding, 0);

                if (width <= TD_TEXT_MAX_W || curLen == 0) {
                    memcpy(cur, trial, trialLen + 1);
                    curLen = trialLen;
                } else {
                    if (dLineCount < TD_MAX_LINES) {
                        memcpy(dLines[dLineCount], cur, curLen + 1);
                        dLineCharCounts[dLineCount] = utf8NumChars(dLines[dLineCount]);
                        dPageTotalChars += dLineCharCounts[dLineCount];
                        dLineCount++;
                    }
                    if (wordLen >= TD_MAX_LINE_LEN) {
                        wordLen = TD_MAX_LINE_LEN - 1;
                    }
                    memcpy(cur, text + wordStart, wordLen);
                    cur[wordLen] = '\0';
                    curLen = wordLen;
                }
            }
            wordStart = i + 1;
        }
        if (c == '\0') {
            break;
        }
        i++;
    }
    if (curLen > 0 && dLineCount < TD_MAX_LINES) {
        memcpy(dLines[dLineCount], cur, curLen + 1);
        dLineCharCounts[dLineCount] = utf8NumChars(dLines[dLineCount]);
        dPageTotalChars += dLineCharCounts[dLineCount];
        dLineCount++;
    }

    dRevealedChars = 0;
    dCharAccumulator = 0;
}

void openTutorialDialogPages(const char *const *pages, int pageCount) {
    dPages = pages;
    dPageCount = pageCount;
    dPageIndex = 0;
    dState = TD_TRANSITION_IN;
    dTransitionTime = 0;
    dCompletedFlag = false;
    if (pageCount > 0) {
        wrapPage(pages[0]);
    }
}

bool isTutorialDialogActive(void) {
    return dState != TD_HIDDEN;
}

bool isTutorialDialogComplete(void) {
    return dCompletedFlag;
}

void updateTutorialDialog(float deltaTime) {
    if (dState == TD_HIDDEN) {
        return;
    }

    if (dState == TD_TRANSITION_IN) {
        dTransitionTime += deltaTime;
        if (dTransitionTime >= TD_IN_DURATION) {
            dTransitionTime = TD_IN_DURATION;
            dState = TD_TYPING;
        }
        return;
    }

    if (dState == TD_TRANSITION_OUT) {
        dTransitionTime += deltaTime;
        if (dTransitionTime >= TD_OUT_DURATION) {
            dState = TD_HIDDEN;
            dCompletedFlag = true;
        }
        return;
    }

    if (dState == TD_TYPING) {
        dCharAccumulator += deltaTime;
        while (dCharAccumulator >= TD_CHAR_TIME && dRevealedChars < dPageTotalChars) {
            dCharAccumulator -= TD_CHAR_TIME;
            dRevealedChars++;
        }
        if (dRevealedChars >= dPageTotalChars) {
            dState = TD_AWAIT_ADVANCE;
        }
    }

    PDButtons pushed;
    pd->system->getButtonState(NULL, &pushed, NULL);
    if (pushed & kButtonA) {
        if (dState == TD_TYPING) {
            dRevealedChars = dPageTotalChars;
            dState = TD_AWAIT_ADVANCE;
        } else if (dState == TD_AWAIT_ADVANCE) {
            dPageIndex++;
            if (dPageIndex >= dPageCount) {
                dState = TD_TRANSITION_OUT;
                dTransitionTime = 0;
            } else {
                wrapPage(dPages[dPageIndex]);
                dState = TD_TYPING;
            }
        }
    }
}

void drawTutorialDialog(void) {
    if (dState == TD_HIDDEN) {
        return;
    }

    int boxY;
    if (dState == TD_TRANSITION_IN) {
        float t = easeOutCubic(dTransitionTime / TD_IN_DURATION);
        boxY = (int)lerp((float)SCREEN_HEIGHT, (float)TD_BOX_Y_BASE, t);
    } else if (dState == TD_TRANSITION_OUT) {
        float t = easeInCubic(dTransitionTime / TD_OUT_DURATION);
        boxY = (int)lerp((float)TD_BOX_Y_BASE, (float)SCREEN_HEIGHT, t);
    } else {
        boxY = TD_BOX_Y_BASE;
    }

    pd->graphics->fillRect(TD_BOX_X, boxY, TD_BOX_W, TD_BOX_HEIGHT, kColorBlack);
    pd->graphics->drawRect(TD_BOX_X, boxY, TD_BOX_W, TD_BOX_HEIGHT, kColorWhite);

    if (dState == TD_TRANSITION_IN || dState == TD_TRANSITION_OUT) {
        return;
    }

    pd->graphics->setFont(fontSubEN);
    int fontHeight = pd->graphics->getFontHeight(fontSubEN);
    int textY = boxY + TD_BOX_PADDING_Y;
    int consumed = 0;
    for (int i = 0; i < dLineCount; i++) {
        int lineLen = dLineCharCounts[i];
        int charsThisLine = dRevealedChars - consumed;
        if (charsThisLine > lineLen) {
            charsThisLine = lineLen;
        }
        if (charsThisLine > 0) {
            int byteLen = utf8ByteOffsetForChar(dLines[i], charsThisLine);
            pd->graphics->drawText(dLines[i], byteLen, kUTF8Encoding, TD_TEXT_X, textY + i * (fontHeight + TD_LINE_LEADING));
        }
        consumed += lineLen;
        if (consumed >= dRevealedChars) {
            break;
        }
    }

    if (dState == TD_AWAIT_ADVANCE) {
        LCDBitmap *icon = BITMAPS[BITMAP_SPELL_A_BUTTON_ICON];
        int iw, ih;
        pd->graphics->getBitmapData(icon, &iw, &ih, NULL, NULL, NULL);
        int iconX = TD_BOX_X + TD_BOX_W - iw - 4;
        int iconY = boxY + TD_BOX_HEIGHT - ih - 3;
        pd->graphics->drawBitmap(icon, iconX, iconY, kBitmapUnflipped);
    }
}
