#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "game/systems/runeManager.h"
#include "globals/helpers.h"
#include "game/rendering/textures.h"
#include "game/settings/settings.h"
#include "globals/localization.h"
#include "game/player/spells/spells.h"
#include "game/player/classes/classData.h"
#include "game/systems/chainLightningManager.h"
#include "game/systems/timerManager.h"
#include "game/player/player.h"
#include "game/entities/projectiles.h"
#include "game/rendering/particles.h"

#define DRAGONS_BREATH_SPEED 120.0f
#define DRAGONS_BREATH_DAMAGE 8
#define DRAGONS_BREATH_PROJECTILE_COUNT 8
#define DRAGONS_BREATH_CONE_ANGLE (30.0f * PI / 180.0f)
#define DRAGONS_BREATH_INTERVAL 0.1f

static void thunderStoneCallback(float primaryValue, float secondaryValue) {
    startChainLightningOnNearestEnemy(playerX, playerY, CELL_SIZE * 5, (int)primaryValue, (int)secondaryValue);
}

static void meditationCallback(float primaryValue, float secondaryValue) {
    healPlayer(primaryValue);
}

static void bomberCallback(float primaryValue, float secondaryValue) {
    (void)primaryValue;
    (void)secondaryValue;
    spawnParticle(PARTICLE_TYPE_BOMB, playerX, playerY, playerX, playerY, PARTICLE_TYPE_NONE, false /*enemyParticle*/);
}

static void fireDragonsBreath(void) {
    SpellProjectileData data = {
        .speed = DRAGONS_BREATH_SPEED,
        .textureIndex = TEXTURE_FIREBALL,
        .frameCount = 4,
        .frameTime = 0.09f
    };
    // Random angle within a cone in front of the player
    float angle = playerAngle + randFloat(-0.5f, 0.5f) * DRAGONS_BREATH_CONE_ANGLE;
    float dx = cosf(angle);
    float dy = sinf(angle);
    float startX = playerX + dx * (CELL_SIZE / 2);
    float startY = playerY + dy * (CELL_SIZE / 2);
    firePlayerProjectile(startX, startY, dx, dy, DRAGONS_BREATH_DAMAGE, data);
    playSound(SFX_FIREBALL);
}

static void dragonsBreathTimerCallback(int repeatCount, void *userData) {
    (void)repeatCount;
    (void)userData;
    fireDragonsBreath();
}

static void dragonsBreathCallback(float primaryValue, float secondaryValue) {
    (void)primaryValue;
    (void)secondaryValue;
    // Spray the fireballs one at a time, staggered over DRAGONS_BREATH_INTERVAL
    fireDragonsBreath();
    addTimer(DRAGONS_BREATH_INTERVAL, DRAGONS_BREATH_PROJECTILE_COUNT - 1, dragonsBreathTimerCallback, NULL);
}

static void secondWindCallback(float primaryValue, float secondaryValue) {
    (void)primaryValue;
    (void)secondaryValue;
    grantUtilitySpellCharge();
}

static void flywheelCallback(float primaryValue, float secondaryValue) {
    (void)secondaryValue;
    reduceSpecialSpellCooldown(primaryValue);
}

// Name and description strings are loaded from localization.csv into the
// nameEN/nameJP/descriptionEN/descriptionJP fields by initRuneLocalization(),
// keyed off RUNE_LOC_IDS below (rune.<id>.name and rune.<id>.desc).
RuneData RUNE_DATA[RUNE_TYPE_COUNT] = {
    [RUNE_TYPE_VAMPIRISM] = {
        .iconIndex = BITMAP_RUNE_VAMPIRISM,
        .primaryBaseValue = 0.1f,
        .primaryPerLevel = 0.1f,
        .maxLevel = 3,
        .primaryModifyOperation = MODIFY_OP_MULTIPLY
    },
    [RUNE_TYPE_WINGED_BOOTS] = {
        .iconIndex = BITMAP_RUNE_WINGED_BOOTS,
        .primaryBaseValue   = 0.33f,
        .primaryPerLevel    = 0.33f,
        .secondaryBaseValue = 1,
        .secondaryPerLevel  = 1,
        .maxLevel = 3
    },
    [RUNE_TYPE_HEALTHY] = {
        .iconIndex = BITMAP_RUNE_HEALTHY,
        .primaryBaseValue = 15,
        .primaryPerLevel = 15,
        .maxLevel = 3
    },
    [RUNE_TYPE_SWIFT] = {
        .iconIndex = BITMAP_RUNE_SWIFT,
        .primaryBaseValue = 0.25f,
        .primaryPerLevel = 0.25f,
        .maxLevel = 3,
        .primaryModifyOperation = MODIFY_OP_MULTIPLY
    },
    [RUNE_TYPE_DAGGER] = {
        .iconIndex = BITMAP_RUNE_DAGGER,
        .primaryBaseValue = 3,
        .primaryPerLevel = 3,
        .secondaryBaseValue = 10,
        .secondaryPerLevel = 10,
        .maxLevel = 3,
        .spellTarget = RUNE_SPELL_TARGET_BASIC,
        .primaryModifyOperation = MODIFY_OP_ADD,
        .applySpellModifierPrimary = modifySpellDamage
    },
    [RUNE_TYPE_BROADSWORD] = {
        .iconIndex = BITMAP_RUNE_BROADSWORD,
        .primaryBaseValue = 3,
        .primaryPerLevel = 3,
        .secondaryBaseValue = 10,
        .secondaryPerLevel = 10,
        .maxLevel = 3,
        .spellTarget = RUNE_SPELL_TARGET_SPECIAL,
        .primaryModifyOperation = MODIFY_OP_ADD,
        .applySpellModifierPrimary = modifySpellDamage
    },
    [RUNE_TYPE_POCKET_WATCH] = {
        .iconIndex = BITMAP_RUNE_POCKET_WATCH,
        .primaryBaseValue = -0.2f,
        .primaryPerLevel = -0.15f,
        .maxLevel = 3,
        .spellTarget = RUNE_SPELL_TARGET_BASIC,
        .primaryModifyOperation = MODIFY_OP_MULTIPLY,
        .applySpellModifierPrimary = modifySpellCooldown
    },
    [RUNE_TYPE_HOURGLASS] = {
        .iconIndex = BITMAP_RUNE_HOURGLASS,
        .primaryBaseValue = -0.2f,
        .primaryPerLevel = -0.15f,
        .maxLevel = 3,
        .spellTarget = RUNE_SPELL_TARGET_SPECIAL,
        .primaryModifyOperation = MODIFY_OP_MULTIPLY,
        .applySpellModifierPrimary = modifySpellCooldown
    },
    [RUNE_TYPE_SLINGSHOT] = {
        .iconIndex = BITMAP_RUNE_SLINGSHOT,
        .primaryBaseValue = 0.3f,
        .primaryPerLevel = 0.35f,
        .maxLevel = 3,
        .spellTarget = RUNE_SPELL_TARGET_BASIC,
        .primaryModifyOperation = MODIFY_OP_MULTIPLY,
        .applySpellModifierPrimary = modifyProjectileSpeed,
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE | RUNE_CLASS_FILTER_RANGER
    },
    [RUNE_TYPE_LONGBOW] = {
        .iconIndex = BITMAP_RUNE_LONGBOW,
        .primaryBaseValue = 0.3f,
        .primaryPerLevel = 0.35f,
        .maxLevel = 3,
        .spellTarget = RUNE_SPELL_TARGET_SPECIAL,
        .primaryModifyOperation = MODIFY_OP_MULTIPLY,
        .applySpellModifierPrimary = modifyProjectileSpeed,
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE | RUNE_CLASS_FILTER_RANGER
    },
    [RUNE_TYPE_BEAR_PAW] = {
        .iconIndex = BITMAP_RUNE_BEAR_PAW,
        .primaryBaseValue   = 0.5f,
        .primaryPerLevel    = 0.5f,
        .secondaryBaseValue = 0.5f,
        .secondaryPerLevel  = 0.5f,
        .maxLevel = 3,
        .spellTarget = RUNE_SPELL_TARGET_BASIC,
        .primaryModifyOperation   = MODIFY_OP_ADD,
        .secondaryModifyOperation = MODIFY_OP_ADD,
        .applySpellModifierPrimary   = modifySpellKnockbackTime,
        .applySpellModifierSecondary = modifySpellKnockbackAmount,
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE | RUNE_CLASS_FILTER_RANGER
    },
    [RUNE_TYPE_IRON_GAUNTLET] = {
        .iconIndex = BITMAP_RUNE_IRON_GAUNTLET,
        .primaryBaseValue   = 0.5f,
        .primaryPerLevel    = 0.5f,
        .secondaryBaseValue = 0.5f,
        .secondaryPerLevel  = 0.5f,
        .maxLevel = 3,
        .spellTarget = RUNE_SPELL_TARGET_SPECIAL,
        .primaryModifyOperation   = MODIFY_OP_ADD,
        .secondaryModifyOperation = MODIFY_OP_ADD,
        .applySpellModifierPrimary   = modifySpellKnockbackTime,
        .applySpellModifierSecondary = modifySpellKnockbackAmount,
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE | RUNE_CLASS_FILTER_RANGER
    },
    [RUNE_TYPE_FROST_SHARD] = {
        .iconIndex = BITMAP_RUNE_FROST_SHARD,
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
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE | RUNE_CLASS_FILTER_RANGER
    },
    [RUNE_TYPE_ICE_LANCE] = {
        .iconIndex = BITMAP_RUNE_ICE_LANCE,
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
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE | RUNE_CLASS_FILTER_RANGER
    },
    [RUNE_TYPE_NEEDLE_AND_THREAD] = {
        .iconIndex = BITMAP_RUNE_NEEDLE_AND_THREAD,
        .primaryBaseValue = 1,
        .primaryPerLevel = 1,
        .maxLevel = 3,
        .spellTarget = RUNE_SPELL_TARGET_BASIC,
        .primaryModifyOperation = MODIFY_OP_ADD,
        .applySpellModifierPrimary = modifyProjectilePierceCount,
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE | RUNE_CLASS_FILTER_RANGER
    },
    [RUNE_TYPE_HAMMER_AND_NAILS] = {
        .iconIndex = BITMAP_RUNE_HAMMER_AND_NAILS,
        .primaryBaseValue = 1,
        .primaryPerLevel = 1,
        .maxLevel = 3,
        .spellTarget = RUNE_SPELL_TARGET_SPECIAL,
        .primaryModifyOperation = MODIFY_OP_ADD,
        .applySpellModifierPrimary = modifyProjectilePierceCount,
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE | RUNE_CLASS_FILTER_RANGER
    },
    [RUNE_TYPE_MAGNIFYING_GLASS] = {
        .iconIndex = BITMAP_RUNE_MAGNIFYING_GLASS,
        .primaryBaseValue   = 2,
        .primaryPerLevel    = 2,
        .secondaryBaseValue = 15 * (PI / 180),
        .secondaryPerLevel = -5 * (PI / 180),
        .tertiaryBaseValue  = 0.5f,
        .maxLevel = 2,
        .spellTarget = RUNE_SPELL_TARGET_BASIC,
        .primaryModifyOperation   = MODIFY_OP_ADD,
        .secondaryModifyOperation = MODIFY_OP_REPLACE_IF_ZERO,
        .tertiaryModifyOperation  = MODIFY_OP_MULTIPLY,
        .applySpellModifierPrimary   = modifyProjectileSpreadCount,
        .applySpellModifierSecondary = modifyProjectileSpreadAngle,
        .applySpellModifierTertiary  = modifySpellDamage,
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE | RUNE_CLASS_FILTER_RANGER
    },
    [RUNE_TYPE_QUARTZ_CLUSTER] = {
        .iconIndex = BITMAP_RUNE_QUARTZ_CLUSTER,
        .primaryBaseValue   = 2,
        .primaryPerLevel    = 2,
        .secondaryBaseValue = 15 * (PI / 180),
        .secondaryPerLevel = -5 * (PI / 180),
        .tertiaryBaseValue  = 0.5f,
        .maxLevel = 2,
        .spellTarget = RUNE_SPELL_TARGET_SPECIAL,
        .primaryModifyOperation   = MODIFY_OP_ADD,
        .secondaryModifyOperation = MODIFY_OP_REPLACE_IF_ZERO,
        .tertiaryModifyOperation  = MODIFY_OP_MULTIPLY,
        .applySpellModifierPrimary   = modifyProjectileSpreadCount,
        .applySpellModifierSecondary = modifyProjectileSpreadAngle,
        .applySpellModifierTertiary  = modifySpellDamage,
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE | RUNE_CLASS_FILTER_RANGER
    },
    [RUNE_TYPE_ENCHANTED_AMULET] = {
        .iconIndex = BITMAP_RUNE_ENCHANTED_AMULET,
        .primaryBaseValue   = 1,
        .primaryPerLevel    = 1,
        .secondaryBaseValue = 0.15f,
        .maxLevel = 3,
        .spellTarget = RUNE_SPELL_TARGET_BASIC,
        .primaryModifyOperation   = MODIFY_OP_ADD,
        .secondaryModifyOperation = MODIFY_OP_REPLACE_IF_ZERO,
        .applySpellModifierPrimary   = modifyRepeatCount,
        .applySpellModifierSecondary = modifyRepeatActivationTime,
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE | RUNE_CLASS_FILTER_RANGER
    },
    [RUNE_TYPE_ARCANE_STAFF] = {
        .iconIndex = BITMAP_RUNE_ARCANE_STAFF,
        .primaryBaseValue   = 1,
        .primaryPerLevel    = 1,
        .secondaryBaseValue = 0.15f,
        .maxLevel = 3,
        .spellTarget = RUNE_SPELL_TARGET_SPECIAL,
        .primaryModifyOperation   = MODIFY_OP_ADD,
        .secondaryModifyOperation = MODIFY_OP_REPLACE_IF_ZERO,
        .applySpellModifierPrimary   = modifyRepeatCount,
        .applySpellModifierSecondary = modifyRepeatActivationTime,
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE | RUNE_CLASS_FILTER_RANGER
    },
    [RUNE_TYPE_THUNDERCLOUD] = {
        .iconIndex = BITMAP_RUNE_THUNDERCLOUD,
        .primaryBaseValue   = 10, // damage
        .primaryPerLevel    = 0,
        .secondaryBaseValue = 1, // chain count
        .secondaryPerLevel  = 1,
        .maxLevel = 3,
        .triggerType = RUNE_TRIGGER_TIME,
        .triggerBaseValue = 5.0f,
        .triggerPerLevel  = 0.0f,
        .triggerCallback  = thunderStoneCallback
    },
    [RUNE_TYPE_MEDITATION] = {
        .iconIndex = BITMAP_RUNE_MEDITATION,
        .primaryBaseValue = 5,
        .primaryPerLevel  = 5,
        .maxLevel = 3,
        .triggerType = RUNE_TRIGGER_ROOM_CLEAR,
        .triggerBaseValue = 1.0f,
        .triggerCallback  = meditationCallback
    },
    [RUNE_TYPE_THORNS] = {
        // Reflect handled directly in damagePlayer()
        .iconIndex = BITMAP_RUNE_THORNS,
        .primaryBaseValue = 12,
        .primaryPerLevel  = 0,
        .maxLevel = 1
    },
    [RUNE_TYPE_BRASS_KNUCKLES] = {
        .iconIndex = BITMAP_RUNE_BRASS_KNUCKLES,
        .primaryBaseValue   = 0.6f, // absolute projectile lifetime (seconds)
        .primaryPerLevel    = -0.1f,
        .secondaryBaseValue = 1.5f, // damage multiplier
        .secondaryPerLevel  = 0.5f,
        .maxLevel = 3,
        .spellTarget = RUNE_SPELL_TARGET_BASIC,
        .primaryModifyOperation   = MODIFY_OP_ADD,
        .secondaryModifyOperation = MODIFY_OP_MULTIPLY,
        .applySpellModifierPrimary   = setProjectileActiveTime,
        .applySpellModifierSecondary = modifySpellDamage,
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE | RUNE_CLASS_FILTER_RANGER
    },
    [RUNE_TYPE_BLUNDERBUSS] = {
        .iconIndex = BITMAP_RUNE_BLUNDERBUSS,
        .primaryBaseValue   = 0.6f, // absolute projectile lifetime (seconds)
        .primaryPerLevel    = -0.1f,
        .secondaryBaseValue = 2.0f, // damage multiplier
        .secondaryPerLevel  = 0.5f,
        .maxLevel = 3,
        .spellTarget = RUNE_SPELL_TARGET_SPECIAL,
        .primaryModifyOperation   = MODIFY_OP_ADD,
        .secondaryModifyOperation = MODIFY_OP_MULTIPLY,
        .applySpellModifierPrimary   = setProjectileActiveTime,
        .applySpellModifierSecondary = modifySpellDamage,
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE | RUNE_CLASS_FILTER_RANGER
    },
    [RUNE_TYPE_RICOCHET] = {
        .iconIndex = BITMAP_RUNE_RICOCHET,
        .primaryBaseValue = 1,
        .primaryPerLevel  = 1,
        .maxLevel = 3,
        .spellTarget = RUNE_SPELL_TARGET_BASIC,
        .primaryModifyOperation = MODIFY_OP_ADD,
        .applySpellModifierPrimary = modifyProjectileBounceCount,
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE | RUNE_CLASS_FILTER_RANGER
    },
    [RUNE_TYPE_BOMBER] = {
        .iconIndex = BITMAP_RUNE_BOMBER,
        .maxLevel = 1,
        .triggerType = RUNE_TRIGGER_DASH,
        .triggerBaseValue = 1.0f,
        .triggerCallback  = bomberCallback
    },
    [RUNE_TYPE_DRAGONS_BREATH] = {
        .iconIndex = BITMAP_RUNE_DRAGONS_BREATH,
        .maxLevel = 1,
        .triggerType = RUNE_TRIGGER_DASH,
        .triggerBaseValue = 1.0f,
        .triggerCallback  = dragonsBreathCallback
    },
    [RUNE_TYPE_ARMORED_SKIN] = {
        // Knockback + damage handled directly in damagePlayer()
        .iconIndex = BITMAP_RUNE_ARMORED_SKIN,
        .primaryBaseValue = 10,
        .primaryPerLevel  = 0,
        .maxLevel = 1
    },
    [RUNE_TYPE_VIKING] = {
        // Damage multiplier handled directly in damageEnemy()
        .iconIndex = BITMAP_RUNE_VIKING,
        .primaryBaseValue = 2,
        .primaryPerLevel  = 0,
        .maxLevel = 1
    },
    [RUNE_TYPE_SECOND_WIND] = {
        .iconIndex = BITMAP_RUNE_SECOND_WIND,
        .maxLevel = 1,
        .triggerType = RUNE_TRIGGER_KILL_ENEMY,
        .triggerBaseValue = 1.0f,
        .triggerCallback  = secondWindCallback
    },
    [RUNE_TYPE_FLYWHEEL] = {
        .iconIndex = BITMAP_RUNE_FLYWHEEL,
        .primaryBaseValue = 1.0f, // cooldown reduction in seconds per dash
        .maxLevel = 1,
        .triggerType = RUNE_TRIGGER_DASH,
        .triggerBaseValue = 1.0f,
        .triggerCallback  = flywheelCallback
    }
};

// String id for each rune, used to build the localization.csv keys
// rune.<id>.name and rune.<id>.desc. Order matches the RuneType enum.
static const char *RUNE_LOC_IDS[RUNE_TYPE_COUNT] = {
    [RUNE_TYPE_VAMPIRISM] = "vampirism",
    [RUNE_TYPE_WINGED_BOOTS] = "winged_boots",
    [RUNE_TYPE_HEALTHY] = "healthy",
    [RUNE_TYPE_SWIFT] = "swift",
    [RUNE_TYPE_DAGGER] = "dagger",
    [RUNE_TYPE_BROADSWORD] = "broadsword",
    [RUNE_TYPE_POCKET_WATCH] = "pocket_watch",
    [RUNE_TYPE_HOURGLASS] = "hourglass",
    [RUNE_TYPE_SLINGSHOT] = "slingshot",
    [RUNE_TYPE_LONGBOW] = "longbow",
    [RUNE_TYPE_BEAR_PAW] = "bear_paw",
    [RUNE_TYPE_IRON_GAUNTLET] = "iron_gauntlet",
    [RUNE_TYPE_FROST_SHARD] = "frost_shard",
    [RUNE_TYPE_ICE_LANCE] = "ice_lance",
    [RUNE_TYPE_NEEDLE_AND_THREAD] = "needle_and_thread",
    [RUNE_TYPE_HAMMER_AND_NAILS] = "hammer_and_nails",
    [RUNE_TYPE_MAGNIFYING_GLASS] = "magnifying_glass",
    [RUNE_TYPE_QUARTZ_CLUSTER] = "quartz_cluster",
    [RUNE_TYPE_ENCHANTED_AMULET] = "enchanted_amulet",
    [RUNE_TYPE_ARCANE_STAFF] = "arcane_staff",
    [RUNE_TYPE_THUNDERCLOUD] = "thundercloud",
    [RUNE_TYPE_MEDITATION] = "meditation",
    [RUNE_TYPE_THORNS] = "thorns",
    [RUNE_TYPE_BRASS_KNUCKLES] = "brass_knuckles",
    [RUNE_TYPE_BLUNDERBUSS] = "blunderbuss",
    [RUNE_TYPE_RICOCHET] = "ricochet",
    [RUNE_TYPE_BOMBER] = "bomber",
    [RUNE_TYPE_DRAGONS_BREATH] = "dragons_breath",
    [RUNE_TYPE_ARMORED_SKIN] = "armored_skin",
    [RUNE_TYPE_VIKING] = "viking",
    [RUNE_TYPE_SECOND_WIND] = "second_wind",
    [RUNE_TYPE_FLYWHEEL] = "flywheel",
};

void initRuneLocalization(void) {
    for (int i = RUNE_TYPE_NONE + 1; i < RUNE_TYPE_COUNT; i++) {
        const char *id = RUNE_LOC_IDS[i];
        if (id == NULL) {
            continue;
        }
        char *nameKey = NULL;
        char *descKey = NULL;
        pd->system->formatString(&nameKey, "rune.%s.name", id);
        pd->system->formatString(&descKey, "rune.%s.desc", id);
        RUNE_DATA[i].nameEN = (char *)getLocText(nameKey, LANGUAGE_EN);
        RUNE_DATA[i].nameJP = (char *)getLocText(nameKey, LANGUAGE_JP);
        RUNE_DATA[i].descriptionEN = (char *)getLocText(descKey, LANGUAGE_EN);
        RUNE_DATA[i].descriptionJP = (char *)getLocText(descKey, LANGUAGE_JP);
        pd->system->realloc(nameKey, 0);
        pd->system->realloc(descKey, 0);
    }
}

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
    // setRuneLevel(RUNE_TYPE_THUNDERCLOUD, 1);
    // setRuneLevel(RUNE_TYPE_THORNS, 1);
    // setRuneLevel(RUNE_TYPE_BRASS_KNUCKLES, 1);
    // setRuneLevel(RUNE_TYPE_BLUNDERBUSS, 1);
    // setRuneLevel(RUNE_TYPE_RICOCHET, 1);
    // setRuneLevel(RUNE_TYPE_BOMBER, 1);
    // setRuneLevel(RUNE_TYPE_DRAGONS_BREATH, 1);
    // setRuneLevel(RUNE_TYPE_ARMORED_SKIN, 1);
    // setRuneLevel(RUNE_TYPE_VIKING, 1);
    // setRuneLevel(RUNE_TYPE_SECOND_WIND, 1);
    // setRuneLevel(RUNE_TYPE_FLYWHEEL, 1);
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
    clampPlayerHealth();
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
    clampPlayerHealth();
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
        if (rune->applySpellModifierTertiary) {
            float value = rune->tertiaryBaseValue + rune->tertiaryPerLevel * (level - 1);
            if (rune->spellTarget == RUNE_SPELL_TARGET_BASIC || rune->spellTarget == RUNE_SPELL_TARGET_BOTH) {
                rune->applySpellModifierTertiary(basicSpell, value, rune->tertiaryModifyOperation);
            }
            if (rune->spellTarget == RUNE_SPELL_TARGET_SPECIAL || rune->spellTarget == RUNE_SPELL_TARGET_BOTH) {
                rune->applySpellModifierTertiary(specialSpell, value, rune->tertiaryModifyOperation);
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