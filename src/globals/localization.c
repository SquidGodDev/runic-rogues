#define TARGET_EXTENSION 1
#include "pd_api.h"

#include <stdbool.h>
#include <stddef.h>

#include "globals/globals.h"
#include "globals/localization.h"
#include "game/settings/settings.h"

// Unified localization loader. All strings live in Source/data/localization.csv,
// which is bundled into the .pdx. This module reads it once at boot, keeps the
// file contents in a single buffer, and exposes the rows both by string key
// (getLocText) and by StringKey enum (getLocString/getLocStringFor).

#define LOC_CSV_PATH "data/localization.csv"

typedef struct LocRow {
    const char *key;
    const char *en;
    const char *jp; // may be empty; empty means "fall back to en"
} LocRow;

typedef struct LocEntry {
    const char *en;
    const char *jp; // NULL means "fall back to en"
} LocEntry;

static char *locBuffer = NULL;      // owns all string data for the app lifetime
static LocRow *locRows = NULL;
static int locRowCount = 0;
static LocEntry LOC_TABLE[STR_COUNT];

// Maps each StringKey to its CSV row key. Designated initializers keep this in
// sync with the enum regardless of source order; any missing entry stays NULL.
static const char *STR_KEYS[STR_COUNT] = {
    [STR_MENU_MENU] = "menu.menu",
    [STR_MENU_CLOSE] = "menu.close",
    [STR_MENU_SKIP_TUTORIAL] = "menu.skip_tutorial",

    [STR_QUIT_RUN_TITLE] = "quit.title",
    [STR_QUIT_RUN_BODY] = "quit.body",
    [STR_YES] = "common.yes",
    [STR_NO] = "common.no",

    [STR_SETTING_LANGUAGE] = "setting.language",
    [STR_LANG_ENGLISH] = "lang.english",
    [STR_LANG_JP] = "lang.jp",
    [STR_SETTING_DRAW_FPS] = "setting.draw_fps",
    [STR_SETTING_CONTROL] = "setting.control",
    [STR_CONTROL_CRANK] = "control.crank",
    [STR_CONTROL_DPAD] = "control.dpad",
    [STR_SECTION_CRANK_CONTROLS] = "section.crank_controls",
    [STR_SECTION_DPAD_CONTROLS] = "section.dpad_controls",
    [STR_SETTING_CRANK_TURN_SPEED] = "setting.crank_turn_speed",
    [STR_SETTING_ABSOLUTE_CRANK] = "setting.absolute_crank",
    [STR_SETTING_ABSOLUTE_CRANK_OFFSET] = "setting.absolute_crank_offset",
    [STR_SETTING_CRANK_RATIO] = "setting.crank_ratio",
    [STR_CRANK_RATIO_1X] = "crank_ratio.1x",
    [STR_CRANK_RATIO_HALF] = "crank_ratio.half",
    [STR_CRANK_RATIO_THIRD] = "crank_ratio.third",
    [STR_SETTING_INVERT_CRANK] = "setting.invert_crank",
    [STR_SETTING_DPAD_TURN_SPEED] = "setting.dpad_turn_speed",
    [STR_SETTING_DPAD_STRAFE] = "setting.dpad_strafe",
    [STR_SETTING_INVERT_DPAD] = "setting.invert_dpad",
    [STR_SETTING_SWAP_SPELL_BUTTONS] = "setting.swap_spell_buttons",
    [STR_SETTING_MUSIC_VOLUME] = "setting.music_volume",
    [STR_SETTING_SFX_VOLUME] = "setting.sfx_volume",
    [STR_SETTING_RESET] = "setting.reset",
    [STR_RESET_CONFIRM_TITLE] = "reset.title",
    [STR_RESET_CONFIRM_BODY] = "reset.body",

    [STR_SETTING_SKIP_TO_ROOM] = "setting.skip_to_room",
    [STR_SETTING_AREA_OVERRIDE] = "setting.area_override",
    [STR_SETTING_INSTANT_ROOM_CLEAR] = "setting.instant_room_clear",
    [STR_SETTING_INSTANT_KILL_BOSS] = "setting.instant_kill_boss",
    [STR_SETTING_START_IN_TEST_AREA] = "setting.start_in_test_area",
    [STR_SETTING_SKIP_AREA_MAP] = "setting.skip_area_map",

    [STR_EXIT_CLOSE_MENU] = "exit.close_menu",
    [STR_EXIT_QUIT_RUN] = "exit.quit_run",

    [STR_TITLE_BUTTON_START] = "title.start",
    [STR_TITLE_BUTTON_TUTORIAL] = "title.tutorial",
    [STR_TITLE_BUTTON_SETTINGS] = "title.settings",

    [STR_CLASS_SELECT_TITLE] = "class_select.title",

    [STR_LOADING_DONE] = "loading.done",
    [STR_LOADING_FONTS] = "loading.fonts",
    [STR_LOADING_TEXTURES] = "loading.textures",
    [STR_LOADING_AUDIO] = "loading.audio",

    [STR_GAMEOVER_VICTORY] = "gameover.victory",
    [STR_GAMEOVER_DEFEATED] = "gameover.defeated",
    [STR_GAMEOVER_LOCATION_FORMAT] = "gameover.location_format",
    [STR_GAMEOVER_STAT_TIME] = "gameover.stat_time",
    [STR_GAMEOVER_STAT_LOCATION] = "gameover.stat_location",
    [STR_GAMEOVER_STAT_DAMAGE_DEALT] = "gameover.stat_damage_dealt",
    [STR_GAMEOVER_STAT_DAMAGE_TAKEN] = "gameover.stat_damage_taken",
    [STR_GAMEOVER_STAT_ACCURACY] = "gameover.stat_accuracy",
    [STR_GAMEOVER_STAT_DODGES] = "gameover.stat_dodges",
    [STR_GAMEOVER_BUTTON_TRY_AGAIN] = "gameover.try_again",
    [STR_GAMEOVER_BUTTON_RETURN_TO_TITLE] = "gameover.return_to_title",

    [STR_AREA_TWILIGHT_COURT] = "area.twilight_court",
    [STR_AREA_LOST_ARCHIVES] = "area.lost_archives",
    [STR_AREA_ECHOING_CAVERNS] = "area.echoing_caverns",
    [STR_AREA_TEST] = "area.test",

    [STR_ROOM_BOSS] = "room.boss",
    [STR_ROOM_FORMAT] = "room.format",

    [STR_POPUP_ROOM_CLEARED] = "popup.room_cleared",
    [STR_POPUP_AREA_CLEARED] = "popup.area_cleared",
    [STR_POPUP_ELDER_BOSS] = "popup.elder_boss",
    [STR_POPUP_WRAITH_BOSS] = "popup.wraith_boss",
    [STR_POPUP_DRAGON_BOSS] = "popup.dragon_boss",

    [STR_TUTORIAL_R1_1] = "tutorial.r1_1",
    [STR_TUTORIAL_R1_2] = "tutorial.r1_2",
    [STR_TUTORIAL_R1_3] = "tutorial.r1_3",
    [STR_TUTORIAL_R2_1] = "tutorial.r2_1",
    [STR_TUTORIAL_R2_2] = "tutorial.r2_2",
    [STR_TUTORIAL_R2_3] = "tutorial.r2_3",
    [STR_TUTORIAL_R3_1] = "tutorial.r3_1",
    [STR_TUTORIAL_R3_2] = "tutorial.r3_2",
    [STR_TUTORIAL_R3_3] = "tutorial.r3_3",
    [STR_TUTORIAL_R4_1] = "tutorial.r4_1",
    [STR_TUTORIAL_R4_2] = "tutorial.r4_2",
    [STR_TUTORIAL_R4_3] = "tutorial.r4_3",
    [STR_TUTORIAL_R4_4] = "tutorial.r4_4",
    [STR_TUTORIAL_R5_1] = "tutorial.r5_1",
    [STR_TUTORIAL_R5_2] = "tutorial.r5_2",
    [STR_TUTORIAL_R5_3] = "tutorial.r5_3",
    [STR_TUTORIAL_R5_4] = "tutorial.r5_4",
    [STR_TUTORIAL_R6_1] = "tutorial.r6_1",
    [STR_TUTORIAL_NEXT_ROOM] = "tutorial.next_room",

    [STR_SHOP_TYPE_RUNE] = "shop.type_rune",
    [STR_SHOP_SKIP_NAME] = "shop.skip_name",
    [STR_SHOP_SKIP_DESC] = "shop.skip_desc",
};

// Parses one CSV field starting at *pp, unescaping quotes in place, and
// null-terminates it. Returns the field start; *pp is left pointing at the
// delimiter that ended the field and *sepOut receives that delimiter char
// (',', '\n', '\r', or '\0'). The delimiter is captured before the terminator
// is written because an unquoted field terminates exactly on the delimiter byte
// (out == p), so writing '\0' there clobbers *p — callers must branch on *sepOut
// rather than re-reading *p.
static char *parseField(char **pp, char *sepOut) {
    char *p = *pp;
    char *field = p;
    char *out = p;
    bool quoted = false;
    if (*p == '"') {
        quoted = true;
        p++;
    }
    while (true) {
        char c = *p;
        if (quoted) {
            if (c == '"') {
                if (p[1] == '"') {
                    *out++ = '"';
                    p += 2;
                    continue;
                }
                quoted = false;
                p++;
                continue;
            }
            if (c == '\0') {
                break;
            }
            *out++ = c;
            p++;
            continue;
        }
        if (c == ',' || c == '\n' || c == '\r' || c == '\0') {
            break;
        }
        *out++ = c;
        p++;
    }
    *sepOut = *p;
    *out = '\0';
    *pp = p;
    return field;
}

static void parseCSV(void) {
    // Upper bound on rows: one per line.
    int maxRows = 1;
    for (char *p = locBuffer; *p != '\0'; p++) {
        if (*p == '\n') {
            maxRows++;
        }
    }
    locRows = pd->system->realloc(NULL, sizeof(LocRow) * maxRows);
    if (locRows == NULL) {
        pd->system->logToConsole("localization: failed to allocate %d rows", maxRows);
        return;
    }

    char *p = locBuffer;
    // Skip a UTF-8 BOM if present.
    if ((unsigned char)p[0] == 0xEF && (unsigned char)p[1] == 0xBB && (unsigned char)p[2] == 0xBF) {
        p += 3;
    }

    while (*p != '\0') {
        if (*p == '\n' || *p == '\r') {
            p++;
            continue;
        }
        if (*p == '#') {
            while (*p != '\0' && *p != '\n') {
                p++;
            }
            continue;
        }

        char sep;
        char *key = parseField(&p, &sep);
        char *en = "";
        char *jp = "";
        if (sep == ',') {
            p++;
            en = parseField(&p, &sep);
        }
        if (sep == ',') {
            p++;
            jp = parseField(&p, &sep);
        }
        // Advance past the row terminator. sep holds the real delimiter even
        // when parseField nulled *p (unquoted field ending on the delimiter).
        if (sep == '\r' && p[1] == '\n') {
            p += 2;
        } else if (sep != '\0') {
            p++;
        }

        if (key[0] == '\0') {
            continue;
        }
        locRows[locRowCount].key = key;
        locRows[locRowCount].en = en;
        locRows[locRowCount].jp = jp;
        locRowCount++;
    }
}

static const LocRow *findRow(const char *key) {
    for (int i = 0; i < locRowCount; i++) {
        if (strcmp(locRows[i].key, key) == 0) {
            return &locRows[i];
        }
    }
    return NULL;
}

void initLocalization(void) {
    locRowCount = 0;

    SDFile *file = pd->file->open(LOC_CSV_PATH, kFileRead);
    if (file == NULL) {
        pd->system->logToConsole("localization: could not open %s: %s", LOC_CSV_PATH, pd->file->geterr());
        for (int i = 0; i < STR_COUNT; i++) {
            LOC_TABLE[i] = (LocEntry){ "", NULL };
        }
        return;
    }

    pd->file->seek(file, 0, SEEK_END);
    int size = pd->file->tell(file);
    pd->file->seek(file, 0, SEEK_SET);
    if (size <= 0) {
        pd->system->logToConsole("localization: %s is empty", LOC_CSV_PATH);
        pd->file->close(file);
        return;
    }

    locBuffer = pd->system->realloc(NULL, size + 1);
    if (locBuffer == NULL) {
        pd->system->logToConsole("localization: failed to allocate %d bytes", size + 1);
        pd->file->close(file);
        return;
    }

    int totalRead = 0;
    while (totalRead < size) {
        int n = pd->file->read(file, locBuffer + totalRead, size - totalRead);
        if (n <= 0) {
            break;
        }
        totalRead += n;
    }
    pd->file->close(file);
    locBuffer[totalRead] = '\0';

    parseCSV();

    // Resolve the StringKey table up front so getLocString is a plain lookup.
    for (int i = 0; i < STR_COUNT; i++) {
        const char *key = STR_KEYS[i];
        if (key == NULL) {
            pd->system->logToConsole("localization: StringKey %d has no CSV key mapping", i);
            LOC_TABLE[i] = (LocEntry){ "", NULL };
            continue;
        }
        const LocRow *row = findRow(key);
        if (row == NULL) {
            pd->system->logToConsole("localization: missing CSV key '%s'", key);
            LOC_TABLE[i] = (LocEntry){ "", NULL };
            continue;
        }
        LOC_TABLE[i].en = row->en;
        LOC_TABLE[i].jp = (row->jp != NULL && row->jp[0] != '\0') ? row->jp : NULL;
    }

    pd->system->logToConsole("localization: loaded %d strings from %s", locRowCount, LOC_CSV_PATH);
}

const char *getLocText(const char *key, Language language) {
    const LocRow *row = findRow(key);
    if (row == NULL) {
        pd->system->logToConsole("localization: missing CSV key '%s'", key);
        return "";
    }
    if (language == LANGUAGE_JP && row->jp != NULL && row->jp[0] != '\0') {
        return row->jp;
    }
    return row->en != NULL ? row->en : "";
}

const char *getLocStringFor(StringKey key, Language language) {
    if (key < 0 || key >= STR_COUNT) {
        return "";
    }
    const LocEntry *entry = &LOC_TABLE[key];
    if (language == LANGUAGE_JP && entry->jp != NULL) {
        return entry->jp;
    }
    return entry->en != NULL ? entry->en : "";
}

const char *getLocString(StringKey key) {
    return getLocStringFor(key, SETTINGS.language);
}
