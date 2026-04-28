#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "game/settings/settings.h"

Settings SETTINGS;

void initSettings(void) {
    SETTINGS = (Settings) {
        .language = LANGUAGE_EN,
        .drawMinimapWalls = true,
        .spellButtonsReversed = false,
        .drawFPS = true
    };
}
