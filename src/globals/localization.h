#pragma once

#include "game/settings/settings.h"

// Unified localization. Every user-facing string in the game lives in
// Source/localization.csv (columns: key,en,jp), which is bundled into the .pdx
// and loaded once at boot by initLocalization().
//
// UI strings are looked up through the StringKey enum below; each key maps to a
// CSV row via the STR_KEYS table in localization.c. Data tables (runes, spells,
// classes, shop) resolve their strings by string key through getLocText().
//
// To add or change a UI string: add a row to the CSV and, if the key is new,
// add an enum entry here plus its key string to STR_KEYS in localization.c.
// An empty jp value in the CSV falls back to the en value.
typedef enum StringKey {
    // System menu items
    STR_MENU_MENU,
    STR_MENU_CLOSE,
    STR_MENU_SKIP_TUTORIAL,

    // Quit confirmation dialog
    STR_QUIT_RUN_TITLE,
    STR_QUIT_RUN_BODY,
    STR_YES,
    STR_NO,

    // Skip tutorial confirmation dialog
    STR_SKIP_TUTORIAL_TITLE,
    STR_SKIP_TUTORIAL_BODY,

    // In-game menu settings tabs
    STR_SETTING_LANGUAGE,
    STR_LANG_ENGLISH,
    STR_LANG_JP,
    STR_SETTING_DRAW_FPS,
    STR_SETTING_SHOW_TIMER,
    STR_SETTING_FRAME_RATE,
    STR_FPS_30,
    STR_FPS_40,
    STR_FPS_50,
    STR_SETTING_CONTROL,
    STR_CONTROL_CRANK,
    STR_CONTROL_DPAD,
    STR_SECTION_CRANK_CONTROLS,
    STR_SECTION_DPAD_CONTROLS,
    STR_SETTING_CRANK_TURN_SPEED,
    STR_SETTING_ABSOLUTE_CRANK,
    STR_SETTING_ABSOLUTE_CRANK_OFFSET,
    STR_SETTING_CRANK_RATIO,
    STR_CRANK_RATIO_1X,
    STR_CRANK_RATIO_HALF,
    STR_CRANK_RATIO_THIRD,
    STR_SETTING_INVERT_CRANK,
    STR_SETTING_DPAD_TURN_SPEED,
    STR_SETTING_DPAD_LEFT_RIGHT,
    STR_DPAD_LR_STRAFE,
    STR_DPAD_LR_TURN,
    STR_SETTING_INVERT_DPAD,
    STR_SETTING_SWAP_SPELL_BUTTONS,
    STR_SETTING_TOGGLE_FIRE_BASIC,
    STR_SETTING_TOGGLE_FIRE_SPECIAL,
    STR_SETTING_MUSIC_VOLUME,
    STR_SETTING_SFX_VOLUME,
    STR_SETTING_RESET,
    STR_RESET_CONFIRM_TITLE,
    STR_RESET_CONFIRM_BODY,

    // In-game menu debug tab (English-only dev feature)
    STR_SETTING_SKIP_TO_ROOM,
    STR_SETTING_AREA_OVERRIDE,
    STR_SETTING_INSTANT_ROOM_CLEAR,
    STR_SETTING_INSTANT_KILL_BOSS,
    STR_SETTING_ONE_HP_BOSSES,
    STR_SETTING_START_IN_TEST_AREA,
    STR_SETTING_TEST_FIRST_ROOM_ENEMY,
    STR_SETTING_SKIP_AREA_MAP,

    // In-game menu exit tab
    STR_EXIT_CLOSE_MENU,
    STR_EXIT_QUIT_RUN,

    // Title scene
    STR_TITLE_BUTTON_START,
    STR_TITLE_BUTTON_TUTORIAL,
    STR_TITLE_BUTTON_SETTINGS,

    // Class select scene
    STR_CLASS_SELECT_TITLE,
    STR_CLASS_SELECT_BACK,
    STR_CLASS_SELECT_SELECT,
    STR_CLASS_SELECT_BASIC_TAG,
    STR_CLASS_SELECT_SPECIAL_TAG,
    STR_CLASS_SELECT_LOCKED,
    STR_CLASS_SELECT_UNLOCK_HINT, // printf format: name of the class to win with

    // Difficulty selector (STR_DIFFICULTY_0..4 must stay contiguous and ordered)
    STR_DIFFICULTY_0,
    STR_DIFFICULTY_1,
    STR_DIFFICULTY_2,
    STR_DIFFICULTY_3,
    STR_DIFFICULTY_4,
    STR_DIFFICULTY_LOCKED,

    // Loading scene
    STR_LOADING_DONE,
    STR_LOADING_FONTS,
    STR_LOADING_TEXTURES,
    STR_LOADING_AUDIO,

    // Game over scene
    STR_GAMEOVER_VICTORY,
    STR_GAMEOVER_DEFEATED,
    STR_GAMEOVER_LOCATION_FORMAT, // printf format: area name, room number
    STR_GAMEOVER_STAT_TIME,
    STR_GAMEOVER_STAT_LOCATION,
    STR_GAMEOVER_STAT_DAMAGE_DEALT,
    STR_GAMEOVER_STAT_DAMAGE_TAKEN,
    STR_GAMEOVER_STAT_ACCURACY,
    STR_GAMEOVER_STAT_DODGES,
    STR_GAMEOVER_BUTTON_TRY_AGAIN,
    STR_GAMEOVER_BUTTON_RETURN_TO_TITLE,

    // Area names
    STR_AREA_TWILIGHT_COURT,
    STR_AREA_LOST_ARCHIVES,
    STR_AREA_ECHOING_CAVERNS,
    STR_AREA_TEST,

    // In-game room HUD
    STR_ROOM_BOSS,
    STR_ROOM_FORMAT, // printf format: current room index, total rooms

    // Popup banners (rendered to bitmaps for both languages on init)
    STR_POPUP_ROOM_CLEARED,
    STR_POPUP_AREA_CLEARED,
    STR_POPUP_ELDER_BOSS,
    STR_POPUP_WRAITH_BOSS,
    STR_POPUP_DRAGON_BOSS,

    // Interactable prompt labels (rendered to bitmaps for both languages on init)
    STR_PROMPT_NEXT_FLOOR,

    // Tutorial dialog pages
    STR_TUTORIAL_R1_1,
    STR_TUTORIAL_R1_2,
    STR_TUTORIAL_R1_3,
    STR_TUTORIAL_R1_4,
    STR_TUTORIAL_R2_1,
    STR_TUTORIAL_R2_2,
    STR_TUTORIAL_R2_3,
    STR_TUTORIAL_R3_1,
    STR_TUTORIAL_R3_2,
    STR_TUTORIAL_R3_3,
    STR_TUTORIAL_R4_1,
    STR_TUTORIAL_R4_2,
    STR_TUTORIAL_R4_3,
    STR_TUTORIAL_R4_4,
    STR_TUTORIAL_R5_1,
    STR_TUTORIAL_R5_2,
    STR_TUTORIAL_R6_1,
    STR_TUTORIAL_R6_2,
    STR_TUTORIAL_NEXT_ROOM,

    // Tutorial objective HUD (printf format: current, target)
    STR_TUTORIAL_OBJ_ARROW,
    STR_TUTORIAL_OBJ_ENEMY,
    STR_TUTORIAL_OBJ_DASH,

    // Shop
    STR_SHOP_TYPE_RUNE,
    STR_SHOP_REROLL_NAME,
    STR_SHOP_REROLL_DESC,

    STR_COUNT
} StringKey;

// Loads Source/localization.csv and builds the lookup tables. Call once at boot
// (after initSettings) before any string is requested.
void initLocalization(void);

// Returns the string for the active language (SETTINGS.language), falling back
// to English if the active-language entry is empty.
const char *getLocString(StringKey key);

// Returns the string for a specific language. Use this when you need to render
// both languages up front (e.g. caching bitmaps for instant language switching).
const char *getLocStringFor(StringKey key, Language language);

// Raw lookup by string key (e.g. "rune.vampirism.name"). Used by the data
// tables (runes, spells, classes) to resolve their strings at init. Falls back
// to English when the requested language's value is empty; returns "" if the
// key is missing.
const char *getLocText(const char *key, Language language);
