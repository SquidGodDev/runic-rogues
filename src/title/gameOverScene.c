#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/localization.h"
#include "globals/sceneManager.h"

#include "title/gameOverScene.h"
#include "game/rendering/textures.h"
#include "game/UI/runeInventory.h"
#include "game/UI/gameMenu.h"
#include "game/UI/quitConfirmDialog.h"
#include "game/settings/settings.h"
#include "game/systems/runStats.h"
#include "game/systems/progressionManager.h"
#include "game/systems/audioManager.h"

#define GO_HEADER_Y                12
#define GO_STATS_TOP_Y             36
#define GO_STATS_LINE_HEIGHT       12
#define GO_STATS_LABEL_X           16
#define GO_STATS_VALUE_RIGHT_X     184
#define GO_BUTTONS_Y               70
#define GO_BUTTON_VERTICAL_GAP     8
#define GO_BUTTON_PADDING_X        4
#define GO_BUTTON_PADDING_Y        2

#define GO_STAT_COUNT              6
#define GO_BUTTON_COUNT            2

#define HEADER_TRACKING 0
#define BODY_TRACKING   0

static int selectedButton;

static const char *headerText(void) {
    return getLocString(runStatsGetVictory() ? STR_GAMEOVER_VICTORY : STR_GAMEOVER_DEFEATED);
}

static LCDFont *headerFont(void) {
    return (SETTINGS.language == LANGUAGE_JP) ? fontTitleJP : fontTitleEN;
}

static LCDFont *bodyFont(void) {
    return (SETTINGS.language == LANGUAGE_JP) ? fontSubJP : fontSubEN;
}

static const char *statLabel(int index) {
    switch (index) {
        case 0: return getLocString(STR_GAMEOVER_STAT_TIME);
        case 1: return getLocString(STR_GAMEOVER_STAT_LOCATION);
        case 2: return getLocString(STR_GAMEOVER_STAT_DAMAGE_DEALT);
        case 3: return getLocString(STR_GAMEOVER_STAT_DAMAGE_TAKEN);
        case 4: return getLocString(STR_GAMEOVER_STAT_ACCURACY);
        case 5: return getLocString(STR_GAMEOVER_STAT_DODGES);
    }
    return "";
}

static const char *areaName(Area area) {
    switch (area) {
        case AREA_TWILIGHT_COURT:  return getLocString(STR_AREA_TWILIGHT_COURT);
        case AREA_LOST_ARCHIVES:   return getLocString(STR_AREA_LOST_ARCHIVES);
        case AREA_ECHOING_CAVERNS: return getLocString(STR_AREA_ECHOING_CAVERNS);
        case AREA_TEST:            return getLocString(STR_AREA_TEST);
        default: return "";
    }
}

static char *makeTimeString(void) {
    return runStatsMakeTimeString();
}

// Like makeTimeString but with hundredths of a second: "M:SS.cc" (or "H:MM:SS.cc").
static char *makePreciseTimeString(void) {
    float t = runStatsGetTime();
    if (t < 0.0f) {
        t = 0.0f;
    }
    int totalCentis = (int)(t * 100.0f + 0.5f);
    int hours = totalCentis / 360000;
    int minutes = (totalCentis % 360000) / 6000;
    int seconds = (totalCentis % 6000) / 100;
    int centis = totalCentis % 100;
    char *out = NULL;
    if (hours > 0) {
        pd->system->formatString(&out, "%d:%02d:%02d.%02d", hours, minutes, seconds, centis);
    } else {
        pd->system->formatString(&out, "%d:%02d.%02d", minutes, seconds, centis);
    }
    return out;
}

static char *makeLocationString(void) {
    char *out = NULL;
    const char *area = areaName(runStatsGetFinalArea());
    int room = runStatsGetFinalRoom();
    pd->system->formatString(&out, getLocString(STR_GAMEOVER_LOCATION_FORMAT), area, room);
    return out;
}

static char *makeStatValue(int index) {
    char *out = NULL;
    switch (index) {
        case 0: return makeTimeString();
        case 1: return makeLocationString();
        case 2: pd->system->formatString(&out, "%d", runStatsGetDamageDealt()); break;
        case 3: pd->system->formatString(&out, "%d", runStatsGetDamageTaken()); break;
        case 4: {
            int pct = runStatsGetAccuracyPercent();
            if (pct < 0) {
                pd->system->formatString(&out, "-");
            } else {
                pd->system->formatString(&out, "%d%%", pct);
            }
            break;
        }
        case 5: pd->system->formatString(&out, "%d", runStatsGetDodges()); break;
        default: pd->system->formatString(&out, ""); break;
    }
    return out;
}

static const char *buttonText(int index) {
    return getLocString(index == 0 ? STR_GAMEOVER_BUTTON_TRY_AGAIN : STR_GAMEOVER_BUTTON_RETURN_TO_TITLE);
}

void initGameOverScene(void) {
    stopSong();
    removeGameMenuItem();
    freeRuneInventoryCache();

    selectedButton = 0;
}

static void drawHeader(void) {
    const char *text = headerText();
    int byteLen = strlen(text);
    LCDFont *font = headerFont();
    pd->graphics->setFont(font);
    pd->graphics->setTextTracking(0);
    int width = pd->graphics->getTextWidth(font, text, byteLen, kUTF8Encoding, 0);
    int x = (SCREEN_WIDTH - width) / 2;
    pd->graphics->drawText(text, byteLen, kUTF8Encoding, x, GO_HEADER_Y);
}

static void drawStatLine(int index, int y) {
    LCDFont *font = bodyFont();
    pd->graphics->setFont(font);

    const char *label = statLabel(index);
    pd->graphics->drawText(label, strlen(label), kUTF8Encoding, GO_STATS_LABEL_X, y);

    char *value = makeStatValue(index);
    if (value != NULL) {
        int valueWidth = pd->graphics->getTextWidth(font, value, strlen(value), kUTF8Encoding, 0);
        pd->graphics->drawText(value, strlen(value), kUTF8Encoding, GO_STATS_VALUE_RIGHT_X - valueWidth, y);
        pd->system->realloc(value, 0);
    }
}

// Final run time, centered on screen.
static void drawFinalTime(void) {
    char *timeStr = makePreciseTimeString();
    if (timeStr == NULL) {
        return;
    }
    char *label = NULL;
    pd->system->formatString(&label, "Time: %s", timeStr);
    pd->system->realloc(timeStr, 0);
    if (label == NULL) {
        return;
    }
    LCDFont *font = bodyFont();
    pd->graphics->setFont(font);
    pd->graphics->setTextTracking(BODY_TRACKING);
    int len = strlen(label);
    int width = pd->graphics->getTextWidth(font, label, len, kUTF8Encoding, 0);
    int fontH = pd->graphics->getFontHeight(font);
    int x = (SCREEN_WIDTH - width) / 2;
    // Center within the empty band between the header block and the buttons.
    int bandTop = GO_HEADER_Y + pd->graphics->getFontHeight(headerFont());
    int y = bandTop + (GO_BUTTONS_Y - bandTop - fontH) / 2;
    pd->graphics->drawText(label, len, kUTF8Encoding, x, y);
    pd->system->realloc(label, 0);
}

static void drawStats(void) {
    if (true) {
        // For now, skip for later refactor
        return;
    }
    for (int i = 0; i < GO_STAT_COUNT; i++) {
        int y = GO_STATS_TOP_Y + i * GO_STATS_LINE_HEIGHT;
        drawStatLine(i, y);
    }
}

static void drawButtons(void) {
    LCDFont *font = bodyFont();
    pd->graphics->setFont(font);
    int fontHeight = pd->graphics->getFontHeight(font);
    int buttonHeight = fontHeight + GO_BUTTON_PADDING_Y * 2;

    int y = GO_BUTTONS_Y;
    for (int i = 0; i < GO_BUTTON_COUNT; i++) {
        const char *t = buttonText(i);
        int textWidth = pd->graphics->getTextWidth(font, t, strlen(t), kUTF8Encoding, 0);
        int rectW = textWidth + GO_BUTTON_PADDING_X * 2;
        int rectX = (SCREEN_WIDTH - rectW) / 2;

        pd->graphics->drawText(t, strlen(t), kUTF8Encoding, rectX + GO_BUTTON_PADDING_X, y + GO_BUTTON_PADDING_Y);

        if (i == selectedButton) {
            pd->graphics->drawRect(rectX - 1, y - 1, rectW + 2, buttonHeight + 2, kColorWhite);
        }

        y += buttonHeight + GO_BUTTON_VERTICAL_GAP;
    }
}

void updateGameOverScene(float deltaTime) {
    (void)deltaTime;

    PDButtons pushed;
    pd->system->getButtonState(NULL, &pushed, NULL);

    if (!isSceneTransitioning()) {
        if (pushed & kButtonUp) {
            playSound(SFX_UI_NAVIGATE);
            selectedButton = (selectedButton - 1 + GO_BUTTON_COUNT) % GO_BUTTON_COUNT;
        } else if (pushed & kButtonDown) {
            playSound(SFX_UI_NAVIGATE);
            selectedButton = (selectedButton + 1) % GO_BUTTON_COUNT;
        } else if (pushed & kButtonA) {
            playSound(SFX_UI_CLICK);
            if (selectedButton == 0) {
                switchScene(SCENE_CLASS_SELECT);
            } else {
                switchScene(SCENE_TITLE);
            }
        }
    }

    pd->graphics->fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, kColorBlack);
    pd->graphics->setDrawMode(kDrawModeInverted);
    drawHeader();
    pd->graphics->setDrawMode(kDrawModeCopy);
    drawFinalTime();
    drawStats();
    drawButtons();
}
