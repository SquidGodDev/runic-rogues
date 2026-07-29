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

// Selectable display refresh rate. Index into the cycle; FPS_SETTING_40 is the
// default and maps to the original hard-coded 40 FPS behavior.
typedef enum FpsSetting {
    FPS_SETTING_30,
    FPS_SETTING_40,
    FPS_SETTING_50,
    FPS_SETTING_COUNT
} FpsSetting;

// Bar settings store a discrete level; the middle level is the default and
// maps to the original hard-coded behavior.
#define SETTING_BAR_MAX_LEVEL 6
#define SETTING_BAR_DEFAULT_LEVEL 3

// Crank turn speed is a 0-100 int; 50 keeps the original 1x feel and the extremes
// span a wide sensitivity range (see getCrankTurnSpeedMultiplier).
#define SETTING_CRANK_TURN_SPEED_MIN 0
#define SETTING_CRANK_TURN_SPEED_MAX 100
#define SETTING_CRANK_TURN_SPEED_DEFAULT 50

#define SETTING_CRANK_OFFSET_MIN 0
#define SETTING_CRANK_OFFSET_MAX 359

// Absolute-crank gearing: how many crank revolutions map to a full 360° turn.
// Index into the cycle; 1x is the default (one revolution = one turn).
// What the d-pad left/right buttons do. Strafe is the default and maps to the
// original behavior.
typedef enum DpadLeftRightMode {
    DPAD_LR_STRAFE,
    DPAD_LR_TURN,
    DPAD_LR_MODE_COUNT
} DpadLeftRightMode;

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
    bool toggleFireBasic;   // press (instead of hold) to toggle continuous basic-spell fire
    bool toggleFireSpecial; // press (instead of hold) to toggle continuous special-spell fire
    bool drawFPS;
    bool showTimer;
    int targetFps; // FpsSetting index; selects the display refresh rate
    int crankTurnSpeed;
    int dpadTurnSpeed;
    int dpadLeftRight; // DpadLeftRightMode index
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

int getTargetFps(void);
void applyTargetFps(void);

float getCrankTurnSpeedMultiplier(void);
float getDpadTurnSpeedMultiplier(void);
float getAbsoluteCrankRatio(void);
float getMusicVolumeSetting(void);
float getSfxVolumeSetting(void);
