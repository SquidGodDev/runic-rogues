#include "game/settings/settings.h"

Settings SETTINGS;

void resetGeneralSettings(void) {
    SETTINGS.language = LANGUAGE_EN;
    SETTINGS.drawFPS = true;
}

void resetControlSettings(void) {
    SETTINGS.crankTurnSpeed = SETTING_BAR_DEFAULT_LEVEL;
    SETTINGS.dpadTurnSpeed = SETTING_BAR_DEFAULT_LEVEL;
    SETTINGS.dpadStrafe = true;
    SETTINGS.absoluteCrank = false;
    SETTINGS.absoluteCrankOffset = 0;
    SETTINGS.invertCrank = false;
    SETTINGS.invertDpad = false;
    SETTINGS.spellButtonsReversed = false;
}

void resetAudioSettings(void) {
    SETTINGS.musicVolume = SETTING_BAR_DEFAULT_LEVEL;
    SETTINGS.sfxVolume = SETTING_BAR_DEFAULT_LEVEL;
}

void initSettings(void) {
    resetGeneralSettings();
    resetControlSettings();
    resetAudioSettings();
    SETTINGS.drawMinimapWalls = true; // Legacy: based off old minimap which is no longer used
}

static float barLevelToRatio(int level) {
    return (float)level / SETTING_BAR_MAX_LEVEL;
}

float getCrankTurnSpeedMultiplier(void) {
    return 0.5f + barLevelToRatio(SETTINGS.crankTurnSpeed);
}

float getDpadTurnSpeedMultiplier(void) {
    return 0.5f + barLevelToRatio(SETTINGS.dpadTurnSpeed);
}

float getMusicVolumeSetting(void) {
    return barLevelToRatio(SETTINGS.musicVolume);
}

float getSfxVolumeSetting(void) {
    return barLevelToRatio(SETTINGS.sfxVolume);
}
