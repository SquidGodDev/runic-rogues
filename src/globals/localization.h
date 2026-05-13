#pragma once

// Unified localization. All scattered UI strings live in the table in
// localization.c, indexed by the StringKey enum below. To add or change a
// string, edit localization.c (and add an enum entry here if it's new).
// If a JP value is NULL in the table, getLocString falls back to EN.
typedef enum StringKey {
    // System menu items
    STR_MENU_QUIT_RUN,
    STR_MENU_RUNES,
    STR_MENU_CLOSE,
    STR_MENU_SKIP_TUTORIAL,

    // Quit confirmation dialog
    STR_QUIT_RUN_TITLE,
    STR_QUIT_RUN_BODY,
    STR_YES,
    STR_NO,

    // Title scene
    STR_TITLE_LINE_1,
    STR_TITLE_LINE_2,
    STR_TITLE_BUTTON_START,
    STR_TITLE_BUTTON_TUTORIAL,

    // Class select scene
    STR_CLASS_SELECT_TITLE,

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
    STR_AREA_ECHOING_CAVERNS,
    STR_AREA_LOST_ARCHIVES,
    STR_AREA_TEST,

    // In-game room HUD
    STR_ROOM_BOSS,
    STR_ROOM_FORMAT, // printf format: current room index, total rooms

    STR_COUNT
} StringKey;

const char *getLocString(StringKey key);
