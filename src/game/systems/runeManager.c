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
    },
    [RUNE_TYPE_DAGGER] = {
        .nameEN = "Dagger",
        .nameJP = "",
        .descriptionEN = "Increases basic spell damage.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryValues = {3, 5, 8},
        .spellTarget = RUNE_SPELL_TARGET_BASIC,
        .applySpellModifierPrimary = modifySpellDamage
    },
    [RUNE_TYPE_BROADSWORD] = {
        .nameEN = "Broadsword",
        .nameJP = "",
        .descriptionEN = "Increases special spell damage.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryValues = {3, 5, 8},
        .spellTarget = RUNE_SPELL_TARGET_SPECIAL,
        .applySpellModifierPrimary = modifySpellDamage
    },
    [RUNE_TYPE_POCKET_WATCH] = {
        .nameEN = "Pocket Watch",
        .nameJP = "",
        .descriptionEN = "Reduces basic spell cooldown.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryValues = {-0.05f, -0.08f, -0.12f},
        .spellTarget = RUNE_SPELL_TARGET_BASIC,
        .applySpellModifierPrimary = modifySpellCooldown
    },
    [RUNE_TYPE_HOURGLASS] = {
        .nameEN = "Hourglass",
        .nameJP = "",
        .descriptionEN = "Reduces special spell cooldown.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryValues = {-0.5f, -1.0f, -1.5f},
        .spellTarget = RUNE_SPELL_TARGET_SPECIAL,
        .applySpellModifierPrimary = modifySpellCooldown
    },
    [RUNE_TYPE_SLING] = {
        .nameEN = "Sling",
        .nameJP = "",
        .descriptionEN = "Increases basic spell projectile speed.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryValues = {15, 25, 40},
        .spellTarget = RUNE_SPELL_TARGET_BASIC,
        .applySpellModifierPrimary = modifyProjectileSpeed
    },
    [RUNE_TYPE_LONGBOW] = {
        .nameEN = "Longbow",
        .nameJP = "",
        .descriptionEN = "Increases special spell projectile speed.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryValues = {15, 25, 40},
        .spellTarget = RUNE_SPELL_TARGET_SPECIAL,
        .applySpellModifierPrimary = modifyProjectileSpeed
    },
    [RUNE_TYPE_COPPER_PAN] = {
        .nameEN = "Copper Pan",
        .nameJP = "",
        .descriptionEN = "Basic spell hits knock enemies back further.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryValues   = {0.05f, 0.1f, 0.15f},
        .secondaryValues = {0.3f, 0.6f, 1.0f},
        .spellTarget = RUNE_SPELL_TARGET_BASIC,
        .applySpellModifierPrimary   = modifySpellKnockbackTime,
        .applySpellModifierSecondary = modifySpellKnockbackAmount
    },
    [RUNE_TYPE_IRON_GAUNTLET] = {
        .nameEN = "Iron Gauntlet",
        .nameJP = "",
        .descriptionEN = "Special spell hits knock enemies back further.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryValues   = {0.05f, 0.1f, 0.2f},
        .secondaryValues = {0.5f, 1.0f, 1.5f},
        .spellTarget = RUNE_SPELL_TARGET_SPECIAL,
        .applySpellModifierPrimary   = modifySpellKnockbackTime,
        .applySpellModifierSecondary = modifySpellKnockbackAmount
    },
    [RUNE_TYPE_FROST_SHARD] = {
        .nameEN = "Frost Shard",
        .nameJP = "",
        .descriptionEN = "Basic spell projectiles slow enemies on hit.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryValues   = {0.5f, 1.0f, 1.5f},
        .secondaryValues = {0.3f, 0.5f, 0.7f},
        .spellTarget = RUNE_SPELL_TARGET_BASIC,
        .applySpellModifierPrimary   = modifyProjectileSlowTime,
        .applySpellModifierSecondary = modifyProjectileSlowAmount
    },
    [RUNE_TYPE_ICE_LANCE] = {
        .nameEN = "Ice Lance",
        .nameJP = "",
        .descriptionEN = "Special spell projectiles slow enemies on hit.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryValues   = {0.5f, 1.0f, 1.5f},
        .secondaryValues = {0.3f, 0.5f, 0.7f},
        .spellTarget = RUNE_SPELL_TARGET_SPECIAL,
        .applySpellModifierPrimary   = modifyProjectileSlowTime,
        .applySpellModifierSecondary = modifyProjectileSlowAmount
    },
    [RUNE_TYPE_NEEDLE_AND_THREAD] = {
        .nameEN = "Needle and Thread",
        .nameJP = "",
        .descriptionEN = "Basic spell projectiles pierce through enemies.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryValues = {1, 2, 3},
        .spellTarget = RUNE_SPELL_TARGET_BASIC,
        .applySpellModifierPrimary = modifyProjectileHitCount
    },
    [RUNE_TYPE_HAMMER_AND_NAILS] = {
        .nameEN = "Hammer and Nails",
        .nameJP = "",
        .descriptionEN = "Special spell projectiles pierce through enemies.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryValues = {1, 2, 3},
        .spellTarget = RUNE_SPELL_TARGET_SPECIAL,
        .applySpellModifierPrimary = modifyProjectileHitCount
    },
    [RUNE_TYPE_QUARTZ_SHARD] = {
        .nameEN = "Quartz Shard",
        .nameJP = "",
        .descriptionEN = "Basic spell fires additional projectiles.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryValues   = {1, 2, 3},
        .secondaryValues = {5, 8, 10},
        .spellTarget = RUNE_SPELL_TARGET_BASIC,
        .applySpellModifierPrimary   = modifyProjectileSpreadCount,
        .applySpellModifierSecondary = modifyProjectileSpreadAngle
    },
    [RUNE_TYPE_QUARTZ_CLUSTER] = {
        .nameEN = "Quartz Cluster",
        .nameJP = "",
        .descriptionEN = "Special spell fires additional projectiles.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryValues   = {1, 2, 3},
        .secondaryValues = {5, 8, 10},
        .spellTarget = RUNE_SPELL_TARGET_SPECIAL,
        .applySpellModifierPrimary   = modifyProjectileSpreadCount,
        .applySpellModifierSecondary = modifyProjectileSpreadAngle
    },
    [RUNE_TYPE_ENCHANTED_BRANCH] = {
        .nameEN = "Enchanted Branch",
        .nameJP = "",
        .descriptionEN = "Basic repeat spells fire additional times.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryValues   = {1, 2, 3},
        .secondaryValues = {-0.02f, -0.03f, -0.05f},
        .spellTarget = RUNE_SPELL_TARGET_BASIC,
        .applySpellModifierPrimary   = modifyRepeatCount,
        .applySpellModifierSecondary = modifyRepeatActivationTime
    },
    [RUNE_TYPE_ARCANE_STAFF] = {
        .nameEN = "Arcane Staff",
        .nameJP = "",
        .descriptionEN = "Special repeat spells fire additional times.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryValues   = {1, 2, 3},
        .secondaryValues = {-0.02f, -0.03f, -0.05f},
        .spellTarget = RUNE_SPELL_TARGET_SPECIAL,
        .applySpellModifierPrimary   = modifyRepeatCount,
        .applySpellModifierSecondary = modifyRepeatActivationTime
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