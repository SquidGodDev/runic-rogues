#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "game/settings/settings.h"

Settings SETTINGS;

void initSettings(void) {
    SETTINGS = (Settings) {
        .language = LANGUAGE_EN,
        .spellButtonsReversed = false,
        .drawFPS = true,
        // 
        // - Language [enum] (EN, JP)
        // - Turn speed [int] (d-pad, crank)
        // - Turn acceleration [int] (d-pad, crank)
        // - Strafing enabled [bool] (crank)
        // - Absolute angle [bool] (crank)
        // - Swap spell controls [bool]
        // - Inverse controls [bool]
        // - Music Volume [int]
        // - SFX Volume [int]
        .drawMinimapWalls = true,
    };
}
