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

void initAudioManager(void);
void playSound(SFX sfx);