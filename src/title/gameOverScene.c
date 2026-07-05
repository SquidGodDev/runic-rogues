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

#define GO_HEADER_Y                12
#define GO_STATS_TOP_Y             36
#define GO_STATS_LINE_HEIGHT       12
#define GO_STATS_LABEL_X           16
#define GO_STATS_VALUE_RIGHT_X     184
#define GO_BUTTONS_Y               70
#define GO_BUTTON_VERTICAL_GAP     8
#define GO_BUTTON_PADDING_X        4
#define GO_BUTTON_PADDING_Y        2

#define GO_DEFEAT_CHAR_TIME        0.10f
#define GO_VICTORY_CHAR_TIME       0.05f
#define GO_HEADER_PAUSE            0.50f
#define GO_STAT_REVEAL_INTERVAL    0.18f
#define GO_STATS_PAUSE             0.40f
#define GO_BUTTONS_FADE_TIME       0.30f

#define GO_STAT_COUNT              6
#define GO_BUTTON_COUNT            2

typedef enum {
    GO_PHASE_HEADER,
    GO_PHASE_PAUSE_AFTER_HEADER,
    GO_PHASE_STATS,
    GO_PHASE_PAUSE_AFTER_STATS,
    GO_PHASE_BUTTONS,
    GO_PHASE_INTERACTIVE
} GameOverPhase;

static GameOverPhase phase;
static float charAccumulator;
static float pauseTimer;
static float buttonsRevealTimer;
static int   revealedHeaderChars;
static int   revealedStatLines;
static int   selectedButton;

static int utf8NumChars(const char *s) {
    int count = 0;
    while (*s) {
        unsigned char c = (unsigned char)*s;
        if ((c & 0xC0) != 0x80) {
            count++;
        }
        s++;
    }
    return count;
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
    char *out = NULL;
    int totalSeconds = (int)runStatsGetTime();
    if (totalSeconds < 0) {
        totalSeconds = 0;
    }
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;
    if (hours > 0) {
        pd->system->formatString(&out, "%d:%02d:%02d", hours, minutes, seconds);
    } else {
        pd->system->formatString(&out, "%d:%02d", minutes, seconds);
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

static void skipToInteractive(void) {
    revealedHeaderChars = utf8NumChars(headerText());
    revealedStatLines = GO_STAT_COUNT;
    buttonsRevealTimer = GO_BUTTONS_FADE_TIME;
    phase = GO_PHASE_INTERACTIVE;
}

void initGameOverScene(void) {
    removeGameMenuItem();
    freeRuneInventoryCache();

    phase = GO_PHASE_HEADER;
    charAccumulator = 0.0f;
    pauseTimer = 0.0f;
    buttonsRevealTimer = 0.0f;
    revealedHeaderChars = 0;
    revealedStatLines = 0;
    selectedButton = 0;
}

static void drawHeader(void) {
    const char *text = headerText();
    int byteLen = utf8ByteOffsetForChar(text, revealedHeaderChars);
    if (byteLen <= 0) {
        return;
    }
    LCDFont *font = headerFont();
    pd->graphics->setFont(font);
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

static void drawStats(void) {
    if (true) {
        // For now, skip for later refactor
        return;
    }
    for (int i = 0; i < revealedStatLines; i++) {
        int y = GO_STATS_TOP_Y + i * GO_STATS_LINE_HEIGHT;
        drawStatLine(i, y);
    }
}

static void drawButtons(void) {
    if (buttonsRevealTimer <= 0.0f) {
        return;
    }

    LCDFont *font = bodyFont();
    pd->graphics->setFont(font);
    int fontHeight = pd->graphics->getFontHeight(font);
    int buttonHeight = fontHeight + GO_BUTTON_PADDING_Y * 2;

    int slideOffset = 0;
    if (buttonsRevealTimer < GO_BUTTONS_FADE_TIME) {
        float t = buttonsRevealTimer / GO_BUTTONS_FADE_TIME;
        slideOffset = (int)((1.0f - t) * 6.0f);
    }

    int y = GO_BUTTONS_Y + slideOffset;
    for (int i = 0; i < GO_BUTTON_COUNT; i++) {
        const char *t = buttonText(i);
        int textWidth = pd->graphics->getTextWidth(font, t, strlen(t), kUTF8Encoding, 0);
        int rectW = textWidth + GO_BUTTON_PADDING_X * 2;
        int rectX = (SCREEN_WIDTH - rectW) / 2;

        pd->graphics->setDrawMode(kDrawModeInverted);
        pd->graphics->drawText(t, strlen(t), kUTF8Encoding, rectX + GO_BUTTON_PADDING_X, y + GO_BUTTON_PADDING_Y);

        if (i == selectedButton && phase == GO_PHASE_INTERACTIVE) {
            pd->graphics->setDrawMode(kDrawModeCopy);
            pd->graphics->drawRect(rectX - 1, y - 1, rectW + 2, buttonHeight + 2, kColorWhite);
        }

        y += buttonHeight + GO_BUTTON_VERTICAL_GAP;
    }
    pd->graphics->setDrawMode(kDrawModeCopy);
}

void updateGameOverScene(float deltaTime) {
    PDButtons pushed;
    pd->system->getButtonState(NULL, &pushed, NULL);

    bool justSkipped = false;
    if (phase != GO_PHASE_INTERACTIVE && pushed != 0) {
        skipToInteractive();
        justSkipped = true;
    } else {
        switch (phase) {
            case GO_PHASE_HEADER: {
                int totalChars = utf8NumChars(headerText());
                float charTime = runStatsGetVictory() ? GO_VICTORY_CHAR_TIME : GO_DEFEAT_CHAR_TIME;
                charAccumulator += deltaTime;
                while (charAccumulator >= charTime && revealedHeaderChars < totalChars) {
                    charAccumulator -= charTime;
                    revealedHeaderChars++;
                }
                if (revealedHeaderChars >= totalChars) {
                    revealedHeaderChars = totalChars;
                    phase = GO_PHASE_PAUSE_AFTER_HEADER;
                    pauseTimer = GO_HEADER_PAUSE;
                }
                break;
            }
            case GO_PHASE_PAUSE_AFTER_HEADER:
                pauseTimer -= deltaTime;
                if (pauseTimer <= 0.0f) {
                    phase = GO_PHASE_BUTTONS;
                    buttonsRevealTimer = 0.0f;
                }
                break;
            case GO_PHASE_STATS:
                charAccumulator += deltaTime;
                while (charAccumulator >= GO_STAT_REVEAL_INTERVAL && revealedStatLines < GO_STAT_COUNT) {
                    charAccumulator -= GO_STAT_REVEAL_INTERVAL;
                    revealedStatLines++;
                }
                if (revealedStatLines >= GO_STAT_COUNT) {
                    revealedStatLines = GO_STAT_COUNT;
                    phase = GO_PHASE_PAUSE_AFTER_STATS;
                    pauseTimer = GO_STATS_PAUSE;
                }
                break;
            case GO_PHASE_PAUSE_AFTER_STATS:
                pauseTimer -= deltaTime;
                if (pauseTimer <= 0.0f) {
                    phase = GO_PHASE_BUTTONS;
                    buttonsRevealTimer = 0.0f;
                }
                break;
            case GO_PHASE_BUTTONS:
                buttonsRevealTimer += deltaTime;
                if (buttonsRevealTimer >= GO_BUTTONS_FADE_TIME) {
                    buttonsRevealTimer = GO_BUTTONS_FADE_TIME;
                    phase = GO_PHASE_INTERACTIVE;
                }
                break;
            case GO_PHASE_INTERACTIVE:
                break;
        }
    }

    if (phase == GO_PHASE_INTERACTIVE && !isSceneTransitioning() && !justSkipped) {
        if (pushed & kButtonUp) {
            selectedButton = (selectedButton - 1 + GO_BUTTON_COUNT) % GO_BUTTON_COUNT;
        } else if (pushed & kButtonDown) {
            selectedButton = (selectedButton + 1) % GO_BUTTON_COUNT;
        } else if (pushed & kButtonA) {
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
    drawStats();
    pd->graphics->setDrawMode(kDrawModeCopy);
    drawButtons();
}
