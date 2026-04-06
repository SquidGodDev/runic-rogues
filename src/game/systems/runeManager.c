#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "game/systems/runeManager.h"
#include "game/rendering/textures.h"
#include "game/settings/settings.h"
#include "game/player/spells/spells.h"
#include "game/player/classes/classData.h"
#include "game/systems/chainLightningManager.h"
#include "game/player/player.h"

static void thunderStoneCallback(float primaryValue, float secondaryValue) {
    startChainLightningOnNearestEnemy(playerX, playerY, CELL_SIZE * 5, (int)primaryValue, (int)secondaryValue);
}

RuneData RUNE_DATA[RUNE_TYPE_COUNT] = {
    [RUNE_TYPE_VAMPIRISM] = {
        .nameEN = "Vampirism",
        .nameJP = "ヴァンパイリズム",
        .descriptionEN = "Gain {1} health when an enemy is defeated.",
        .descriptionJP = "敵を倒すと体力を回復する。",
        .iconIndex = BITMAP_RUNE_VAMPIRISM,
        .textureIndex = TEXTURE_RUNE_VAMPIRISM,
        .primaryBaseValue = 2,
        .primaryPerLevel = 2,
        .maxLevel = 3
    },
    [RUNE_TYPE_WINGED_BOOTS] = {
        .nameEN = "Winged Boots",
        .nameJP = "ウィングブーツ",
        .descriptionEN = "Gain {2} extra dash charge(s) and lower dash cooldown.",
        .descriptionJP = "ダッシュの回数を増やし、ダッシュのクールダウン時間を短縮する。",
        .iconIndex = BITMAP_RUNE_WINGED_BOOTS,
        .textureIndex = TEXTURE_RUNE_WINGED_BOOTS,
        .primaryBaseValue   = 0.33f,
        .primaryPerLevel    = 0.33f,
        .secondaryBaseValue = 1,
        .secondaryPerLevel  = 1,
        .maxLevel = 3
    },
    [RUNE_TYPE_HEALTHY] = {
        .nameEN = "Healthy",
        .nameJP = "ヘルシー",
        .descriptionEN = "Increases maximum health by {1}.",
        .descriptionJP = "最大体力が増加する。",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryBaseValue = 15,
        .primaryPerLevel = 15,
        .maxLevel = 3
    },
    [RUNE_TYPE_SWIFT] = {
        .nameEN = "Swift",
        .nameJP = "スウィフト",
        .descriptionEN = "Increases movement speed by {1}%.",
        .descriptionJP = "移動速度が増加する。",
        .iconIndex = BITMAP_RUNE_SWIFT,
        .textureIndex = TEXTURE_RUNE_SWIFT,
        .primaryBaseValue = 0.25f,
        .primaryPerLevel = 0.25f,
        .maxLevel = 3,
        .primaryModifyOperation = MODIFY_OP_MULTIPLY
    },
    [RUNE_TYPE_DAGGER] = {
        .nameEN = "Dagger",
        .nameJP = "",
        .descriptionEN = "Basic spell deals {1} extra damage.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryBaseValue = 3,
        .primaryPerLevel = 3,
        .maxLevel = 3,
        .spellTarget = RUNE_SPELL_TARGET_BASIC,
        .primaryModifyOperation = MODIFY_OP_ADD,
        .applySpellModifierPrimary = modifySpellDamage
    },
    [RUNE_TYPE_BROADSWORD] = {
        .nameEN = "Broadsword",
        .nameJP = "",
        .descriptionEN = "Special spell deals {1} extra damage.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryBaseValue = 3,
        .primaryPerLevel = 3,
        .maxLevel = 3,
        .spellTarget = RUNE_SPELL_TARGET_SPECIAL,
        .primaryModifyOperation = MODIFY_OP_ADD,
        .applySpellModifierPrimary = modifySpellDamage
    },
    [RUNE_TYPE_POCKET_WATCH] = {
        .nameEN = "Pocket Watch",
        .nameJP = "",
        .descriptionEN = "Reduces basic spell cooldown by {1}%.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryBaseValue = -0.2f,
        .primaryPerLevel = -0.15f,
        .maxLevel = 3,
        .spellTarget = RUNE_SPELL_TARGET_BASIC,
        .primaryModifyOperation = MODIFY_OP_MULTIPLY,
        .applySpellModifierPrimary = modifySpellCooldown
    },
    [RUNE_TYPE_HOURGLASS] = {
        .nameEN = "Hourglass",
        .nameJP = "",
        .descriptionEN = "Reduces special spell cooldown by {1}%.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryBaseValue = -0.2f,
        .primaryPerLevel = -0.15f,
        .maxLevel = 3,
        .spellTarget = RUNE_SPELL_TARGET_SPECIAL,
        .primaryModifyOperation = MODIFY_OP_MULTIPLY,
        .applySpellModifierPrimary = modifySpellCooldown
    },
    [RUNE_TYPE_SLINGSHOT] = {
        .nameEN = "Slingshot",
        .nameJP = "",
        .descriptionEN = "Increases basic spell projectile speed by {1}%.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryBaseValue = 0.3f,
        .primaryPerLevel = 0.35f,
        .maxLevel = 3,
        .spellTarget = RUNE_SPELL_TARGET_BASIC,
        .primaryModifyOperation = MODIFY_OP_MULTIPLY,
        .applySpellModifierPrimary = modifyProjectileSpeed,
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE
    },
    [RUNE_TYPE_LONGBOW] = {
        .nameEN = "Longbow",
        .nameJP = "",
        .descriptionEN = "Increases special spell projectile speed by {1}%.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryBaseValue = 0.3f,
        .primaryPerLevel = 0.35f,
        .maxLevel = 3,
        .spellTarget = RUNE_SPELL_TARGET_SPECIAL,
        .primaryModifyOperation = MODIFY_OP_MULTIPLY,
        .applySpellModifierPrimary = modifyProjectileSpeed,
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE
    },
    [RUNE_TYPE_BEAR_PAW] = {
        .nameEN = "Bear Paw",
        .nameJP = "",
        .descriptionEN = "Basic spell hits knock enemies back further.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryBaseValue   = 0.15f,
        .primaryPerLevel    = 0.15f,
        .secondaryBaseValue = 0.35f,
        .secondaryPerLevel  = 0.35f,
        .maxLevel = 3,
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
        .primaryBaseValue   = 0.15f,
        .primaryPerLevel    = 0.15f,
        .secondaryBaseValue = 0.5f,
        .secondaryPerLevel  = 0.5f,
        .maxLevel = 3,
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
        .primaryBaseValue   = 0.5f,
        .primaryPerLevel    = 0.5f,
        .secondaryBaseValue = 0.25f,
        .secondaryPerLevel  = 0.25f,
        .maxLevel = 3,
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
        .primaryBaseValue   = 0.5f,
        .primaryPerLevel    = 0.5f,
        .secondaryBaseValue = 0.25f,
        .secondaryPerLevel  = 0.25f,
        .maxLevel = 3,
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
        .descriptionEN = "Basic spell projectiles pierce through {1} additional enemies.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryBaseValue = 1,
        .primaryPerLevel = 1,
        .maxLevel = 3,
        .spellTarget = RUNE_SPELL_TARGET_BASIC,
        .primaryModifyOperation = MODIFY_OP_ADD,
        .applySpellModifierPrimary = modifyProjectilePierceCount,
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE
    },
    [RUNE_TYPE_HAMMER_AND_NAILS] = {
        .nameEN = "Hammer and Nails",
        .nameJP = "",
        .descriptionEN = "Special spell projectiles pierce through {1} additional enemies.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryBaseValue = 1,
        .primaryPerLevel = 1,
        .maxLevel = 3,
        .spellTarget = RUNE_SPELL_TARGET_SPECIAL,
        .primaryModifyOperation = MODIFY_OP_ADD,
        .applySpellModifierPrimary = modifyProjectilePierceCount,
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE
    },
    [RUNE_TYPE_MAGNIFYING_GLASS] = {
        .nameEN = "Magnifying Glass",
        .nameJP = "",
        .descriptionEN = "Basic spell fires {1} additional projectiles in a spread.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryBaseValue   = 2,
        .primaryPerLevel    = 2,
        .secondaryBaseValue = 15 * (PI / 180),
        .maxLevel = 2,
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
        .descriptionEN = "Special spell fires {1} additional projectiles in a spread.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryBaseValue   = 2,
        .primaryPerLevel    = 2,
        .secondaryBaseValue = 15 * (PI / 180),
        .maxLevel = 2,
        .spellTarget = RUNE_SPELL_TARGET_SPECIAL,
        .primaryModifyOperation   = MODIFY_OP_ADD,
        .secondaryModifyOperation = MODIFY_OP_REPLACE_IF_ZERO,
        .applySpellModifierPrimary   = modifyProjectileSpreadCount,
        .applySpellModifierSecondary = modifyProjectileSpreadAngle,
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE
    },
    [RUNE_TYPE_ENCHANTED_AMULET] = {
        .nameEN = "Enchanted Amulet",
        .nameJP = "",
        .descriptionEN = "Basic spells fire {1} additional times.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryBaseValue   = 1,
        .primaryPerLevel    = 1,
        .secondaryBaseValue = 0.5f,
        .maxLevel = 3,
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
        .descriptionEN = "Special spells fire {1} additional times.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryBaseValue   = 1,
        .primaryPerLevel    = 1,
        .secondaryBaseValue = 0.5f,
        .maxLevel = 3,
        .spellTarget = RUNE_SPELL_TARGET_SPECIAL,
        .primaryModifyOperation   = MODIFY_OP_ADD,
        .secondaryModifyOperation = MODIFY_OP_REPLACE_IF_ZERO,
        .applySpellModifierPrimary   = modifyRepeatCount,
        .applySpellModifierSecondary = modifyRepeatActivationTime,
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE
    },
    [RUNE_TYPE_THUNDER_STONE] = {
        .nameEN = "Thunder Stone",
        .nameJP = "",
        .descriptionEN = "Strike a nearby enemy with lightning every {T}s for {1} damage, chaining to {2} enemies.",
        .descriptionJP = "",
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .textureIndex = TEXTURE_RUNE_HEALTHY,
        .primaryBaseValue   = 10, // damage
        .primaryPerLevel    = 0,
        .secondaryBaseValue = 1, // chain count
        .secondaryPerLevel  = 1,
        .maxLevel = 3,
        .triggerType = RUNE_TRIGGER_TIME,
        .triggerBaseValue = 5.0f,
        .triggerPerLevel  = 0.0f,
        .triggerCallback  = thunderStoneCallback
    }
};

int runeLevels[RUNE_TYPE_COUNT];
float runeTriggerCounters[RUNE_TYPE_COUNT];

void initRunes(void) {
    for (int i = 0; i < RUNE_TYPE_COUNT; i++) {
        runeLevels[i] = 0;
        runeTriggerCounters[i] = 0.0f;
    }

    // == Testing runes ==
    // setRuneLevel(RUNE_TYPE_VAMPIRISM, 1);
    // setRuneLevel(RUNE_TYPE_HEALTHY, 1);
    // setRuneLevel(RUNE_TYPE_SWIFT, 1);
    // setRuneLevel(RUNE_TYPE_DAGGER, 1);
    // setRuneLevel(RUNE_TYPE_BROADSWORD, 1);
    // setRuneLevel(RUNE_TYPE_POCKET_WATCH, 1);
    // setRuneLevel(RUNE_TYPE_HOURGLASS, 1);
    // setRuneLevel(RUNE_TYPE_SLINGSHOT, 1);
    // setRuneLevel(RUNE_TYPE_LONGBOW, 1);
    // setRuneLevel(RUNE_TYPE_BEAR_PAW, 1);
    // setRuneLevel(RUNE_TYPE_IRON_GAUNTLET, 1);
    // setRuneLevel(RUNE_TYPE_FROST_SHARD, 1);
    // setRuneLevel(RUNE_TYPE_ICE_LANCE, 1);
    // setRuneLevel(RUNE_TYPE_NEEDLE_AND_THREAD, 1);
    // setRuneLevel(RUNE_TYPE_HAMMER_AND_NAILS, 1);
    // setRuneLevel(RUNE_TYPE_MAGNIFYING_GLASS, 1);
    // setRuneLevel(RUNE_TYPE_QUARTZ_CLUSTER, 1);
    // setRuneLevel(RUNE_TYPE_ENCHANTED_AMULET, 1);
    // setRuneLevel(RUNE_TYPE_ARCANE_STAFF, 1);
    // setRuneLevel(RUNE_TYPE_THUNDER_STONE, 1);
}

bool runeIsOwned(RuneType type) {
    return runeLevels[type] > 0;
}

void addRune(RuneType type) {
    if (runeIsMaxLevel(type)) {
        return;
    }

    runeLevels[type]++;

    recalculateRuneEffectsToSpells();
}

void setRuneLevel(RuneType type, int level) {
    if (level < 0) {
        level = 0;
    }
    if (level > RUNE_DATA[type].maxLevel) {
        level = RUNE_DATA[type].maxLevel;
    }
    runeLevels[type] = level;

    recalculateRuneEffectsToSpells();
}

int getRuneLevel(RuneType type) {
    return runeLevels[type];
}

int getRuneMaxLevel(RuneType type) {
    return RUNE_DATA[type].maxLevel;
}

bool runeIsMaxLevel(RuneType type) {
    return runeLevels[type] >= RUNE_DATA[type].maxLevel;
}

char *getRuneName(RuneType type, Language language) {
    if (language == LANGUAGE_EN) {
        return RUNE_DATA[type].nameEN;
    } else {
        return RUNE_DATA[type].nameJP;
    }
}

static char descBufferEN[256];
static char descBufferJP[256];

static void strReplaceFirst(char *buf, const char *token, const char *replacement) {
    char *pos = strstr(buf, token);
    if (pos == NULL) {
        return;
    }
    size_t tokenLen = strlen(token);
    size_t repLen   = strlen(replacement);
    size_t restLen  = strlen(pos + tokenLen);
    memmove(pos + repLen, pos + tokenLen, restLen + 1);
    memcpy(pos, replacement, repLen);
}

char *getRuneDescription(RuneType type, Language language, int runeLevel) {
    const RuneData *rune = &RUNE_DATA[type];
    const char *tmpl = (language == LANGUAGE_EN) ? rune->descriptionEN : rune->descriptionJP;
    char *buf        = (language == LANGUAGE_EN) ? descBufferEN : descBufferJP;

    if (strstr(tmpl, "{1}") == NULL && strstr(tmpl, "{2}") == NULL && strstr(tmpl, "{T}") == NULL) {
        return (char *)tmpl;
    }

    strncpy(buf, tmpl, 255);
    buf[255] = '\0';

    if (runeLevel < 1) {
        runeLevel = 1;
    }

    if (strstr(buf, "{1}") != NULL) {
        float pv = fabsf(rune->primaryBaseValue + rune->primaryPerLevel * (runeLevel - 1));
        char *pvStr = NULL;
        if (rune->primaryModifyOperation == MODIFY_OP_MULTIPLY) {
            pd->system->formatString(&pvStr, "%d", (int)roundf(pv * 100));
        } else if ((float)(int)pv == pv) {
            pd->system->formatString(&pvStr, "%d", (int)pv);
        } else {
            int tenths = (int)roundf(pv * 10);
            pd->system->formatString(&pvStr, "%d.%d", tenths / 10, tenths % 10);
        }
        strReplaceFirst(buf, "{1}", pvStr);
        pd->system->realloc(pvStr, 0);
    }

    if (strstr(buf, "{2}") != NULL) {
        float sv = fabsf(rune->secondaryBaseValue + rune->secondaryPerLevel * (runeLevel - 1));
        char *svStr = NULL;
        if (rune->secondaryModifyOperation == MODIFY_OP_MULTIPLY) {
            pd->system->formatString(&svStr, "%d", (int)roundf(sv * 100));
        } else if ((float)(int)sv == sv) {
            pd->system->formatString(&svStr, "%d", (int)sv);
        } else {
            int tenths = (int)roundf(sv * 10);
            pd->system->formatString(&svStr, "%d.%d", tenths / 10, tenths % 10);
        }
        strReplaceFirst(buf, "{2}", svStr);
        pd->system->realloc(svStr, 0);
    }

    if (strstr(buf, "{T}") != NULL) {
        float tv = fabsf(rune->triggerBaseValue + rune->triggerPerLevel * (runeLevel - 1));
        char *tvStr = NULL;
        if ((float)(int)tv == tv) {
            pd->system->formatString(&tvStr, "%d", (int)tv);
        } else {
            int tenths = (int)roundf(tv * 10);
            pd->system->formatString(&tvStr, "%d.%d", tenths / 10, tenths % 10);
        }
        strReplaceFirst(buf, "{T}", tvStr);
        pd->system->realloc(tvStr, 0);
    }

    return buf;
}

int getRuneIconIndex(RuneType type) {
    return RUNE_DATA[type].iconIndex;
}

int getRuneTextureIndex(RuneType type) {
    return RUNE_DATA[type].textureIndex;
}

RuneClassFilter getRuneClassFilter(RuneType type) {
    return RUNE_DATA[type].classFilter;
}

float getRuneValue(RuneType type) {
    int level = runeLevels[type];
    if (level <= 0) {
        return 0;
    }
    return RUNE_DATA[type].primaryBaseValue + RUNE_DATA[type].primaryPerLevel * (level - 1);
}

float getRuneSecondaryValue(RuneType type) {
    int level = runeLevels[type];
    if (level <= 0) {
        return 0;
    }
    return RUNE_DATA[type].secondaryBaseValue + RUNE_DATA[type].secondaryPerLevel * (level - 1);
}

void applyRuneEffectsToSpells(void) {
    const ClassData *classData = getCurrentClassData();
    for (int i = 0; i < RUNE_TYPE_COUNT; i++) {
        RuneType runeType = (RuneType)i;
        int level = runeLevels[runeType];
        if (level <= 0) {
            continue;
        }
        RuneData *rune = &RUNE_DATA[runeType];
        if (rune->spellTarget == RUNE_SPELL_TARGET_NONE) {
            continue;
        }
        SpellData *basicSpell   = &spellData[classData->basicSpell];
        SpellData *specialSpell = &spellData[classData->specialSpell];
        if (rune->applySpellModifierPrimary) {
            float value = rune->primaryBaseValue + rune->primaryPerLevel * (level - 1);
            if (rune->spellTarget == RUNE_SPELL_TARGET_BASIC || rune->spellTarget == RUNE_SPELL_TARGET_BOTH) {
                rune->applySpellModifierPrimary(basicSpell, value, rune->primaryModifyOperation);
            }
            if (rune->spellTarget == RUNE_SPELL_TARGET_SPECIAL || rune->spellTarget == RUNE_SPELL_TARGET_BOTH) {
                rune->applySpellModifierPrimary(specialSpell, value, rune->primaryModifyOperation);
            }
        }
        if (rune->applySpellModifierSecondary) {
            float value = rune->secondaryBaseValue + rune->secondaryPerLevel * (level - 1);
            if (rune->spellTarget == RUNE_SPELL_TARGET_BASIC || rune->spellTarget == RUNE_SPELL_TARGET_BOTH) {
                rune->applySpellModifierSecondary(basicSpell, value, rune->secondaryModifyOperation);
            }
            if (rune->spellTarget == RUNE_SPELL_TARGET_SPECIAL || rune->spellTarget == RUNE_SPELL_TARGET_BOTH) {
                rune->applySpellModifierSecondary(specialSpell, value, rune->secondaryModifyOperation);
            }
        }
    }
}

void signalRuneTrigger(RuneTriggerType type, float amount) {
    if (type == RUNE_TRIGGER_NONE) {
        return;
    }
    for (int i = 0; i < RUNE_TYPE_COUNT; i++) {
        int level = runeLevels[i];
        if (level <= 0) {
            continue;
        }
        RuneData *rune = &RUNE_DATA[i];
        if (rune->triggerType != type) {
            continue;
        }
        if (rune->triggerCallback == NULL) {
            continue;
        }
        float threshold = rune->triggerBaseValue + rune->triggerPerLevel * (level - 1);
        if (threshold <= 0.0f) {
            continue;
        }
        runeTriggerCounters[i] += amount;
        if (runeTriggerCounters[i] >= threshold) {
            runeTriggerCounters[i] -= threshold;
            float primaryValue = rune->primaryBaseValue + rune->primaryPerLevel * (level - 1);
            float secondaryValue = rune->secondaryBaseValue + rune->secondaryPerLevel * (level - 1);
            rune->triggerCallback(primaryValue, secondaryValue);
        }
    }
}