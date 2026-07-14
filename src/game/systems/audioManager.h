#pragma once

#include "pd_api.h"

typedef enum SFX {
    SFX_SHOOT,
    SFX_WALL_HIT,
    SFX_ENEMY_HIT,
    SFX_ENEMY_DEATH,
    SFX_PLAYER_HIT,
    SFX_LIGHTNING,
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