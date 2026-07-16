#pragma once

#include "pd_api.h"

typedef enum SFX {
    SFX_SHOOT,
    SFX_WALL_HIT,
    SFX_ENEMY_HIT,
    SFX_ENEMY_DEATH,
    SFX_PLAYER_HIT,
    SFX_LIGHTNING,
    SFX_UI_NAVIGATE_TAB,
    SFX_UI_NAVIGATE,
    SFX_UI_ADJUST_UP,
    SFX_UI_ADJUST_DOWN,
    SFX_UI_ADJUST_NEUTRAL,
    SFX_UI_RUNE_SLIDE,
    SFX_UI_ERROR,
    SFX_UI_CHECKBOX_CHECKED,
    SFX_UI_CHECKBOX_UNCHECKED,
    SFX_UI_CLICK,
    SFX_UI_CLICK_NO,
    SFX_SHOP_SELECT,
    SFX_AREA_MAP_PLAYER_SLIDE,
    SFX_AREA_MAP_AREA_SLIDE,
    SFX_AREA_MAP_SELECT,
    SFX_CLASS_SELECT,
    SFX_CLASS_BACK,
    SFX_CLASS_ERROR,
    SFX_SCENE_TRANSITION_OUT,
    SFX_SCENE_TRANSITION_IN,
    SFX_TUTORIAL_TYPEWRITER,
    SFX_TUTORIAL_ARROW_PICKUP,
    SFX_TUTORIAL_PASS,
    SFX_TUTORIAL_CONFIRM,
    SFX_COUNT
} SFX;

// Scaffolding for future audio file loading.
typedef enum SFXFile {
    SFX_FILE_COUNT
} SFXFile;

typedef enum Music {
    MUSIC_NONE = -1,
    MUSIC_TITLE,
    MUSIC_TUTORIAL,
    MUSIC_RUNIC_ROGUES_1,
    MUSIC_RUNIC_ROGUES_2,
    MUSIC_RUNIC_ROGUES_3,
    MUSIC_COUNT
} Music;

int getAudioAssetCount(void);
void loadAudioAsset(int step);
void playSound(SFX sfx);

void playSong(Music song);
void stopSong(void);
Music getCurrentSong(void);
void updateMusicManager(float deltaTime);

void setMusicLowpassEnabled(bool enabled);
void setMusicPaused(bool paused);
void setMusicPausedWithFade(bool paused);
void resetMusicManager(void);