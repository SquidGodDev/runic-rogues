#pragma once

#include "pd_api.h"

typedef enum SFX {
    SFX_SHOOT,
    SFX_WALL_HIT,
    SFX_ENEMY_HIT,
    SFX_ENEMY_DEATH,
    SFX_PLAYER_HIT,
    SFX_COUNT
} SFX;

// Scaffolding for future audio file loading.
typedef enum SFXFile {
    SFX_FILE_COUNT
} SFXFile;

typedef enum Music {
    MUSIC_COUNT
} Music;

int getAudioAssetCount(void);
void loadAudioAsset(int step);
void playSound(SFX sfx);