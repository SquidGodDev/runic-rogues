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
    SETTINGS.showTimer = false;
    SETTINGS.targetFps = FPS_SETTING_40;
}

void resetControlSettings(void) {
    SETTINGS.controlScheme = CONTROL_SCHEME_CRANK;
    SETTINGS.crankTurnSpeed = SETTING_CRANK_TURN_SPEED_DEFAULT;
    SETTINGS.dpadTurnSpeed = SETTING_BAR_DEFAULT_LEVEL;
    SETTINGS.dpadLeftRight = DPAD_LR_STRAFE;
    SETTINGS.absoluteCrank = false;
    SETTINGS.absoluteCrankOffset = 0;
    SETTINGS.crankRatio = CRANK_RATIO_1X;
    SETTINGS.invertCrank = false;
    SETTINGS.invertDpad = false;
    SETTINGS.spellButtonsReversed = false;
    SETTINGS.toggleFireBasic = false;
    SETTINGS.toggleFireSpecial = false;
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
    DEBUG.testFirstRoomEnemy = 0;
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

int getTargetFps(void) {
    switch (SETTINGS.targetFps) {
        case FPS_SETTING_30:
            return 30;
        case FPS_SETTING_50:
            return 50;
        default:
            return DEFAULT_TARGET_FPS;
    }
}

void applyTargetFps(void) {
    pd->display->setRefreshRate((float)getTargetFps());
}

static float barLevelToRatio(int level) {
    return (float)level / SETTING_BAR_MAX_LEVEL;
}

#define MUSIC_VOLUME_MAX 0.4f
#define SFX_VOLUME_MAX   1.0f

float getCrankTurnSpeedMultiplier(void) {
    // Exponential response so 50 keeps the original 1x feel while the extremes span
    // a wide sensitivity range: 0 is 8x slower, 100 is 8x faster (small crank
    // movements make big angle changes).
    float t = (float)(SETTINGS.crankTurnSpeed - SETTING_CRANK_TURN_SPEED_DEFAULT) / SETTING_CRANK_TURN_SPEED_DEFAULT;
    return powf(8.0f, t);
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
