#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "game/systems/runeManager.h"
#include "game/rendering/textures.h"
#include "game/settings/settings.h"

typedef struct RuneData {
    char *nameEN;
    char *nameJP;
    char *descriptionEN;
    char *descriptionJP;
    int iconIndex;
    int textureIndex;
    float primaryValue;
    float secondaryValue;
} RuneData;

RuneData RUNE_DATA[RUNE_TYPE_COUNT] = {
    [RUNE_TYPE_VAMPIRISM] = {
        .nameEN = "Vampirism",
        .nameJP = "ヴァンパイリズム",
        .descriptionEN = "Gain health when an enemy is defeated.",
        .descriptionJP = "敵を倒すと体力を回復する。",
        .iconIndex = BITMAP_RUNE_VAMPIRISM,
        .textureIndex = TEXTURE_RUNE_VAMPIRISM,
        .primaryValue = 2 // Heal for 2 hp when defeating enemies
    },
    [RUNE_TYPE_WINGED_BOOTS] = {
        .nameEN = "Winged Boots",
        .nameJP = "ウィングブーツ",
        .descriptionEN = "Increase number of dashes and lowers dash cooldown.",
        .descriptionJP = "ダッシュの回数を増やし、ダッシュのクールダウン時間を短縮する。",
        .iconIndex = BITMAP_RUNE_WINGED_BOOTS,
        .textureIndex = TEXTURE_RUNE_WINGED_BOOTS,
        .primaryValue = 0.5f, // Increases dash charge rate by 50%,
        .secondaryValue = 1 // Increases number of dashes by 1
    },
    [RUNE_TYPE_HEALTHY] = {
        .nameEN = "Healthy",
        .nameJP = "ヘルシー",
        .descriptionEN = "Increases maximum health.",
        .descriptionJP = "最大体力が増加する。",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryValue = 20 // Increases max health by 20
    },
    [RUNE_TYPE_SWIFT] = {
        .nameEN = "Swift",
        .nameJP = "スウィフト",
        .descriptionEN = "Increases movement speed.",
        .descriptionJP = "移動速度が増加する。",
        .iconIndex = BITMAP_RUNE_SWIFT,
        .textureIndex = TEXTURE_RUNE_SWIFT,
        .primaryValue = 0.3f // Increases move speed by 30%
    }
};

typedef struct Rune {
    bool owned;
} Rune;

Rune runes[RUNE_TYPE_COUNT];

void initRunes(void) {
    for (int i = 0; i < RUNE_TYPE_COUNT; i++) {
        runes[i].owned = false;
    }
}

bool runeIsOwned(RuneType type) {
    return runes[type].owned;
}

void addRune(RuneType type) {
    runes[type].owned = true;
}

char *getRuneName(RuneType type, Language language) {
    if (language == LANGUAGE_EN) {
        return RUNE_DATA[type].nameEN;
    } else {
        return RUNE_DATA[type].nameJP;
    }
}

char *getRuneDescription(RuneType type, Language language) {
    if (language == LANGUAGE_EN) {
        return RUNE_DATA[type].descriptionEN;
    } else {
        return RUNE_DATA[type].descriptionJP;
    }
}

int getRuneIconIndex(RuneType type) {
    return RUNE_DATA[type].iconIndex;
}

int getRuneTextureIndex(RuneType type) {
    return RUNE_DATA[type].textureIndex;
}

float getRunePrimaryValue(RuneType type) {
    return RUNE_DATA[type].primaryValue;
}

float getRuneSecondaryValue(RuneType type) {
    return RUNE_DATA[type].secondaryValue;
}