#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "game/systems/runeManager.h"
#include "game/rendering/textures.h"
#include "game/settings/settings.h"
#include "game/player/spells/spells.h"
#include "game/player/classes/classData.h"

RuneData RUNE_DATA[RUNE_TYPE_COUNT] = {
    [RUNE_TYPE_VAMPIRISM] = {
        .nameEN = "Vampirism",
        .nameJP = "ヴァンパイリズム",
        .descriptionEN = "Gain health when an enemy is defeated.",
        .descriptionJP = "敵を倒すと体力を回復する。",
        .iconIndex = BITMAP_RUNE_VAMPIRISM,
        .textureIndex = TEXTURE_RUNE_VAMPIRISM,
        .primaryValues = {2, 4, 6} // Health gained
    },
    [RUNE_TYPE_WINGED_BOOTS] = {
        .nameEN = "Winged Boots",
        .nameJP = "ウィングブーツ",
        .descriptionEN = "Increase number of dashes and lowers dash cooldown.",
        .descriptionJP = "ダッシュの回数を増やし、ダッシュのクールダウン時間を短縮する。",
        .iconIndex = BITMAP_RUNE_WINGED_BOOTS,
        .textureIndex = TEXTURE_RUNE_WINGED_BOOTS,
        .primaryValues   = {0.5f, 0.75f, 1.0f}, // Dash charge rate increase %
        .secondaryValues = {1, 2, 3} // Dash count increase
    },
    [RUNE_TYPE_HEALTHY] = {
        .nameEN = "Healthy",
        .nameJP = "ヘルシー",
        .descriptionEN = "Increases maximum health.",
        .descriptionJP = "最大体力が増加する。",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryValues = {20, 35, 50} // Health increase
    },
    [RUNE_TYPE_SWIFT] = {
        .nameEN = "Swift",
        .nameJP = "スウィフト",
        .descriptionEN = "Increases movement speed.",
        .descriptionJP = "移動速度が増加する。",
        .iconIndex = BITMAP_RUNE_SWIFT,
        .textureIndex = TEXTURE_RUNE_SWIFT,
        .primaryValues = {0.3f, 0.5f, 0.7f} // Speed increase %
    }
};

RuneType runes[MAX_OWNED_RUNES];
int ownedRuneLevel[RUNE_TYPE_COUNT];

void initRunes(void) {
    for (int i = 0; i < MAX_OWNED_RUNES; i++) {
        runes[i] = RUNE_TYPE_NONE;
    }
    for (int i = 0; i < RUNE_TYPE_COUNT; i++) {
        ownedRuneLevel[i] = 0;
    }
}

bool runeIsOwned(RuneType type) {
    return ownedRuneLevel[type] > 0;
}

void addRune(RuneType type, int level) {
    for (int i = 0; i < MAX_OWNED_RUNES; i++) {
        if (runes[i] == RUNE_TYPE_NONE) {
            runes[i] = type;
            break;
        }
    }
    ownedRuneLevel[type] = level;
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

float getRuneValue(RuneType type) {
    int level = ownedRuneLevel[type];
    if (level == 0) {
        return 0;
    }
    return RUNE_DATA[type].primaryValues[level - 1];
}

float getRuneSecondaryValue(RuneType type) {
    int level = ownedRuneLevel[type];
    if (level == 0) {
        return 0;
    }
    return RUNE_DATA[type].secondaryValues[level - 1];
}

void applyRuneEffectsToSpells(void) {
    const ClassData *classData = getCurrentClassData();
    for (int i = 0; i < RUNE_TYPE_COUNT; i++) {
        RuneType runeType = (RuneType)i;
        int level = ownedRuneLevel[runeType];
        if (level == 0) {
            continue;
        }
        RuneData *rune = &RUNE_DATA[runeType];
        if (rune->spellTarget == RUNE_SPELL_TARGET_NONE) {
            continue;
        }
        SpellData *basicSpell   = &spellData[classData->basicSpell];
        SpellData *specialSpell = &spellData[classData->specialSpell];
        if (rune->applySpellModifierPrimary) {
            float value = rune->primaryValues[level - 1];
            if (rune->spellTarget == RUNE_SPELL_TARGET_BASIC || rune->spellTarget == RUNE_SPELL_TARGET_BOTH) {
                rune->applySpellModifierPrimary(basicSpell, value);
            }
            if (rune->spellTarget == RUNE_SPELL_TARGET_SPECIAL || rune->spellTarget == RUNE_SPELL_TARGET_BOTH) {
                rune->applySpellModifierPrimary(specialSpell, value);
            }
        }
        if (rune->applySpellModifierSecondary) {
            float value = rune->secondaryValues[level - 1];
            if (rune->spellTarget == RUNE_SPELL_TARGET_BASIC || rune->spellTarget == RUNE_SPELL_TARGET_BOTH) {
                rune->applySpellModifierSecondary(basicSpell, value);
            }
            if (rune->spellTarget == RUNE_SPELL_TARGET_SPECIAL || rune->spellTarget == RUNE_SPELL_TARGET_BOTH) {
                rune->applySpellModifierSecondary(specialSpell, value);
            }
        }
    }
}