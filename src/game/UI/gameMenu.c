#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/localization.h"
#include "globals/saveSystem.h"
#include "game/gameScene.h"
#include "game/UI/gameMenu.h"
#include "game/UI/runeInventory.h"
#include "game/UI/quitConfirmDialog.h"
#include "game/player/player.h"
#include "game/rendering/textures.h"
#include "game/settings/settings.h"

// Tab bar layout
#define TAB_BASELINE_Y   15
#define TAB_WIDTH        23

typedef enum MenuTab {
    MENU_TAB_INVENTORY,
    MENU_TAB_GENERAL,
    MENU_TAB_CONTROLS,
    MENU_TAB_AUDIO,
    MENU_TAB_EXIT,
    MENU_TAB_COUNT
} MenuTab;

static const int TAB_X[MENU_TAB_COUNT] = { 4, 32, 60, 88, 173 };
static const int TAB_ICONS[MENU_TAB_COUNT] = {
    BITMAP_MENU_INVENTORY_ICON,
    BITMAP_MENU_GEAR_ICON,
    BITMAP_MENU_INPUT_ICON,
    BITMAP_MENU_AUDIO_ICON,
    BITMAP_MENU_EXIT_ICON,
};

typedef enum MenuFocus {
    FOCUS_TABS,
    FOCUS_CONTENT,
    FOCUS_SCROLLBAR,
    FOCUS_CONFIRM
} MenuFocus;

// Settings list layout
#define LIST_TOP         20
#define ROW_HEIGHT       16
#define ROW_GAP          2     // vertical space between list rows
#define ROW_STRIDE       (ROW_HEIGHT + ROW_GAP)
#define VISIBLE_ROWS     ((SCREEN_HEIGHT - LIST_TOP) / ROW_STRIDE)  // rows that fit fully on screen
#define LIST_CLIP_TOP    (TAB_BASELINE_Y + 1)  // rows scroll under the tab bar; clip above this
#define ROW_X            4
#define ROW_WIDTH        192
#define CONTROL_RIGHT_X  190
#define BAR_WIDTH        44

// Settings scrollbar — position indicator shown when a tab's list exceeds VISIBLE_ROWS.
// Not selectable; the crank scrolls the list. When present, controls and the selection
// outline shift left by SETTINGS_SCROLLBAR_GAP to make room.
#define SETTINGS_SCROLLBAR_GAP      5
#define SETTINGS_SCROLLBAR_X        193
#define SETTINGS_SCROLLBAR_W        5
#define SETTINGS_SCROLLBAR_Y        19
#define SETTINGS_SCROLLBAR_TRACK_H  100
#define SETTINGS_SCROLL_SENSITIVITY 0.4f   // pixels of scroll per crank degree
#define SETTINGS_SCROLL_LERP        0.4f   // per-frame approach toward the scroll target
#define SETTINGS_INT_CRANK_SENSITIVITY 1.0f  // int units per crank degree (a full turn ≈ 360)

// Confirmation dialog layout
#define DIALOG_WIDTH 170
#define DIALOG_HEIGHT 88
#define DIALOG_X ((SCREEN_WIDTH - DIALOG_WIDTH) / 2)
#define DIALOG_Y ((SCREEN_HEIGHT - DIALOG_HEIGHT) / 2)

#define HOLD_DELAY  0.2f
#define HOLD_REPEAT 0.1f

// CONTROL_INT held-adjust acceleration: the repeat interval ramps from _START down
// to _MIN over _ACCEL_TIME seconds of continuous holding.
#define INT_ADJ_REPEAT_START 0.1f
#define INT_ADJ_REPEAT_MIN   0.02f
#define INT_ADJ_ACCEL_TIME   1.0f

typedef enum SettingControlType {
    CONTROL_LANGUAGE,
    CONTROL_CHECKBOX,
    CONTROL_BAR,
    CONTROL_INT,
    CONTROL_RESET_BUTTON
} SettingControlType;

typedef struct SettingRowDef {
    StringKey nameKey;
    SettingControlType type;
    void *value; // bool* for CONTROL_CHECKBOX, int* for CONTROL_BAR / CONTROL_INT
    int minValue; // CONTROL_INT only
    int maxValue; // CONTROL_INT only
} SettingRowDef;

typedef struct SettingsTabDef {
    const SettingRowDef *rows;
    int rowCount;
    void (*resetFunc)(void);
} SettingsTabDef;

static const SettingRowDef GENERAL_ROWS[] = {
    { STR_SETTING_LANGUAGE, CONTROL_LANGUAGE, NULL },
    { STR_SETTING_DRAW_FPS, CONTROL_CHECKBOX, &SETTINGS.drawFPS },
    { STR_SETTING_RESET, CONTROL_RESET_BUTTON, NULL },
};

static const SettingRowDef CONTROLS_ROWS[] = {
    { STR_SETTING_CRANK_TURN_SPEED, CONTROL_BAR, &SETTINGS.crankTurnSpeed },
    { STR_SETTING_DPAD_STRAFE, CONTROL_CHECKBOX, &SETTINGS.dpadStrafe },
    { STR_SETTING_ABSOLUTE_CRANK, CONTROL_CHECKBOX, &SETTINGS.absoluteCrank },
    { STR_SETTING_ABSOLUTE_CRANK_OFFSET, CONTROL_INT, &SETTINGS.absoluteCrankOffset, SETTING_CRANK_OFFSET_MIN, SETTING_CRANK_OFFSET_MAX },
    { STR_SETTING_INVERT_CRANK, CONTROL_CHECKBOX, &SETTINGS.invertCrank },
    { STR_SETTING_DPAD_TURN_SPEED, CONTROL_BAR, &SETTINGS.dpadTurnSpeed },
    { STR_SETTING_INVERT_DPAD, CONTROL_CHECKBOX, &SETTINGS.invertDpad },
    { STR_SETTING_SWAP_SPELL_BUTTONS, CONTROL_CHECKBOX, &SETTINGS.spellButtonsReversed },
    { STR_SETTING_RESET, CONTROL_RESET_BUTTON, NULL },
};

static const SettingRowDef AUDIO_ROWS[] = {
    { STR_SETTING_MUSIC_VOLUME, CONTROL_BAR, &SETTINGS.musicVolume },
    { STR_SETTING_SFX_VOLUME, CONTROL_BAR, &SETTINGS.sfxVolume },
    { STR_SETTING_RESET, CONTROL_RESET_BUTTON, NULL },
};

static const SettingsTabDef SETTINGS_TABS[] = {
    [MENU_TAB_GENERAL]  = { GENERAL_ROWS, 3, resetGeneralSettings },
    [MENU_TAB_CONTROLS] = { CONTROLS_ROWS, 9, resetControlSettings },
    [MENU_TAB_AUDIO]    = { AUDIO_ROWS, 3, resetAudioSettings },
};

static PDMenuItem *menuItem;

static MenuTab activeTab;
static MenuTab selectedTab;
static MenuFocus focus;
static int listSelRow;
static int exitSelRow;
static int confirmSel;

static float scrollPx;        // current animated list scroll offset in pixels
static float scrollTargetPx;  // where the list is scrolling toward

static PDButtons adjHeldDir;
static float adjHoldTimer;
static float adjHoldElapsed;   // time held past HOLD_DELAY, drives repeat acceleration
static float adjRepeatTimer;   // counts down to the next accelerated repeat step
static float intCrankAccum;    // fractional crank degrees pending for the selected int

static PDButtons navHeldDir;
static float navHoldTimer;

static bool settingsDirty;     // a setting changed but hasn't been persisted yet

static void createCloseMenuItem(void);

static void markSettingsDirty(void) {
    settingsDirty = true;
}

// Persist deferred setting changes. Continuous edits (crank / accelerated hold) mark
// dirty and flush once the interaction goes idle, rather than saving every frame.
static void flushSettingsSave(void) {
    if (settingsDirty) {
        saveGame();
        settingsDirty = false;
    }
}

static bool isSettingsTab(MenuTab tab) {
    return tab == MENU_TAB_GENERAL || tab == MENU_TAB_CONTROLS || tab == MENU_TAB_AUDIO;
}

static void closeGameMenu(void) {
    saveGame();
    settingsDirty = false;
    suppressHeldPlayerButtons();
    setGameState(GAME_STATE_ACTIVE);
    createGameMenuItem();
}

static void enterTabSelect(void) {
    flushSettingsSave();
    focus = FOCUS_TABS;
    selectedTab = activeTab;
}

static void resetContentState(void) {
    listSelRow = 0;
    exitSelRow = 0;
    adjHeldDir = 0;
    adjHoldTimer = 0.0f;
    adjHoldElapsed = 0.0f;
    adjRepeatTimer = 0.0f;
    intCrankAccum = 0.0f;
    scrollPx = 0.0f;
    scrollTargetPx = 0.0f;
}

static void enterContent(void) {
    focus = FOCUS_CONTENT;
    resetContentState();
}

static float settingsMaxScrollPx(const SettingsTabDef *tab) {
    int extraRows = tab->rowCount - VISIBLE_ROWS;
    return extraRows > 0 ? (float)(extraRows * ROW_STRIDE) : 0.0f;
}

// Nudge the scroll target so the selected row sits fully within the visible band.
// Leaves the target untouched (preserving a free crank offset) while the row is visible.
static void ensureRowVisible(void) {
    float selTop = (float)(listSelRow * ROW_STRIDE);
    float bandBottom = (float)((VISIBLE_ROWS - 1) * ROW_STRIDE);
    if (selTop < scrollTargetPx) {
        scrollTargetPx = selTop;
    } else if (selTop > scrollTargetPx + bandBottom) {
        scrollTargetPx = selTop - bandBottom;
    }
}

// Enter a settings tab's list with the selection on the last row, snapped into view.
// Used when wrapping into the list from the tab bar with Up.
static void enterContentAtBottom(void) {
    enterContent();
    listSelRow = SETTINGS_TABS[activeTab].rowCount - 1;
    ensureRowVisible();
    scrollPx = scrollTargetPx;
}

static bool stepIntSetting(int *value, int delta, int minValue, int maxValue, bool wrap) {
    int newValue = *value + delta;
    if (wrap) {
        int range = maxValue - minValue + 1;
        newValue = minValue + (((newValue - minValue) % range) + range) % range;
    } else if (newValue < minValue) {
        newValue = minValue;
    } else if (newValue > maxValue) {
        newValue = maxValue;
    }
    if (newValue != *value) {
        *value = newValue;
        return true;
    }
    return false;
}

// Returns the direction within dirMask that should move the selection this frame,
// firing once on press and then repeating while held (HOLD_DELAY, HOLD_REPEAT).
// Used for both the settings list (up/down) and the tab bar (left/right).
static PDButtons navRepeat(PDButtons dirMask, PDButtons current, PDButtons pushed, float deltaTime) {
    PDButtons dirPushed = pushed & dirMask;
    PDButtons dirCurrent = current & dirMask;
    if (dirPushed) {
        navHeldDir = dirPushed;
        navHoldTimer = 0.0f;
        return dirPushed;
    }
    if (dirCurrent && dirCurrent == navHeldDir) {
        float prevTimer = navHoldTimer;
        navHoldTimer += deltaTime;
        if (navHoldTimer >= HOLD_DELAY) {
            float phase = navHoldTimer - HOLD_DELAY;
            float prevPhase = prevTimer >= HOLD_DELAY ? prevTimer - HOLD_DELAY : 0.0f;
            if ((int)(phase / HOLD_REPEAT) > (int)(prevPhase / HOLD_REPEAT)) {
                return navHeldDir;
            }
        }
        return 0;
    }
    navHeldDir = 0;
    navHoldTimer = 0.0f;
    return 0;
}

static void toggleLanguage(void) {
    SETTINGS.language = SETTINGS.language == LANGUAGE_EN ? LANGUAGE_JP : LANGUAGE_EN;
    saveGame();
}

static void adjustInt(int *value, int minValue, int maxValue, bool wrap, bool accelerate,
                      PDButtons current, PDButtons pushed, float deltaTime) {
    PDButtons dirPushed = pushed & (kButtonLeft | kButtonRight);
    PDButtons dirCurrent = current & (kButtonLeft | kButtonRight);
    if (dirPushed) {
        adjHeldDir = dirPushed;
        adjHoldTimer = 0.0f;
        adjHoldElapsed = 0.0f;
        adjRepeatTimer = 0.0f;
        int step = (dirPushed & kButtonLeft) ? -1 : 1;
        if (stepIntSetting(value, step, minValue, maxValue, wrap)) {
            markSettingsDirty();
        }
    } else if (dirCurrent && dirCurrent == adjHeldDir) {
        adjHoldTimer += deltaTime;
        if (adjHoldTimer >= HOLD_DELAY) {
            adjHoldElapsed += deltaTime;
            adjRepeatTimer -= deltaTime;
            if (adjRepeatTimer <= 0.0f) {
                float interval = HOLD_REPEAT;
                if (accelerate) {
                    float t = adjHoldElapsed / INT_ADJ_ACCEL_TIME;
                    if (t > 1.0f) {
                        t = 1.0f;
                    }
                    interval = INT_ADJ_REPEAT_START + (INT_ADJ_REPEAT_MIN - INT_ADJ_REPEAT_START) * t;
                }
                adjRepeatTimer += interval;
                if (adjRepeatTimer < 0.0f) {
                    adjRepeatTimer = 0.0f;
                }
                int step = (adjHeldDir & kButtonLeft) ? -1 : 1;
                if (stepIntSetting(value, step, minValue, maxValue, wrap)) {
                    markSettingsDirty();
                }
            }
        }
    } else {
        adjHeldDir = 0;
        adjHoldTimer = 0.0f;
        adjHoldElapsed = 0.0f;
        adjRepeatTimer = 0.0f;
    }
}

// While a CONTROL_INT row is selected the crank drives the value (looping) instead of
// scrolling the list.
static void adjustIntByCrank(int *value, int minValue, int maxValue, float crankChange) {
    if (crankChange == 0.0f) {
        return;
    }
    intCrankAccum += crankChange * SETTINGS_INT_CRANK_SENSITIVITY;
    int steps = (int)intCrankAccum;
    if (steps != 0) {
        intCrankAccum -= steps;
        if (stepIntSetting(value, steps, minValue, maxValue, true)) {
            markSettingsDirty();
        }
    }
}

static void handleSettingsRowInput(const SettingRowDef *row, PDButtons current, PDButtons pushed, float deltaTime) {
    switch (row->type) {
        case CONTROL_CHECKBOX: {
            if (pushed & kButtonA) {
                bool *value = (bool *)row->value;
                *value = !*value;
                saveGame();
            }
            break;
        }
        case CONTROL_LANGUAGE: {
            if ((pushed & (kButtonLeft | kButtonRight)) || (pushed & kButtonA)) {
                toggleLanguage();
            }
            break;
        }
        case CONTROL_BAR: {
            adjustInt((int *)row->value, 0, SETTING_BAR_MAX_LEVEL, false, false, current, pushed, deltaTime);
            break;
        }
        case CONTROL_INT: {
            adjustInt((int *)row->value, row->minValue, row->maxValue, true, true, current, pushed, deltaTime);
            break;
        }
        case CONTROL_RESET_BUTTON: {
            if (pushed & kButtonA) {
                confirmSel = 1;
                focus = FOCUS_CONFIRM;
            }
            break;
        }
    }
}

// Left/right adjust arrows for language and int controls: outlined when the row is
// not selected, filled when it is. The source bitmap points left; the right arrow is
// the same bitmap flipped horizontally.
#define SETTING_ARROW_W 5
#define SETTING_ARROW_H 7

static void drawSettingArrows(int leftArrowX, int rightArrowX, int centerY, bool selected) {
    int bitmapIndex = selected ? BITMAP_SETTING_ARROW_FILLED : BITMAP_SETTING_ARROW_OUTLINE;
    int y = centerY - SETTING_ARROW_H / 2;
    pd->graphics->drawBitmap(BITMAPS[bitmapIndex], leftArrowX, y, kBitmapUnflipped);
    pd->graphics->drawBitmap(BITMAPS[bitmapIndex], rightArrowX, y, kBitmapFlippedX);
}

static void drawTabBar(void) {
    pd->graphics->drawLine(0, TAB_BASELINE_Y, SCREEN_WIDTH - 1, TAB_BASELINE_Y, 1, kColorWhite);

    for (int i = 0; i < MENU_TAB_COUNT; i++) {
        int x = TAB_X[i];
        bool isActive = (MenuTab)i == activeTab;
        bool isOutlined = focus == FOCUS_TABS && (MenuTab)i == selectedTab;

        if (isActive) {
            int gapX = isOutlined ? x - 2 : x - 1;
            int gapW = isOutlined ? TAB_WIDTH + 4 : TAB_WIDTH + 2;
            pd->graphics->fillRect(gapX, TAB_BASELINE_Y, gapW, 1, kColorBlack);
        }

        if (isOutlined) {
            pd->graphics->setClipRect(x - 3, 1, TAB_WIDTH + 6, isActive ? 14 : 15);
            pd->graphics->drawRoundRect(x - 2, 2, TAB_WIDTH + 4, 22, 3, 2, kColorWhite);
            pd->graphics->clearClipRect();
        } else if (isActive) {
            pd->graphics->setClipRect(x - 1, 2, TAB_WIDTH + 2, 13);
            pd->graphics->drawRoundRect(x - 1, 2, TAB_WIDTH + 2, 20, 2, 1, kColorWhite);
            pd->graphics->clearClipRect();
        } else {
            pd->graphics->setClipRect(x - 1, 2, TAB_WIDTH + 2, 13);
            pd->graphics->drawRoundRect(x, 3, TAB_WIDTH, 14, 2, 1, kColorWhite);
            pd->graphics->clearClipRect();
        }

        pd->graphics->drawBitmap(BITMAPS[TAB_ICONS[i]], x + 1, 4, kBitmapUnflipped);
    }
}

static void drawSettingsTab(const SettingsTabDef *tab) {
    float scrollDiff = scrollTargetPx - scrollPx;
    scrollPx += scrollDiff * SETTINGS_SCROLL_LERP;
    if (scrollDiff < 0.5f && scrollDiff > -0.5f) {
        scrollPx = scrollTargetPx;
    }
    int scrollOffset = (int)(scrollPx + 0.5f);

    LCDFont *font = SETTINGS.language == LANGUAGE_JP ? fontSubJP : fontSubEN;
    int tracking = SETTINGS.language == LANGUAGE_JP ? 1 : -2;
    pd->graphics->setFont(font);
    pd->graphics->setTextTracking(tracking);
    pd->graphics->setDrawMode(kDrawModeCopy);
    int fontH = pd->graphics->getFontHeight(font);

    bool hasScrollbar = tab->rowCount > VISIBLE_ROWS;
    int controlRightX = hasScrollbar ? CONTROL_RIGHT_X - SETTINGS_SCROLLBAR_GAP : CONTROL_RIGHT_X;
    int rowWidth = hasScrollbar ? ROW_WIDTH - SETTINGS_SCROLLBAR_GAP : ROW_WIDTH;

    // Clip rows to the list region so partially-scrolled rows are cut off cleanly
    // and slide under the tab bar rather than drawing over it.
    pd->graphics->setClipRect(0, LIST_CLIP_TOP, SCREEN_WIDTH, SCREEN_HEIGHT - LIST_CLIP_TOP);

    for (int rowIndex = 0; rowIndex < tab->rowCount; rowIndex++) {
        const SettingRowDef *row = &tab->rows[rowIndex];
        int rowY = LIST_TOP + rowIndex * ROW_STRIDE - scrollOffset;
        if (rowY > SCREEN_HEIGHT || rowY + ROW_HEIGHT < LIST_CLIP_TOP) {
            continue;
        }
        bool rowFocused = focus != FOCUS_TABS && rowIndex == listSelRow;
        int textY = rowY + (ROW_HEIGHT - fontH) / 2;
        if (SETTINGS.language == LANGUAGE_JP) {
            textY -= 1;
        }

        if (row->type == CONTROL_RESET_BUTTON) {
            const char *label = getLocString(row->nameKey);
            int labelLen = strlen(label);
            int labelW = pd->graphics->getTextWidth(font, label, labelLen, kUTF8Encoding, tracking);
            int btnW = labelW + 16;
            int btnX = (SCREEN_WIDTH - btnW) / 2;
            textY += 1;
            pd->graphics->drawText(label, labelLen, kUTF8Encoding, btnX + 8, textY);
            if (rowFocused) {
                pd->graphics->drawRoundRect(btnX + 1, rowY, btnW - 2, ROW_HEIGHT, 3, 2, kColorWhite);
            } else {
                pd->graphics->drawRoundRect(btnX + 1, rowY, btnW - 2, ROW_HEIGHT, 3, 1, kColorWhite);
            }
            continue;
        }

        const char *name = getLocString(row->nameKey);
        pd->graphics->drawText(name, strlen(name), kUTF8Encoding, 10, textY);

        if (row->type == CONTROL_CHECKBOX) {
            bool value = *(bool *)row->value;
            int boxX = controlRightX - 8;
            int boxY = rowY + (ROW_HEIGHT - 9) / 2;  // keep prior top; box shrinks to 8x8
            pd->graphics->drawRect(boxX, boxY, 8, 8, kColorWhite);
            if (value) {
                pd->graphics->fillRect(boxX + 2, boxY + 2, 4, 4, kColorWhite);
            } else if (rowFocused) {
                // Checkered 4x4 fill to signal the box is togglable while selected.
                for (int py = 0; py < 4; py++) {
                    for (int px = 0; px < 4; px++) {
                        if ((px + py) % 2 == 0) {
                            pd->graphics->fillRect(boxX + 2 + px, boxY + 2 + py, 1, 1, kColorWhite);
                        }
                    }
                }
            }
        } else if (row->type == CONTROL_BAR) {
            int level = *(int *)row->value;
            int barX = controlRightX - BAR_WIDTH;
            int centerY = rowY + ROW_HEIGHT / 2 - 1;
            // Seven 2px-wide dots spaced by 5px gaps (7px stride); the center dot
            // is drawn taller to mark the middle. The current level's dot is
            // replaced by the selection indicator, so skip it here.
            for (int i = 0; i <= SETTING_BAR_MAX_LEVEL; i++) {
                if (i == level) {
                    continue;
                }
                int dotH = (i == SETTING_BAR_DEFAULT_LEVEL) ? 4 : 2;
                pd->graphics->fillRect(barX + i * 7, centerY - dotH / 2, 2, dotH, kColorWhite);
            }
            // Level indicator: 4x8 rect centered on the current dot. Filled while
            // the row is focused (adjustable), outlined otherwise.
            int selX = barX + level * 7 - 1;
            int selY = centerY - 4;
            if (rowFocused) {
                pd->graphics->fillRect(selX, selY, 4, 8, kColorWhite);
            } else {
                pd->graphics->drawRect(selX, selY, 4, 8, kColorWhite);
            }
        } else if (row->type == CONTROL_LANGUAGE) {
            const char *value = SETTINGS.language == LANGUAGE_JP ? "日本語" : "English";
            int valueLen = strlen(value);
            int valueW = pd->graphics->getTextWidth(font, value, valueLen, kUTF8Encoding, tracking);

            // Reserve a fixed region sized to the wider label so the arrows stay
            // put regardless of the current language, then center the value in it.
            int enW = pd->graphics->getTextWidth(fontSubEN, "English", 7, kUTF8Encoding, -2);
            int jpW = pd->graphics->getTextWidth(fontSubJP, "日本語", strlen("日本語"), kUTF8Encoding, 1);
            int valueRegionW = (enW > jpW ? enW : jpW);

            const int arrowGap = 4;
            int centerY = textY + fontH / 2;

            int rightArrowLeft = controlRightX - SETTING_ARROW_W;
            int valueRegionRight = rightArrowLeft - arrowGap;
            int valueRegionLeft = valueRegionRight - valueRegionW;
            int leftArrowLeft = valueRegionLeft - arrowGap - SETTING_ARROW_W;
            int valueX = valueRegionLeft + (valueRegionW - valueW) / 2;

            pd->graphics->drawText(value, valueLen, kUTF8Encoding, valueX, textY);

            drawSettingArrows(leftArrowLeft, rightArrowLeft, centerY, rowFocused);
        } else if (row->type == CONTROL_INT) {
            // Numbers always render in the EN sub font. Reserve a fixed region sized
            // to three digits so the arrows hold position, then center the value
            // (drawn without leading zeroes) within it.
            char *valueStr = NULL;
            pd->system->formatString(&valueStr, "%d", *(int *)row->value);
            int valueLen = strlen(valueStr);
            int valueRegionW = pd->graphics->getTextWidth(fontSubEN, "000", 3, kUTF8Encoding, -2);
            int valueW = pd->graphics->getTextWidth(fontSubEN, valueStr, valueLen, kUTF8Encoding, -2);

            const int arrowGap = 4;
            int enFontH = pd->graphics->getFontHeight(fontSubEN);
            int numTextY = rowY + (ROW_HEIGHT - enFontH) / 2;
            int centerY = numTextY + enFontH / 2;

            int rightArrowLeft = controlRightX - SETTING_ARROW_W;
            int valueRegionRight = rightArrowLeft - arrowGap;
            int valueRegionLeft = valueRegionRight - valueRegionW;
            int leftArrowLeft = valueRegionLeft - arrowGap - SETTING_ARROW_W;
            int valueX = valueRegionLeft + (valueRegionW - valueW) / 2;

            pd->graphics->setFont(fontSubEN);
            pd->graphics->setTextTracking(-2);
            pd->graphics->drawText(valueStr, valueLen, kUTF8Encoding, valueX, numTextY);
            pd->graphics->setFont(font);
            pd->graphics->setTextTracking(tracking);

            pd->system->realloc(valueStr, 0);

            drawSettingArrows(leftArrowLeft, rightArrowLeft, centerY, rowFocused);
        }

        if (rowFocused) {
            pd->graphics->drawRoundRect(ROW_X, rowY - 1, rowWidth, ROW_HEIGHT, 3, 1, kColorWhite);
        }
    }

    pd->graphics->clearClipRect();

    if (hasScrollbar) {
        int contentH = tab->rowCount * ROW_STRIDE;
        int thumbH = SETTINGS_SCROLLBAR_TRACK_H * VISIBLE_ROWS / tab->rowCount;
        int thumbY = SETTINGS_SCROLLBAR_Y + (int)(SETTINGS_SCROLLBAR_TRACK_H * scrollPx / contentH + 0.5f);
        pd->graphics->drawRoundRect(SETTINGS_SCROLLBAR_X, SETTINGS_SCROLLBAR_Y, SETTINGS_SCROLLBAR_W, SETTINGS_SCROLLBAR_TRACK_H, 2, 1, kColorWhite);
        pd->graphics->fillRect(SETTINGS_SCROLLBAR_X + 2, thumbY + 3, 1, thumbH - 5, kColorWhite);
    }
}

static void drawExitTab(void) {
    LCDFont *font = SETTINGS.language == LANGUAGE_JP ? fontSubJP : fontSubEN;
    int tracking = SETTINGS.language == LANGUAGE_JP ? 1 : -2;
    pd->graphics->setFont(font);
    pd->graphics->setTextTracking(tracking);
    pd->graphics->setDrawMode(kDrawModeCopy);
    int fontH = pd->graphics->getFontHeight(font);

    const StringKey labels[2] = { STR_EXIT_CLOSE_MENU, STR_EXIT_QUIT_RUN };
    const int buttonY[2] = { 38, 72 };
    const int buttonH = 24;

    for (int i = 0; i < 2; i++) {
        const char *label = getLocString(labels[i]);
        int labelLen = strlen(label);
        int labelW = pd->graphics->getTextWidth(font, label, labelLen, kUTF8Encoding, tracking);
        int btnW = labelW + 24;
        int btnX = (SCREEN_WIDTH - btnW) / 2;
        int textY = buttonY[i] + (buttonH - fontH) / 2 + (SETTINGS.language == LANGUAGE_JP ? 0 : 1);
        pd->graphics->drawText(label, labelLen, kUTF8Encoding, btnX + 12, textY);
        if (focus != FOCUS_TABS && exitSelRow == i) {
            pd->graphics->drawRoundRect(btnX, buttonY[i] + 1, btnW - 2, buttonH - 2, 3, 2, kColorWhite);
        } else {
            pd->graphics->drawRoundRect(btnX, buttonY[i] + 1, btnW - 2, buttonH - 2, 3, 1, kColorWhite);
        }
    }
}

static void drawConfirmDialog(void) {
    pd->graphics->fillRoundRect(DIALOG_X - 3, DIALOG_Y - 3, DIALOG_WIDTH + 6, DIALOG_HEIGHT + 6, 3, kColorBlack);
    pd->graphics->fillRect(DIALOG_X, DIALOG_Y, DIALOG_WIDTH, DIALOG_HEIGHT, kColorBlack);
    pd->graphics->drawRoundRect(DIALOG_X, DIALOG_Y, DIALOG_WIDTH, DIALOG_HEIGHT, 3, 2, kColorWhite);

    bool isJapanese = SETTINGS.language == LANGUAGE_JP;

    LCDFont *titleFont = isJapanese ? fontTitleJP : fontTitleEN;
    LCDFont *subFont = isJapanese ? fontSubJP : fontSubEN;
    int titleTracking = isJapanese ? -3 : -1;
    int subTracking = isJapanese ? 1 : -2;
    int subLeading = isJapanese ? 2 : 0;

    pd->graphics->setDrawMode(kDrawModeCopy);

    const char *header = getLocString(STR_RESET_CONFIRM_TITLE);
    int headerLen = strlen(header);
    pd->graphics->setFont(titleFont);
    pd->graphics->setTextTracking(titleTracking);
    int titleHeight = pd->graphics->getFontHeight(titleFont);
    int headerWidth = pd->graphics->getTextWidth(titleFont, header, headerLen, kUTF8Encoding, titleTracking);
    int headerX = DIALOG_X + (DIALOG_WIDTH - headerWidth) / 2;
    int headerY = DIALOG_Y + 6;
    pd->graphics->drawText(header, headerLen, kUTF8Encoding, headerX, headerY);

    const char *body = getLocString(STR_RESET_CONFIRM_BODY);
    int bodyLen = strlen(body);
    int bodyPadding = 6;
    int bodyMaxWidth = DIALOG_WIDTH - 2 * bodyPadding;
    pd->graphics->setFont(subFont);
    pd->graphics->setTextTracking(subTracking);
    pd->graphics->setTextLeading(subLeading);
    int bodyHeight = pd->graphics->getTextHeightForMaxWidth(subFont, body, bodyLen, bodyMaxWidth, kUTF8Encoding, kWrapWord, subTracking, subLeading);
    int bodyY = headerY + titleHeight + (isJapanese ? 6 : 12);
    pd->graphics->drawTextInRect(body, bodyLen, kUTF8Encoding, DIALOG_X + bodyPadding, bodyY, bodyMaxWidth, bodyHeight, kWrapWord, kAlignTextCenter);

    const char *yesText = getLocString(STR_YES);
    const char *noText = getLocString(STR_NO);
    int yesLen = strlen(yesText);
    int noLen = strlen(noText);
    int yesWidth = pd->graphics->getTextWidth(subFont, yesText, yesLen, kUTF8Encoding, subTracking);
    int noWidth = pd->graphics->getTextWidth(subFont, noText, noLen, kUTF8Encoding, subTracking);
    int subFontHeight = pd->graphics->getFontHeight(subFont);

    int totalButtonsWidth = yesWidth + noWidth + 28;
    int buttonsStartX = DIALOG_X + (DIALOG_WIDTH - totalButtonsWidth) / 2;
    int buttonsY = DIALOG_Y + DIALOG_HEIGHT - subFontHeight - 8;

    int noX = buttonsStartX;
    int yesX = buttonsStartX + noWidth + 28;

    pd->graphics->drawText(noText, noLen, kUTF8Encoding, noX, buttonsY);
    pd->graphics->drawText(yesText, yesLen, kUTF8Encoding, yesX, buttonsY);

    int selX = confirmSel == 0 ? yesX : noX;
    int selW = confirmSel == 0 ? yesWidth : noWidth;
    pd->graphics->drawRect(selX - 5, buttonsY - 3, selW + 9, subFontHeight + 5, kColorWhite);
}

void updateGameMenu(float deltaTime) {
    pd->graphics->fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, kColorBlack);

    PDButtons current, pushed;
    pd->system->getButtonState(&current, &pushed, NULL);

    if (focus == FOCUS_CONFIRM) {
        if (pushed & (kButtonLeft | kButtonRight)) {
            confirmSel = 1 - confirmSel;
        }
        if (pushed & kButtonA) {
            if (confirmSel == 0 && isSettingsTab(activeTab)) {
                SETTINGS_TABS[activeTab].resetFunc();
                saveGame();
            }
            focus = FOCUS_CONTENT;
        } else if (pushed & kButtonB) {
            focus = FOCUS_CONTENT;
        }
    } else if (focus == FOCUS_TABS) {
        PDButtons navDir = navRepeat(kButtonLeft | kButtonRight, current, pushed, deltaTime);
        if (navDir & kButtonLeft) {
            selectedTab = (selectedTab + MENU_TAB_COUNT - 1) % MENU_TAB_COUNT;
            activeTab = selectedTab;
            resetContentState();
        } else if (navDir & kButtonRight) {
            selectedTab = (selectedTab + 1) % MENU_TAB_COUNT;
            activeTab = selectedTab;
            resetContentState();
        }
        if (pushed & kButtonDown) {
            enterContent();
            // Seed the hold state so keeping Down held repeats into the list.
            navHeldDir = kButtonDown;
            navHoldTimer = 0.0f;
        } else if ((pushed & kButtonUp) && isSettingsTab(activeTab)) {
            // Wrap into the list from the bottom, seeding hold state so a held Up
            // keeps repeating upward through the list.
            enterContentAtBottom();
            navHeldDir = kButtonUp;
            navHoldTimer = 0.0f;
        } else if (pushed & kButtonA) {
            enterContent();
        } else if (pushed & kButtonB) {
            closeGameMenu();
            return;
        }
    } else if (activeTab == MENU_TAB_INVENTORY) {
        if (focus == FOCUS_CONTENT) {
            if (pushed & (kButtonB | kButtonUp)) {
                enterTabSelect();
            } else if ((pushed & (kButtonA | kButtonDown)) && runeInventoryHasScrollbar()) {
                focus = FOCUS_SCROLLBAR;
                notifyEnterRuneScrollbar();
            }
        } else if (focus == FOCUS_SCROLLBAR) {
            if (pushed & kButtonB) {
                focus = FOCUS_CONTENT;
            } else if ((pushed & kButtonUp) && runeInventoryAtScrollTop()) {
                focus = FOCUS_CONTENT;
            }
        }
    } else if (activeTab == MENU_TAB_EXIT) {
        if (pushed & kButtonB) {
            enterTabSelect();
        } else if (pushed & kButtonUp) {
            if (exitSelRow == 0) {
                enterTabSelect();
            } else {
                exitSelRow--;
            }
        } else if (pushed & kButtonDown) {
            if (exitSelRow < 1) {
                exitSelRow++;
            }
        } else if (pushed & kButtonA) {
            if (exitSelRow == 0) {
                closeGameMenu();
                return;
            }
            initQuitConfirmDialog();
        }
    } else {
        const SettingsTabDef *tab = &SETTINGS_TABS[activeTab];
        PDButtons navDir = navRepeat(kButtonUp | kButtonDown, current, pushed, deltaTime);
        if (pushed & kButtonB) {
            enterTabSelect();
        } else if (navDir & kButtonUp) {
            if (listSelRow > 0) {
                listSelRow--;
                intCrankAccum = 0.0f;
                ensureRowVisible();
            } else if (pushed & kButtonUp) {
                enterTabSelect();
            }
        } else if (navDir & kButtonDown) {
            if (listSelRow < tab->rowCount - 1) {
                listSelRow++;
                intCrankAccum = 0.0f;
                ensureRowVisible();
            } else if (pushed & kButtonDown) {
                enterTabSelect();
            }
        } else {
            handleSettingsRowInput(&tab->rows[listSelRow], current, pushed, deltaTime);
        }

        float crankChange = pd->system->getCrankChange();
        const SettingRowDef *selRow = &tab->rows[listSelRow];
        if (selRow->type == CONTROL_INT) {
            adjustIntByCrank((int *)selRow->value, selRow->minValue, selRow->maxValue, crankChange);
        } else {
            float maxScrollPx = settingsMaxScrollPx(tab);
            if (maxScrollPx > 0.0f && crankChange != 0) {
                scrollTargetPx += crankChange * SETTINGS_SCROLL_SENSITIVITY;
                if (scrollTargetPx < 0.0f) {
                    scrollTargetPx = 0.0f;
                } else if (scrollTargetPx > maxScrollPx) {
                    scrollTargetPx = maxScrollPx;
                }
            }
        }

        if (settingsDirty && adjHeldDir == 0 && crankChange == 0.0f) {
            flushSettingsSave();
        }
    }

    if (activeTab == MENU_TAB_INVENTORY) {
        RuneInventoryFocus runeFocus = RUNE_INV_FOCUS_NONE;
        if (focus == FOCUS_CONTENT) {
            runeFocus = RUNE_INV_FOCUS_RUNES;
        } else if (focus == FOCUS_SCROLLBAR) {
            runeFocus = RUNE_INV_FOCUS_SCROLLBAR;
        }
        updateRuneInventoryTab(deltaTime, runeFocus);
    } else if (activeTab == MENU_TAB_EXIT) {
        drawExitTab();
    } else {
        drawSettingsTab(&SETTINGS_TABS[activeTab]);
    }

    drawTabBar();

    if (focus == FOCUS_CONFIRM) {
        drawConfirmDialog();
    }
}

void initGameMenu(void) {
    initRuneInventoryTab();
    activeTab = MENU_TAB_INVENTORY;
    resetContentState();
    enterTabSelect();
    createCloseMenuItem();
    setGameState(GAME_STATE_MENU);
}

static void onOpenMenuItemSelect(void *userdata) {
    if (gameState == GAME_STATE_ACTIVE) {
        initGameMenu();
    }
}

static void onCloseMenuItemSelect(void *userdata) {
    closeGameMenu();
}

void createGameMenuItem(void) {
    if (menuItem != NULL) {
        pd->system->removeMenuItem(menuItem);
    }
    menuItem = pd->system->addMenuItem(getLocString(STR_MENU_MENU), onOpenMenuItemSelect, NULL);
}

static void createCloseMenuItem(void) {
    if (menuItem != NULL) {
        pd->system->removeMenuItem(menuItem);
    }
    menuItem = pd->system->addMenuItem(getLocString(STR_MENU_CLOSE), onCloseMenuItemSelect, NULL);
}

void removeGameMenuItem(void) {
    if (menuItem != NULL) {
        pd->system->removeMenuItem(menuItem);
        menuItem = NULL;
    }
}
