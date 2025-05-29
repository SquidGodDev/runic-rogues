#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "game/settings/settings.h"

Settings SETTINGS;

void initSettings(void) {
    SETTINGS.language = LANGUAGE_EN;
}
