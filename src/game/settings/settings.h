#pragma once

#include <stdbool.h>

typedef enum Language {
    LANGUAGE_EN,
    LANGUAGE_JP,
    LANGUAGE_COUNT
} Language;

typedef enum ControlScheme {
    CONTROL_SCHEME_CRANK,
    CONTROL_SCHEME_DPAD,
    CONTROL_SCHEME_COUNT
} ControlScheme;

// Bar settings store a discrete level; the middle level is the default and
// maps to the original hard-coded behavior.
#define SETTING_BAR_MAX_LEVEL 6
#define SETTING_BAR_DEFAULT_LEVEL 3

#define SETTING_CRANK_OFFSET_MIN 0
#define SETTING_CRANK_OFFSET_MAX 359

// Absolute-crank gearing: how many crank revolutions map to a full 360° turn.
// Index into the cycle; 1x is the default (one revolution = one turn).
typedef enum CrankRatio {
    CRANK_RATIO_1X,   // 1 revolution = 360°
    CRANK_RATIO_HALF, // 2 revolutions = 360°
    CRANK_RATIO_THIRD, // 3 revolutions = 360°
    CRANK_RATIO_COUNT
} CrankRatio;

typedef struct Settings {
    // Stored as int (not the enum type) so their width is stable under the ARM
    // build's -fshort-enums: the settings menu reads these via *(int *)row->value,
    // which would otherwise over-read a 1-byte enum field on device. The enum
    // constants (LANGUAGE_*, CONTROL_SCHEME_*) are still used for the values.
    int language;
    int controlScheme;
    bool drawMinimapWalls;
    bool spellButtonsReversed;
    bool drawFPS;
    int crankTurnSpeed;
    int dpadTurnSpeed;
    bool dpadStrafe;
    bool absoluteCrank;
    int absoluteCrankOffset; // degrees, 0-360; rotates the absolute-crank reference angle
    int crankRatio;          // CrankRatio index; scales absolute-crank gearing
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
void resetDebugSettings(void);

float getCrankTurnSpeedMultiplier(void);
float getDpadTurnSpeedMultiplier(void);
float getAbsoluteCrankRatio(void);
float getMusicVolumeSetting(void);
float getSfxVolumeSetting(void);
