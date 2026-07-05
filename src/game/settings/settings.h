#pragma once

#include <stdbool.h>

typedef enum Language {
    LANGUAGE_EN,
    LANGUAGE_JP,
    LANGUAGE_COUNT
} Language;

// Bar settings store a discrete level; the middle level is the default and
// maps to the original hard-coded behavior.
#define SETTING_BAR_MAX_LEVEL 6
#define SETTING_BAR_DEFAULT_LEVEL 3

typedef struct Settings {
    Language language;
    bool drawMinimapWalls;
    bool spellButtonsReversed;
    bool drawFPS;
    int crankTurnSpeed;
    int dpadTurnSpeed;
    bool dpadStrafe;
    bool absoluteCrank;
    bool invertCrank;
    bool invertDpad;
    int musicVolume;
    int sfxVolume;
} Settings;

extern Settings SETTINGS;

void initSettings(void);
void resetGeneralSettings(void);
void resetControlSettings(void);
void resetAudioSettings(void);

// 0.5x at level 0, 1.0x at the default middle level, 1.5x at max
float getCrankTurnSpeedMultiplier(void);
float getDpadTurnSpeedMultiplier(void);
// 0.0 at level 0, 1.0 at max
float getMusicVolumeSetting(void);
float getSfxVolumeSetting(void);
