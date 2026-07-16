#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "game/settings/settings.h"

Settings SETTINGS;
DebugFlags DEBUG;

static Language systemLanguage(void) {
    if (pd->system->getLanguage() == kPDLanguageJapanese) {
        return LANGUAGE_JP;
    }
    return LANGUAGE_EN;
}

void resetGeneralSettings(void) {
    SETTINGS.language = systemLanguage();
    SETTINGS.drawFPS = true;
}

void resetControlSettings(void) {
    SETTINGS.controlScheme = CONTROL_SCHEME_CRANK;
    SETTINGS.crankTurnSpeed = SETTING_BAR_DEFAULT_LEVEL;
    SETTINGS.dpadTurnSpeed = SETTING_BAR_DEFAULT_LEVEL;
    SETTINGS.dpadStrafe = true;
    SETTINGS.absoluteCrank = false;
    SETTINGS.absoluteCrankOffset = 0;
    SETTINGS.crankRatio = CRANK_RATIO_1X;
    SETTINGS.invertCrank = false;
    SETTINGS.invertDpad = false;
    SETTINGS.spellButtonsReversed = false;
}

void resetAudioSettings(void) {
    SETTINGS.musicVolume = SETTING_BAR_DEFAULT_LEVEL;
    SETTINGS.sfxVolume = SETTING_BAR_DEFAULT_LEVEL;
}

void resetDebugSettings(void) {
    DEBUG.skipToRoom = 0;
    DEBUG.areaOverride = 0;
    DEBUG.instantRoomClear = false;
    DEBUG.instantKillBoss = false;
    DEBUG.startInTestArea = false;
    DEBUG.skipAreaMap = false;
}

void initSettings(void) {
    resetGeneralSettings();
    resetControlSettings();
    resetAudioSettings();
    SETTINGS.drawMinimapWalls = true; // Legacy: based off old minimap which is no longer used
    DEBUG.enabled = false;
    resetDebugSettings();
}

static float barLevelToRatio(int level) {
    return (float)level / SETTING_BAR_MAX_LEVEL;
}

#define MUSIC_VOLUME_MAX 0.8f
#define SFX_VOLUME_MAX   1.0f

float getCrankTurnSpeedMultiplier(void) {
    return 0.5f + barLevelToRatio(SETTINGS.crankTurnSpeed);
}

float getDpadTurnSpeedMultiplier(void) {
    return 0.5f + barLevelToRatio(SETTINGS.dpadTurnSpeed);
}

float getAbsoluteCrankRatio(void) {
    switch (SETTINGS.crankRatio) {
        case CRANK_RATIO_HALF:
            return 0.5f;
        case CRANK_RATIO_THIRD:
            return 1.0f / 3.0f;
        default:
            return 1.0f;
    }
}

float getMusicVolumeSetting(void) {
    return barLevelToRatio(SETTINGS.musicVolume) * MUSIC_VOLUME_MAX;
}

float getSfxVolumeSetting(void) {
    return barLevelToRatio(SETTINGS.sfxVolume) * SFX_VOLUME_MAX;
}
