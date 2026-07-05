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
#define ROW_X            4
#define ROW_WIDTH        192
#define CONTROL_RIGHT_X  190
#define BAR_WIDTH        44
#define BAR_HEIGHT       9

// Settings scrollbar — position indicator shown when a tab's list exceeds VISIBLE_ROWS.
// Not selectable; the crank scrolls the list. When present, controls and the selection
// outline shift left by SETTINGS_SCROLLBAR_GAP to make room.
#define SETTINGS_SCROLLBAR_GAP      5
#define SETTINGS_SCROLLBAR_X        193
#define SETTINGS_SCROLLBAR_W        5
#define SETTINGS_SCROLLBAR_Y        19
#define SETTINGS_SCROLLBAR_TRACK_H  100
#define SETTINGS_SCROLL_SENSITIVITY 0.02f

// Confirmation dialog layout
#define DIALOG_WIDTH 170
#define DIALOG_HEIGHT 88
#define DIALOG_X ((SCREEN_WIDTH - DIALOG_WIDTH) / 2)
#define DIALOG_Y ((SCREEN_HEIGHT - DIALOG_HEIGHT) / 2)

#define HOLD_DELAY  0.3f
#define HOLD_REPEAT 0.2f

typedef enum SettingControlType {
    CONTROL_LANGUAGE,
    CONTROL_CHECKBOX,
    CONTROL_BAR,
    CONTROL_RESET_BUTTON
} SettingControlType;

typedef struct SettingRowDef {
    StringKey nameKey;
    SettingControlType type;
    void *value; // bool* for CONTROL_CHECKBOX, int* for CONTROL_BAR
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
    { STR_SETTING_ABSOLUTE_CRANK, CONTROL_CHECKBOX, &SETTINGS.absoluteCrank },
    { STR_SETTING_INVERT_CRANK, CONTROL_CHECKBOX, &SETTINGS.invertCrank },
    { STR_SETTING_DPAD_TURN_SPEED, CONTROL_BAR, &SETTINGS.dpadTurnSpeed },
    { STR_SETTING_DPAD_STRAFE, CONTROL_CHECKBOX, &SETTINGS.dpadStrafe },
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
    [MENU_TAB_CONTROLS] = { CONTROLS_ROWS, 8, resetControlSettings },
    [MENU_TAB_AUDIO]    = { AUDIO_ROWS, 3, resetAudioSettings },
};

static PDMenuItem *menuItem;

static MenuTab activeTab;
static MenuTab selectedTab;
static MenuFocus focus;
static int listSelRow;
static int listScroll;
static int exitSelRow;
static int confirmSel;

static PDButtons adjHeldDir;
static float adjHoldTimer;
static float settingsScrollAccum;

static void createCloseMenuItem(void);

static bool isSettingsTab(MenuTab tab) {
    return tab == MENU_TAB_GENERAL || tab == MENU_TAB_CONTROLS || tab == MENU_TAB_AUDIO;
}

static void closeGameMenu(void) {
    saveGame();
    suppressHeldPlayerButtons();
    setGameState(GAME_STATE_ACTIVE);
    createGameMenuItem();
}

static void enterTabSelect(void) {
    focus = FOCUS_TABS;
    selectedTab = activeTab;
}

static void resetContentState(void) {
    listSelRow = 0;
    listScroll = 0;
    exitSelRow = 0;
    adjHeldDir = 0;
    adjHoldTimer = 0.0f;
    settingsScrollAccum = 0.0f;
}

static void enterContent(void) {
    focus = FOCUS_CONTENT;
    resetContentState();
}

static void ensureRowVisible(void) {
    if (listSelRow < listScroll) {
        listScroll = listSelRow;
    } else if (listSelRow >= listScroll + VISIBLE_ROWS) {
        listScroll = listSelRow - VISIBLE_ROWS + 1;
    }
}

static void stepBarSetting(int *value, PDButtons dir) {
    int newValue = *value;
    if (dir & kButtonLeft) {
        newValue--;
    } else if (dir & kButtonRight) {
        newValue++;
    }
    if (newValue < 0) {
        newValue = 0;
    } else if (newValue > SETTING_BAR_MAX_LEVEL) {
        newValue = SETTING_BAR_MAX_LEVEL;
    }
    if (newValue != *value) {
        *value = newValue;
        saveGame();
    }
}

static void handleSettingsRowInput(const SettingRowDef *row, PDButtons current, PDButtons pushed, float deltaTime) {
    PDButtons dirPushed = pushed & (kButtonLeft | kButtonRight);
    PDButtons dirCurrent = current & (kButtonLeft | kButtonRight);

    switch (row->type) {
        case CONTROL_CHECKBOX: {
            if (dirPushed || (pushed & kButtonA)) {
                bool *value = (bool *)row->value;
                *value = !*value;
                saveGame();
            }
            break;
        }
        case CONTROL_LANGUAGE: {
            if (dirPushed || (pushed & kButtonA)) {
                SETTINGS.language = SETTINGS.language == LANGUAGE_EN ? LANGUAGE_JP : LANGUAGE_EN;
                saveGame();
            }
            break;
        }
        case CONTROL_BAR: {
            if (dirPushed) {
                adjHeldDir = dirPushed;
                adjHoldTimer = 0.0f;
                stepBarSetting((int *)row->value, dirPushed);
            } else if (dirCurrent && dirCurrent == adjHeldDir) {
                float prevTimer = adjHoldTimer;
                adjHoldTimer += deltaTime;
                if (adjHoldTimer >= HOLD_DELAY) {
                    float phase = adjHoldTimer - HOLD_DELAY;
                    float prevPhase = prevTimer >= HOLD_DELAY ? prevTimer - HOLD_DELAY : 0.0f;
                    if ((int)(phase / HOLD_REPEAT) > (int)(prevPhase / HOLD_REPEAT)) {
                        stepBarSetting((int *)row->value, adjHeldDir);
                    }
                }
            } else {
                adjHeldDir = 0;
                adjHoldTimer = 0.0f;
            }
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
            pd->graphics->setClipRect(x - 2, 1, TAB_WIDTH + 4, isActive ? 14 : 15);
            pd->graphics->drawRoundRect(x - 2, 1, TAB_WIDTH + 4, 22, 3, 3, kColorWhite);
            pd->graphics->clearClipRect();
        } else if (isActive) {
            pd->graphics->setClipRect(x - 1, 2, TAB_WIDTH + 2, 13);
            pd->graphics->drawRoundRect(x - 1, 2, TAB_WIDTH + 2, 20, 2, 1, kColorWhite);
            pd->graphics->clearClipRect();
        } else {
            pd->graphics->drawRoundRect(x, 3, TAB_WIDTH, 13, 2, 1, kColorWhite);
        }

        pd->graphics->drawBitmap(BITMAPS[TAB_ICONS[i]], x + 1, 4, kBitmapUnflipped);
    }
}

static void drawSettingsTab(const SettingsTabDef *tab) {
    LCDFont *font = SETTINGS.language == LANGUAGE_JP ? fontSubJP : fontSubEN;
    int tracking = SETTINGS.language == LANGUAGE_JP ? 1 : -2;
    pd->graphics->setFont(font);
    pd->graphics->setTextTracking(tracking);
    pd->graphics->setDrawMode(kDrawModeCopy);
    int fontH = pd->graphics->getFontHeight(font);

    bool hasScrollbar = tab->rowCount > VISIBLE_ROWS;
    int controlRightX = hasScrollbar ? CONTROL_RIGHT_X - SETTINGS_SCROLLBAR_GAP : CONTROL_RIGHT_X;
    int rowWidth = hasScrollbar ? ROW_WIDTH - SETTINGS_SCROLLBAR_GAP : ROW_WIDTH;

    // Draw one row past VISIBLE_ROWS so a partially-visible row still peeks in at the
    // bottom (scroll/selection logic keeps the selected row fully on screen).
    for (int i = 0; i <= VISIBLE_ROWS && listScroll + i < tab->rowCount; i++) {
        int rowIndex = listScroll + i;
        const SettingRowDef *row = &tab->rows[rowIndex];
        int rowY = LIST_TOP + i * ROW_STRIDE;
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
            pd->graphics->drawText(label, labelLen, kUTF8Encoding, btnX + 8, textY);
            if (rowFocused) {
                pd->graphics->drawRoundRect(btnX, rowY - 1, btnW, ROW_HEIGHT + 1, 3, 2, kColorWhite);
            } else {
                pd->graphics->drawRoundRect(btnX + 1, rowY + 1, btnW - 2, ROW_HEIGHT - 2, 3, 1, kColorWhite);
            }
            continue;
        }

        const char *name = getLocString(row->nameKey);
        pd->graphics->drawText(name, strlen(name), kUTF8Encoding, 10, textY);

        if (row->type == CONTROL_CHECKBOX) {
            bool value = *(bool *)row->value;
            int boxX = controlRightX - 9;
            int boxY = rowY + (ROW_HEIGHT - 9) / 2;
            pd->graphics->drawRect(boxX, boxY, 9, 9, kColorWhite);
            if (value) {
                pd->graphics->fillRect(boxX + 2, boxY + 2, 5, 5, kColorWhite);
            }
        } else if (row->type == CONTROL_BAR) {
            int level = *(int *)row->value;
            int barX = controlRightX - BAR_WIDTH;
            int barY = rowY + (ROW_HEIGHT - BAR_HEIGHT) / 2;
            pd->graphics->drawRect(barX, barY, BAR_WIDTH, BAR_HEIGHT, kColorWhite);
            if (level > 0) {
                pd->graphics->fillRect(barX + 1, barY + 1, level * 7, BAR_HEIGHT - 2, kColorWhite);
            }
            for (int s = 1; s < SETTING_BAR_MAX_LEVEL; s++) {
                pd->graphics->fillRect(barX + s * 7, barY + 1, 1, BAR_HEIGHT - 2, kColorBlack);
            }
        } else if (row->type == CONTROL_LANGUAGE) {
            pd->graphics->pushContext(NULL);
            pd->graphics->setFont(fontSubEN);
            const char *value = SETTINGS.language == LANGUAGE_JP ? "JP" : "EN";
            int valueLen = strlen(value);
            int valueW = pd->graphics->getTextWidth(fontSubEN, value, valueLen, kUTF8Encoding, tracking);
            pd->graphics->drawText(value, valueLen, kUTF8Encoding, controlRightX - valueW, textY);
            pd->graphics->popContext();
        }

        if (rowFocused) {
            pd->graphics->drawRoundRect(ROW_X, rowY - 1, rowWidth, ROW_HEIGHT + 1, 3, 1, kColorWhite);
        }
    }

    if (hasScrollbar) {
        int thumbH = SETTINGS_SCROLLBAR_TRACK_H * VISIBLE_ROWS / tab->rowCount;
        int thumbY = SETTINGS_SCROLLBAR_Y + SETTINGS_SCROLLBAR_TRACK_H * listScroll / tab->rowCount;
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
        int textY = buttonY[i] + (buttonH - fontH) / 2;
        pd->graphics->drawText(label, labelLen, kUTF8Encoding, btnX + 12, textY);
        if (focus != FOCUS_TABS && exitSelRow == i) {
            pd->graphics->drawRoundRect(btnX, buttonY[i], btnW, buttonH, 3, 2, kColorWhite);
        } else {
            pd->graphics->drawRoundRect(btnX + 1, buttonY[i] + 1, btnW - 2, buttonH - 2, 3, 1, kColorWhite);
        }
    }
}

static void drawConfirmDialog(void) {
    pd->graphics->fillRoundRect(DIALOG_X - 3, DIALOG_Y - 3, DIALOG_WIDTH + 6, DIALOG_HEIGHT + 6, 3, kColorBlack);
    pd->graphics->fillRect(DIALOG_X, DIALOG_Y, DIALOG_WIDTH, DIALOG_HEIGHT, kColorBlack);
    pd->graphics->drawRoundRect(DIALOG_X, DIALOG_Y, DIALOG_WIDTH, DIALOG_HEIGHT, 3, 2, kColorWhite);

    LCDFont *titleFont = SETTINGS.language == LANGUAGE_JP ? fontTitleJP : fontTitleEN;
    LCDFont *subFont = SETTINGS.language == LANGUAGE_JP ? fontSubJP : fontSubEN;
    int titleTracking = SETTINGS.language == LANGUAGE_JP ? 0 : -1;
    int subTracking = SETTINGS.language == LANGUAGE_JP ? 1 : -2;
    int subLeading = SETTINGS.language == LANGUAGE_JP ? 2 : 0;

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
    int bodyMaxWidth = DIALOG_WIDTH - 24;
    pd->graphics->setFont(subFont);
    pd->graphics->setTextTracking(subTracking);
    pd->graphics->setTextLeading(subLeading);
    int bodyHeight = pd->graphics->getTextHeightForMaxWidth(subFont, body, bodyLen, bodyMaxWidth, kUTF8Encoding, kWrapWord, subTracking, subLeading);
    int bodyY = headerY + titleHeight + 10;
    pd->graphics->drawTextInRect(body, bodyLen, kUTF8Encoding, DIALOG_X + 12, bodyY, bodyMaxWidth, bodyHeight, kWrapWord, kAlignTextCenter);

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

    int yesX = buttonsStartX;
    int noX = buttonsStartX + yesWidth + 28;

    pd->graphics->drawText(yesText, yesLen, kUTF8Encoding, yesX, buttonsY);
    pd->graphics->drawText(noText, noLen, kUTF8Encoding, noX, buttonsY);

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
        if (pushed & kButtonLeft) {
            selectedTab = (selectedTab + MENU_TAB_COUNT - 1) % MENU_TAB_COUNT;
            activeTab = selectedTab;
            resetContentState();
        } else if (pushed & kButtonRight) {
            selectedTab = (selectedTab + 1) % MENU_TAB_COUNT;
            activeTab = selectedTab;
            resetContentState();
        }
        if (pushed & (kButtonA | kButtonDown)) {
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
        if (pushed & kButtonB) {
            enterTabSelect();
        } else if (pushed & kButtonUp) {
            if (listSelRow == 0) {
                enterTabSelect();
            } else {
                listSelRow--;
                ensureRowVisible();
            }
        } else if (pushed & kButtonDown) {
            if (listSelRow < tab->rowCount - 1) {
                listSelRow++;
                ensureRowVisible();
            }
        } else {
            handleSettingsRowInput(&tab->rows[listSelRow], current, pushed, deltaTime);
        }

        int maxScroll = tab->rowCount - VISIBLE_ROWS;
        if (maxScroll > 0) {
            float crankChange = pd->system->getCrankChange();
            if (crankChange != 0) {
                settingsScrollAccum += crankChange * SETTINGS_SCROLL_SENSITIVITY;
                int step = (int)settingsScrollAccum;
                settingsScrollAccum -= step;
                listScroll += step;
                if (listScroll < 0) {
                    listScroll = 0;
                } else if (listScroll > maxScroll) {
                    listScroll = maxScroll;
                }
            }
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
