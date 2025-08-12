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
} RuneData;

RuneData RUNE_DATA[RUNE_TYPE_COUNT] = {
    [RUNE_TYPE_VAMPIRISM] = {
        .nameEN = "Vampirism",
        .nameJP = "ヴァンパイリズム",
        .descriptionEN = "Gain health when damaging enemies.",
        .descriptionJP = "敵にダメージを与えると体力を回復する。",
        .iconIndex = BITMAP_RUNE_VAMPIRISM,
        .textureIndex = TEXTURE_RUNE_VAMPIRISM,
        .primaryValue = 0.2f
    },
    [RUNE_TYPE_WINGED_BOOTS] = {
        .nameEN = "Winged Boots",
        .nameJP = "ウィングブーツ",
        .descriptionEN = "Lowers dash cooldown.",
        .descriptionJP = "移動速度が増加する。",
        .iconIndex = BITMAP_RUNE_WINGED_BOOTS,
        .textureIndex = TEXTURE_RUNE_WINGED_BOOTS,
        .primaryValue = 0.3f
    },
    [RUNE_TYPE_HEALTHY] = {
        .nameEN = "Healthy",
        .nameJP = "ヘルシー",
        .descriptionEN = "Increases maximum health.",
        .descriptionJP = "最大体力が増加する。",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryValue = 2.0f
    },
    [RUNE_TYPE_SWIFT] = {
        .nameEN = "Swift",
        .nameJP = "スウィフト",
        .descriptionEN = "Increases movement speed.",
        .descriptionJP = "移動速度が増加する。",
        .iconIndex = BITMAP_RUNE_SWIFT,
        .textureIndex = TEXTURE_RUNE_SWIFT,
        .primaryValue = 0.2f
    },
    [RUNE_TYPE_THORNS] = {
        .nameEN = "Thorns",
        .nameJP = "ソーン",
        .descriptionEN = "Reflects damage back to enemies.",
        .descriptionJP = "敵にダメージを反射する。",
        .iconIndex = BITMAP_RUNE_THORNS,
        .textureIndex = TEXTURE_RUNE_THORNS,
        .primaryValue = 0.1f
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
