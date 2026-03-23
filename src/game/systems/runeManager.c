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
        .primaryValue = 6 // Health gained
    },
    [RUNE_TYPE_WINGED_BOOTS] = {
        .nameEN = "Winged Boots",
        .nameJP = "ウィングブーツ",
        .descriptionEN = "Increase number of dashes and lowers dash cooldown.",
        .descriptionJP = "ダッシュの回数を増やし、ダッシュのクールダウン時間を短縮する。",
        .iconIndex = BITMAP_RUNE_WINGED_BOOTS,
        .textureIndex = TEXTURE_RUNE_WINGED_BOOTS,
        .primaryValue   = 1.0f, // Dash charge rate increase %
        .secondaryValue = 3 // Dash count increase
    },
    [RUNE_TYPE_HEALTHY] = {
        .nameEN = "Healthy",
        .nameJP = "ヘルシー",
        .descriptionEN = "Increases maximum health.",
        .descriptionJP = "最大体力が増加する。",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryValue = 50 // Health increase
    },
    [RUNE_TYPE_SWIFT] = {
        .nameEN = "Swift",
        .nameJP = "スウィフト",
        .descriptionEN = "Increases movement speed.",
        .descriptionJP = "移動速度が増加する。",
        .iconIndex = BITMAP_RUNE_SWIFT,
        .textureIndex = TEXTURE_RUNE_SWIFT,
        .primaryValue = 0.7f // Speed increase %
    },
    [RUNE_TYPE_DAGGER] = {
        .nameEN = "Dagger",
        .nameJP = "",
        .descriptionEN = "Increases basic spell damage.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryValue = 8,
        .spellTarget = RUNE_SPELL_TARGET_BASIC,
        .primaryModifyOperation = MODIFY_OP_ADD,
        .applySpellModifierPrimary = modifySpellDamage
    },
    [RUNE_TYPE_BROADSWORD] = {
        .nameEN = "Broadsword",
        .nameJP = "",
        .descriptionEN = "Increases special spell damage.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryValue = 8,
        .spellTarget = RUNE_SPELL_TARGET_SPECIAL,
        .primaryModifyOperation = MODIFY_OP_ADD,
        .applySpellModifierPrimary = modifySpellDamage
    },
    [RUNE_TYPE_POCKET_WATCH] = {
        .nameEN = "Pocket Watch",
        .nameJP = "",
        .descriptionEN = "Reduces basic spell cooldown.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryValue = 0.5f, // 50% cooldown reduction
        .spellTarget = RUNE_SPELL_TARGET_BASIC,
        .primaryModifyOperation = MODIFY_OP_MULTIPLY,
        .applySpellModifierPrimary = modifySpellCooldown
    },
    [RUNE_TYPE_HOURGLASS] = {
        .nameEN = "Hourglass",
        .nameJP = "",
        .descriptionEN = "Reduces special spell cooldown.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryValue = 0.5f, // 50% cooldown reduction
        .spellTarget = RUNE_SPELL_TARGET_SPECIAL,
        .primaryModifyOperation = MODIFY_OP_MULTIPLY,
        .applySpellModifierPrimary = modifySpellCooldown
    },
    [RUNE_TYPE_SLING] = {
        .nameEN = "Sling",
        .nameJP = "",
        .descriptionEN = "Increases basic spell projectile speed.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryValue = 2.0f, // 2x projectile speed increase
        .spellTarget = RUNE_SPELL_TARGET_BASIC,
        .primaryModifyOperation = MODIFY_OP_MULTIPLY,
        .applySpellModifierPrimary = modifyProjectileSpeed,
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE
    },
    [RUNE_TYPE_LONGBOW] = {
        .nameEN = "Longbow",
        .nameJP = "",
        .descriptionEN = "Increases special spell projectile speed.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryValue = 2.0f, // 2x projectile speed increase
        .spellTarget = RUNE_SPELL_TARGET_SPECIAL,
        .primaryModifyOperation = MODIFY_OP_MULTIPLY,
        .applySpellModifierPrimary = modifyProjectileSpeed,
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE
    },
    [RUNE_TYPE_COPPER_PAN] = {
        .nameEN = "Copper Pan",
        .nameJP = "",
        .descriptionEN = "Basic spell hits knock enemies back further.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryValue   = 0.4f, // Knockback time increase
        .secondaryValue = 1.0f, // Knockback amount increase
        .spellTarget = RUNE_SPELL_TARGET_BASIC,
        .primaryModifyOperation   = MODIFY_OP_ADD,
        .secondaryModifyOperation = MODIFY_OP_ADD,
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
        .primaryValue   = 0.4f, // Knockback time increase
        .secondaryValue = 1.5f, // Knockback amount increase
        .spellTarget = RUNE_SPELL_TARGET_SPECIAL,
        .primaryModifyOperation   = MODIFY_OP_ADD,
        .secondaryModifyOperation = MODIFY_OP_ADD,
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
        .primaryValue   = 1.5f, // Slow time increase
        .secondaryValue = 0.7f, // Slow amount increase
        .spellTarget = RUNE_SPELL_TARGET_BASIC,
        .primaryModifyOperation   = MODIFY_OP_ADD,
        .secondaryModifyOperation = MODIFY_OP_ADD,
        .applySpellModifierPrimary   = modifySlowTime,
        .applySpellModifierSecondary = modifySlowAmount,
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE
    },
    [RUNE_TYPE_ICE_LANCE] = {
        .nameEN = "Ice Lance",
        .nameJP = "",
        .descriptionEN = "Special spell projectiles slow enemies on hit.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryValue   = 1.5f, // Slow time increase
        .secondaryValue = 0.7f, // Slow amount increase
        .spellTarget = RUNE_SPELL_TARGET_SPECIAL,
        .primaryModifyOperation   = MODIFY_OP_ADD,
        .secondaryModifyOperation = MODIFY_OP_ADD,
        .applySpellModifierPrimary   = modifySlowTime,
        .applySpellModifierSecondary = modifySlowAmount,
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE
    },
    [RUNE_TYPE_NEEDLE_AND_THREAD] = {
        .nameEN = "Needle and Thread",
        .nameJP = "",
        .descriptionEN = "Basic spell projectiles pierce through enemies.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryValue = 3,
        .spellTarget = RUNE_SPELL_TARGET_BASIC,
        .primaryModifyOperation = MODIFY_OP_ADD,
        .applySpellModifierPrimary = modifyProjectileHitCount,
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE
    },
    [RUNE_TYPE_HAMMER_AND_NAILS] = {
        .nameEN = "Hammer and Nails",
        .nameJP = "",
        .descriptionEN = "Special spell projectiles pierce through enemies.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryValue = 3,
        .spellTarget = RUNE_SPELL_TARGET_SPECIAL,
        .primaryModifyOperation = MODIFY_OP_ADD,
        .applySpellModifierPrimary = modifyProjectileHitCount,
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE
    },
    [RUNE_TYPE_QUARTZ_SHARD] = {
        .nameEN = "Quartz Shard",
        .nameJP = "",
        .descriptionEN = "Basic spell fires additional spread of projectiles.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryValue   = 2, // Additional projectile count
        .secondaryValue = 15 * (PI / 180), // Set spread angle in radians
        .spellTarget = RUNE_SPELL_TARGET_BASIC,
        .primaryModifyOperation   = MODIFY_OP_ADD,
        .secondaryModifyOperation = MODIFY_OP_REPLACE_IF_ZERO,
        .applySpellModifierPrimary   = modifyProjectileSpreadCount,
        .applySpellModifierSecondary = modifyProjectileSpreadAngle,
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE
    },
    [RUNE_TYPE_QUARTZ_CLUSTER] = {
        .nameEN = "Quartz Cluster",
        .nameJP = "",
        .descriptionEN = "Special spell fires additional spread of projectiles.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryValue   = 2, // Additional projectile count
        .secondaryValue = 15 * (PI / 180), // Set spread angle in radians
        .spellTarget = RUNE_SPELL_TARGET_SPECIAL,
        .primaryModifyOperation   = MODIFY_OP_ADD,
        .secondaryModifyOperation = MODIFY_OP_REPLACE_IF_ZERO,
        .applySpellModifierPrimary   = modifyProjectileSpreadCount,
        .applySpellModifierSecondary = modifyProjectileSpreadAngle,
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE
    },
    [RUNE_TYPE_ENCHANTED_BRANCH] = {
        .nameEN = "Enchanted Branch",
        .nameJP = "",
        .descriptionEN = "Basic spells fire additional times.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryValue   = 2, // Repeat count increase
        .secondaryValue = 0.5f, // Repeat activation time set
        .spellTarget = RUNE_SPELL_TARGET_BASIC,
        .primaryModifyOperation   = MODIFY_OP_ADD,
        .secondaryModifyOperation = MODIFY_OP_REPLACE_IF_ZERO,
        .applySpellModifierPrimary   = modifyRepeatCount,
        .applySpellModifierSecondary = modifyRepeatActivationTime,
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE
    },
    [RUNE_TYPE_ARCANE_STAFF] = {
        .nameEN = "Arcane Staff",
        .nameJP = "",
        .descriptionEN = "Special spells fire additional times.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryValue   = 2, // Repeat count increase
        .secondaryValue = 0.5f, // Repeat activation time set
        .spellTarget = RUNE_SPELL_TARGET_SPECIAL,
        .primaryModifyOperation   = MODIFY_OP_ADD,
        .secondaryModifyOperation = MODIFY_OP_REPLACE_IF_ZERO,
        .applySpellModifierPrimary   = modifyRepeatCount,
        .applySpellModifierSecondary = modifyRepeatActivationTime,
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE
    }
};

bool ownedRunes[RUNE_TYPE_COUNT];

void initRunes(void) {
    for (int i = 0; i < RUNE_TYPE_COUNT; i++) {
        ownedRunes[i] = false;
    }

    // == Testing runes ==
    // addRune(RUNE_TYPE_DAGGER);
    // addRune(RUNE_TYPE_BROADSWORD);
    // addRune(RUNE_TYPE_POCKET_WATCH);
    // addRune(RUNE_TYPE_HOURGLASS);
    // addRune(RUNE_TYPE_SLING);
    // addRune(RUNE_TYPE_LONGBOW);
    // addRune(RUNE_TYPE_COPPER_PAN);
    // addRune(RUNE_TYPE_IRON_GAUNTLET);
    // addRune(RUNE_TYPE_FROST_SHARD);
    // addRune(RUNE_TYPE_ICE_LANCE);
    // addRune(RUNE_TYPE_NEEDLE_AND_THREAD);
    // addRune(RUNE_TYPE_HAMMER_AND_NAILS);
    addRune(RUNE_TYPE_QUARTZ_SHARD);
    addRune(RUNE_TYPE_QUARTZ_CLUSTER);
    // addRune(RUNE_TYPE_ENCHANTED_BRANCH);
    // addRune(RUNE_TYPE_ARCANE_STAFF);

}

bool runeIsOwned(RuneType type) {
    return ownedRunes[type];
}

void addRune(RuneType type) {
    if (runeIsOwned(type)) {
        return;
    }

    ownedRunes[type] = true;

    recalculateRuneEffectsToSpells();
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
    if (!ownedRunes[type]) {
        return 0;
    }
    return RUNE_DATA[type].primaryValue;
}

float getRuneSecondaryValue(RuneType type) {
    if (!ownedRunes[type]) {
        return 0;
    }
    return RUNE_DATA[type].secondaryValue;
}

void applyRuneEffectsToSpells(void) {
    const ClassData *classData = getCurrentClassData();
    for (int i = 0; i < RUNE_TYPE_COUNT; i++) {
        RuneType runeType = (RuneType)i;
        if (!ownedRunes[runeType]) {
            continue;
        }
        RuneData *rune = &RUNE_DATA[runeType];
        if (rune->spellTarget == RUNE_SPELL_TARGET_NONE) {
            continue;
        }
        SpellData *basicSpell   = &spellData[classData->basicSpell];
        SpellData *specialSpell = &spellData[classData->specialSpell];
        if (rune->applySpellModifierPrimary) {
            float value = rune->primaryValue;
            if (rune->spellTarget == RUNE_SPELL_TARGET_BASIC || rune->spellTarget == RUNE_SPELL_TARGET_BOTH) {
                rune->applySpellModifierPrimary(basicSpell, value, rune->primaryModifyOperation);
            }
            if (rune->spellTarget == RUNE_SPELL_TARGET_SPECIAL || rune->spellTarget == RUNE_SPELL_TARGET_BOTH) {
                rune->applySpellModifierPrimary(specialSpell, value, rune->primaryModifyOperation);
            }
        }
        if (rune->applySpellModifierSecondary) {
            float value = rune->secondaryValue;
            if (rune->spellTarget == RUNE_SPELL_TARGET_BASIC || rune->spellTarget == RUNE_SPELL_TARGET_BOTH) {
                rune->applySpellModifierSecondary(basicSpell, value, rune->secondaryModifyOperation);
            }
            if (rune->spellTarget == RUNE_SPELL_TARGET_SPECIAL || rune->spellTarget == RUNE_SPELL_TARGET_BOTH) {
                rune->applySpellModifierSecondary(specialSpell, value, rune->secondaryModifyOperation);
            }
        }
    }
}