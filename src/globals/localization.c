#include <stdbool.h>
#include <stddef.h>

#include "globals/localization.h"
#include "game/settings/settings.h"

// Unified string table. Edit this file to add, change, or translate any UI
// string in the game. Leave the JP column NULL to fall back to the EN value.
//
// Strings NOT in this table (and where to find them if you need to edit them):
//   - Rune names and descriptions: src/game/systems/runeManager.c
//   - Spell names and descriptions: src/game/player/spells/spellData.c
//   - Shop "Skip" page and "Rune" type: src/game/systems/shopManager.c
//   These are kept where they live because they are part of structured data
//   tables rendered into cached bitmaps for both languages simultaneously.

typedef struct LocEntry {
    const char *en;
    const char *jp;
} LocEntry;

static const LocEntry LOC_TABLE[STR_COUNT] = {
    // System menu items (Playdate system menu is ASCII-only, so JP is left as EN)
    [STR_MENU_QUIT_RUN] = {
        "Quit Run",
        NULL,
    },
    [STR_MENU_RUNES] = {
        "Runes",
        NULL,
    },
    [STR_MENU_CLOSE] = {
        "Close",
        NULL,
    },
    [STR_MENU_SKIP_TUTORIAL] = {
        "Skip Tutorial",
        NULL,
    },

    // Quit confirmation dialog
    [STR_QUIT_RUN_TITLE] = {
        "Quit Run?",
        "ランをやめる？",
    },
    [STR_QUIT_RUN_BODY] = {
        "Are you sure? All progress will be lost.",
        "本当にやめますか？進行状況はすべて失われます。",
    },
    [STR_YES] = {
        "Yes",
        "はい",
    },
    [STR_NO] = {
        "No",
        "いいえ",
    },

    // Title scene (currently English-only; add JP translations to enable)
    [STR_TITLE_LINE_1] = {
        "Raycasting",
        NULL,
    },
    [STR_TITLE_LINE_2] = {
        "Game",
        NULL,
    },
    [STR_TITLE_BUTTON_START] = {
        "Start",
        NULL,
    },
    [STR_TITLE_BUTTON_TUTORIAL] = {
        "Tutorial",
        NULL,
    },

    // Class select scene (English-only)
    [STR_CLASS_SELECT_TITLE] = {
        "Select Class",
        NULL,
    },

    // Loading scene (English-only)
    [STR_LOADING_DONE] = {
        "Loaded!",
        NULL,
    },
    [STR_LOADING_FONTS] = {
        "Loading fonts...",
        NULL,
    },
    [STR_LOADING_TEXTURES] = {
        "Loading textures...",
        NULL,
    },
    [STR_LOADING_AUDIO] = {
        "Loading audio...",
        NULL,
    },

    // Game over scene
    [STR_GAMEOVER_VICTORY] = {
        "Victory!",
        "勝利！",
    },
    [STR_GAMEOVER_DEFEATED] = {
        "Defeated...",
        "敗北...",
    },
    [STR_GAMEOVER_LOCATION_FORMAT] = {
        "%s - Room %d",
        "%s %d",
    },
    [STR_GAMEOVER_STAT_TIME] = {
        "TIME",
        "時間",
    },
    [STR_GAMEOVER_STAT_LOCATION] = {
        "LOCATION",
        "場所",
    },
    [STR_GAMEOVER_STAT_DAMAGE_DEALT] = {
        "DAMAGE DEALT",
        "与ダメージ",
    },
    [STR_GAMEOVER_STAT_DAMAGE_TAKEN] = {
        "DAMAGE TAKEN",
        "被ダメージ",
    },
    [STR_GAMEOVER_STAT_ACCURACY] = {
        "ACCURACY",
        "命中率",
    },
    [STR_GAMEOVER_STAT_DODGES] = {
        "DODGES",
        "回避",
    },
    [STR_GAMEOVER_BUTTON_TRY_AGAIN] = {
        "Try again?",
        "もう一度？",
    },
    [STR_GAMEOVER_BUTTON_RETURN_TO_TITLE] = {
        "Return to title",
        "タイトルへ",
    },

    // Area names
    [STR_AREA_TWILIGHT_COURT] = {
        "The Twilight Court",
        "薄明の宮廷",
    },
    [STR_AREA_LOST_ARCHIVES] = {
        "Lost Archives",
        "失われた書庫",
    },
    [STR_AREA_ECHOING_CAVERNS] = {
        "Echoing Caverns",
        "響きの洞窟",
    },
    [STR_AREA_TEST] = {
        "Test",
        "テスト",
    },

    // In-game room HUD
    [STR_ROOM_BOSS] = {
        "Boss",
        "ボス部屋",
    },
    [STR_ROOM_FORMAT] = {
        "Room %d/%d",
        "部屋 %d/%d",
    },

    // Popup banners
    [STR_POPUP_ROOM_CLEARED] = {
        "Room Cleared",
        "部屋クリア",
    },
    [STR_POPUP_AREA_CLEARED] = {
        "Floor Cleared",
        "フロア クリア",
    },
    [STR_POPUP_ELDER_BOSS] = {
        "The Elder",
        "エルダー",
    },
    [STR_POPUP_WRAITH_BOSS] = {
        "The Wraith",
        "レイス",
    },
    [STR_POPUP_DRAGON_BOSS] = {
        "The Dragon",
        "ドラゴン",
    },

    // Tutorial dialog pages (currently English-only; add JP translations to enable)
    [STR_TUTORIAL_R1_1] = {
        "Use the d-pad to move and the crank to look around.",
        NULL,
    },
    [STR_TUTORIAL_R1_2] = {
        "Dock the crank to use d-pad only controls instead.",
        NULL,
    },
    [STR_TUTORIAL_R1_3] = {
        "Try moving to each arrow!",
        NULL,
    },
    [STR_TUTORIAL_R2_1] = {
        "Press the A button to cast your basic spell.",
        NULL,
    },
    [STR_TUTORIAL_R2_2] = {
        "Hold A to keep casting whenever it's off cooldown.",
        NULL,
    },
    [STR_TUTORIAL_R2_3] = {
        "Try hitting the enemy with your basic spell!",
        NULL,
    },
    [STR_TUTORIAL_R3_1] = {
        "Press the B button to cast your special spell.",
        NULL,
    },
    [STR_TUTORIAL_R3_2] = {
        "The mage class has a lightning spell that chains between nearby enemies.",
        NULL,
    },
    [STR_TUTORIAL_R3_3] = {
        "Try casting your special spell on the groups of enemies!",
        NULL,
    },
    [STR_TUTORIAL_R4_1] = {
        "Double-tap a d-pad direction to dash.",
        NULL,
    },
    [STR_TUTORIAL_R4_2] = {
        "Dashes are limited but replenish over time.",
        NULL,
    },
    [STR_TUTORIAL_R4_3] = {
        "You're briefly invincible while dashing.",
        NULL,
    },
    [STR_TUTORIAL_R4_4] = {
        "Dash across the pit ahead!",
        NULL,
    },
    [STR_TUTORIAL_R5_1] = {
        "Each cleared room rewards you with a choice of three runes.",
        NULL,
    },
    [STR_TUTORIAL_R5_2] = {
        "You can view the runes you've collected from the system menu.",
        NULL,
    },
    [STR_TUTORIAL_R5_3] = {
        "Try picking one!",
        NULL,
    },
    [STR_TUTORIAL_R5_4] = {
        "Open it now to take a look!",
        NULL,
    },
    [STR_TUTORIAL_R6_1] = {
        "You've finished the tutorial! Enter the gate to start your first run.",
        NULL,
    },
    [STR_TUTORIAL_NEXT_ROOM] = {
        "Room complete! Follow the arrow to the next room.",
        NULL,
    },
};

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
