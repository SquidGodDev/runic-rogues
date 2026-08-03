#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "game/systems/runeManager.h"
#include "globals/helpers.h"
#include "game/rendering/textures.h"
#include "game/settings/settings.h"
#include "globals/localization.h"
#include "game/player/spells/spells.h"
#include "game/player/classes/classData.h"
#include "game/player/playerStats.h"
#include "game/systems/chainLightningManager.h"
#include "game/systems/timerManager.h"
#include "game/player/player.h"
#include "game/entities/projectiles.h"
#include "game/rendering/particles.h"
#include "game/enemies/enemies.h"
#include "game/entities/pickups.h"

#define DRAGONS_BREATH_SPEED 120.0f
#define DRAGONS_BREATH_DAMAGE 8
#define DRAGONS_BREATH_CONE_ANGLE (15.0f * PI / 180.0f)
#define DRAGONS_BREATH_INTERVAL 0.1f

// Time for a full Crescendo bar to decay back to zero once you stop moving. The
// ramp time (seconds of movement to reach max) is the rune's secondary value.
#define CRESCENDO_DECAY_TIME 1.0f
// Rapier ramps its move-speed bonus per hit (the increment is the rune's
// secondary value); if no hit lands within RAPIER_HIT_WINDOW seconds the bonus
// decays to zero over RAPIER_DECAY_TIME.
#define RAPIER_HIT_WINDOW 1.5f
#define RAPIER_DECAY_TIME 1.0f
// Seconds between damaging root spawns while Rootwalker is moving.
#define ROOTWALKER_INTERVAL 0.35f

// Black Hole pulls for as long as its particle animates (50 frames x 0.1s). The
// pull is a negative knockback, which decays over its own duration, so it is
// re-applied on an interval to keep enemies drifting inward for the full window.
#define BLACK_HOLE_DURATION 5.0f
#define BLACK_HOLE_RADIUS (CELL_SIZE * 2)
#define BLACK_HOLE_PULL_INTERVAL 0.5f
#define BLACK_HOLE_PULL_TIME 0.4f

// Radius around the player within which Kunai damages enemies dashed through.
#define KUNAI_HIT_RADIUS (CELL_SIZE * 0.75f)

static float crescendoTime;    // seconds of continuous movement (0..ramp time)
static float rapierProgress;  // move-speed ramp (0..1)
static float rapierWindow;    // seconds left before Rapier resets
static float rootwalkerTimer; // accumulates while moving, spawns roots at interval
static bool bulwarkAvailable; // Bulwark shield, consumed on the first hit each room
static bool reviveAvailable;  // Revive, consumed on the first death of the run
static float blackHoleTime;      // seconds left on the active black hole
static float blackHolePullTimer; // seconds until the next pull is applied
static float blackHoleX;
static float blackHoleY;
static bool kunaiWasDashing;               // tracks dash edges to reset the hit list
static bool kunaiHit[MAX_ENEMY_COUNT];     // enemies already struck during the current dash

static void rapierCallback(float primaryValue, float secondaryValue, float x, float y) {
    (void)primaryValue;
    (void)x;
    (void)y;
    rapierWindow = RAPIER_HIT_WINDOW;
    rapierProgress += secondaryValue;
    if (rapierProgress > 1.0f) {
        rapierProgress = 1.0f;
    }
}

static void bulwarkCallback(float primaryValue, float secondaryValue, float x, float y) {
    (void)primaryValue;
    (void)secondaryValue;
    (void)x;
    (void)y;
    if (!bulwarkAvailable) {
        return;
    }
    playSound(SFX_SHIELD_RESTORE);
    bulwarkAvailable = true;
}

static void thunderStoneCallback(float primaryValue, float secondaryValue, float x, float y) {
    (void)x;
    (void)y;
    startChainLightningOnNearestEnemy(playerX, playerY, CELL_SIZE * 5, primaryValue, (int)secondaryValue);
}

// Range within which Thunder Boots seeks enemies to arc lightning at on dash.
#define THUNDER_BOOTS_RANGE (CELL_SIZE * 5)

static void thunderBootsCallback(float primaryValue, float secondaryValue, float x, float y) {
    arcLightningToNearestEnemies(x, y, THUNDER_BOOTS_RANGE, primaryValue, (int)secondaryValue);
}

static void meditationCallback(float primaryValue, float secondaryValue, float x, float y) {
    (void)secondaryValue;
    (void)x;
    (void)y;
    healPlayer(primaryValue);
}

static void bomberCallback(float primaryValue, float secondaryValue, float x, float y) {
    (void)primaryValue;
    (void)secondaryValue;
    (void)x;
    (void)y;
    playSound(SFX_BOMB_DROP);
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

static void dragonsBreathCallback(float primaryValue, float secondaryValue, float x, float y) {
    (void)secondaryValue;
    (void)x;
    (void)y;
    // Spray the fireballs one at a time, staggered over DRAGONS_BREATH_INTERVAL.
    // primaryValue is the total projectile count (scales with rune level).
    int projectileCount = (int)primaryValue;
    fireDragonsBreath();
    addTimer(DRAGONS_BREATH_INTERVAL, projectileCount - 1, dragonsBreathTimerCallback, NULL);
}

static void afterimageCallback(float primaryValue, float secondaryValue, float x, float y) {
    (void)primaryValue;
    (void)secondaryValue;
    (void)x;
    (void)y;
    grantUtilitySpellCharge();
}

static void flywheelCallback(float primaryValue, float secondaryValue, float x, float y) {
    (void)secondaryValue;
    (void)x;
    (void)y;
    reduceSpecialSpellCooldown(primaryValue);
}

static void healingPotionCallback(float primaryValue, float secondaryValue, float x, float y) {
    // secondaryValue is the heal amount, read by pickups.c when the potion is collected
    (void)secondaryValue;
    if (randFloat(0.0f, 1.0f) >= primaryValue) {
        return;
    }
    spawnPickup(x, y, PICKUP_TYPE_HEALTH_POTION);
}

static void blackHoleCallback(float primaryValue, float secondaryValue, float x, float y) {
    // Spawns at the defeated enemy's location every N kills (trigger-driven).
    // primaryValue carries no per-kill randomness here; secondaryValue is the pull
    // strength, applied per tick in updateRuneEffects().
    (void)primaryValue;
    (void)secondaryValue;
    if (blackHoleTime > 0.0f) {
        return;
    }
    if (spawnParticle(PARTICLE_TYPE_BLACK_HOLE, x, y, x, y, PARTICLE_TYPE_NONE, false /*enemyParticle*/) < 0) {
        return;
    }
    blackHoleX = x;
    blackHoleY = y;
    blackHoleTime = BLACK_HOLE_DURATION;
    blackHolePullTimer = 0.0f;
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
        .primaryBaseValue = 0.05f, // +X% move speed at max
        .primaryPerLevel = 0.05f,
        .maxLevel = 3,
        .primaryModifyOperation = MODIFY_OP_MULTIPLY
    },
    [RUNE_TYPE_GLASS_DAGGER] = {
        .iconIndex = BITMAP_RUNE_GLASS_DAGGER,
        .primaryBaseValue = 0.1f,
        .primaryPerLevel = 0.1f,
        .secondaryBaseValue = 20,
        .secondaryPerLevel = 20,
        .maxLevel = 3,
        .primaryModifyOperation = MODIFY_OP_MULTIPLY
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
        .applySpellModifierPrimary = modifySpellDamage,
        .classFilter = RUNE_CLASS_FILTER_NONE // Disabled for now
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
        .classFilter = RUNE_CLASS_FILTER_NONE // Disabled for now
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
        .classFilter = RUNE_CLASS_FILTER_NONE // Disabled for now
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
        .rarity = RUNE_RARITY_RARE,
        .primaryBaseValue   = 2,
        .primaryPerLevel    = 2,
        .secondaryBaseValue = 15 * (PI / 180),
        .secondaryPerLevel = -5 * (PI / 180),
        .tertiaryBaseValue  = -0.5f, // damage penalty, half damage
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
        .rarity = RUNE_RARITY_RARE,
        .primaryBaseValue   = 2,
        .primaryPerLevel    = 2,
        .secondaryBaseValue = 15 * (PI / 180),
        .secondaryPerLevel = -5 * (PI / 180),
        .tertiaryBaseValue  = -0.5f, // damage penalty, half damage
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
        .hudIconIndex = BITMAP_RUNE_HUD_ENCHANTED_AMULET,
        .rarity = RUNE_RARITY_RARE,
        .primaryBaseValue   = 5, // cast interval: fire 1 extra time every 5/4/3/2/1 casts
        .primaryPerLevel    = -1,
        .secondaryBaseValue = 1, // repeat count (additional fires per activation)
        .tertiaryBaseValue  = 0.15f,
        .maxLevel = 5,
        .spellTarget = RUNE_SPELL_TARGET_BASIC,
        .primaryModifyOperation   = MODIFY_OP_ADD,
        .secondaryModifyOperation = MODIFY_OP_ADD,
        .tertiaryModifyOperation  = MODIFY_OP_REPLACE_IF_ZERO,
        .applySpellModifierPrimary   = modifyRepeatInterval,
        .applySpellModifierSecondary = modifyRepeatCount,
        .applySpellModifierTertiary  = modifyRepeatActivationTime,
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE | RUNE_CLASS_FILTER_RANGER
    },
    [RUNE_TYPE_ARCANE_STAFF] = {
        .iconIndex = BITMAP_RUNE_ARCANE_STAFF,
        .hudIconIndex = BITMAP_RUNE_HUD_ARCANE_STAFF,
        .rarity = RUNE_RARITY_RARE,
        .primaryBaseValue   = 3, // cast interval: fire 1 extra time every 3/2/1 casts
        .primaryPerLevel    = -1,
        .secondaryBaseValue = 1, // repeat count (additional fires per activation)
        .tertiaryBaseValue  = 0.15f,
        .maxLevel = 3,
        .spellTarget = RUNE_SPELL_TARGET_SPECIAL,
        .primaryModifyOperation   = MODIFY_OP_ADD,
        .secondaryModifyOperation = MODIFY_OP_ADD,
        .tertiaryModifyOperation  = MODIFY_OP_REPLACE_IF_ZERO,
        .applySpellModifierPrimary   = modifyRepeatInterval,
        .applySpellModifierSecondary = modifyRepeatCount,
        .applySpellModifierTertiary  = modifyRepeatActivationTime,
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE | RUNE_CLASS_FILTER_RANGER
    },
    [RUNE_TYPE_THUNDERCLOUD] = {
        .iconIndex = BITMAP_RUNE_THUNDERCLOUD,
        .hudIconIndex = BITMAP_RUNE_HUD_LIGHTNING,
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
        .maxLevel = 1,
        .classFilter = RUNE_CLASS_FILTER_NONE // Disabled for now
    },
    [RUNE_TYPE_BRASS_KNUCKLES] = {
        .iconIndex = BITMAP_RUNE_BRASS_KNUCKLES,
        .rarity = RUNE_RARITY_RARE,
        .primaryBaseValue   = 0.5f, // absolute projectile lifetime (seconds)
        .primaryPerLevel    = -0.1f,
        .secondaryBaseValue = 0.3f, // damage bonus, +30%
        .secondaryPerLevel  = 0.5f,
        .maxLevel = 1,
        .spellTarget = RUNE_SPELL_TARGET_BASIC,
        .primaryModifyOperation   = MODIFY_OP_ADD,
        .secondaryModifyOperation = MODIFY_OP_MULTIPLY,
        .applySpellModifierPrimary   = setProjectileActiveTime,
        .applySpellModifierSecondary = modifySpellDamage,
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE | RUNE_CLASS_FILTER_RANGER
    },
    [RUNE_TYPE_BLUNDERBUSS] = {
        .iconIndex = BITMAP_RUNE_BLUNDERBUSS,
        .rarity = RUNE_RARITY_RARE,
        .primaryBaseValue   = 0.5f, // absolute projectile lifetime (seconds)
        .primaryPerLevel    = -0.1f,
        .secondaryBaseValue = 0.3f, // damage bonus, +30%
        .secondaryPerLevel  = 0.5f,
        .maxLevel = 1,
        .spellTarget = RUNE_SPELL_TARGET_SPECIAL,
        .primaryModifyOperation   = MODIFY_OP_ADD,
        .secondaryModifyOperation = MODIFY_OP_MULTIPLY,
        .applySpellModifierPrimary   = setProjectileActiveTime,
        .applySpellModifierSecondary = modifySpellDamage,
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE | RUNE_CLASS_FILTER_RANGER
    },
    [RUNE_TYPE_RICOCHET] = {
        .iconIndex = BITMAP_RUNE_RICOCHET,
        .rarity = RUNE_RARITY_RARE,
        .primaryBaseValue = 1,
        .primaryPerLevel  = 1,
        .maxLevel = 1,
        .spellTarget = RUNE_SPELL_TARGET_BASIC,
        .primaryModifyOperation = MODIFY_OP_ADD,
        .applySpellModifierPrimary = modifyProjectileBounceCount,
        .classFilter = RUNE_CLASS_FILTER_MAGE | RUNE_CLASS_FILTER_ROGUE | RUNE_CLASS_FILTER_RANGER
    },
    [RUNE_TYPE_BOMBER] = {
        .iconIndex = BITMAP_RUNE_BOMBER,
        .hudIconIndex = BITMAP_RUNE_HUD_BOMB,
        .maxLevel = 3,
        .triggerType = RUNE_TRIGGER_DASH,
        // Drops a bomb every 3 / 2 / 1 dashes at level 1 / 2 / 3.
        .triggerBaseValue = 3.0f,
        .triggerPerLevel  = -1.0f,
        .triggerCallback  = bomberCallback
    },
    [RUNE_TYPE_DRAGONS_BREATH] = {
        .iconIndex = BITMAP_RUNE_DRAGONS_BREATH,
        .primaryBaseValue = 3,
        .primaryPerLevel  = 3,
        .maxLevel = 3,
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
        .hudIconIndex = BITMAP_RUNE_HUD_VIKING,
        .primaryBaseValue = 0.2f, // +20% / +40% / +60% damage below 50% health
        .primaryPerLevel  = 0.2f,
        .maxLevel = 3
    },
    [RUNE_TYPE_AFTERIMAGE] = {
        .iconIndex = BITMAP_RUNE_AFTERIMAGE,
        .maxLevel = 1,
        .triggerType = RUNE_TRIGGER_KILL_ENEMY,
        .triggerBaseValue = 1.0f,
        .triggerCallback  = afterimageCallback
    },
    [RUNE_TYPE_FLYWHEEL] = {
        .iconIndex = BITMAP_RUNE_FLYWHEEL,
        .primaryBaseValue = 1.0f, // cooldown reduction in seconds per dash
        .maxLevel = 1,
        .triggerType = RUNE_TRIGGER_DASH,
        .triggerBaseValue = 1.0f,
        .triggerCallback  = flywheelCallback
    },
    [RUNE_TYPE_CRESCENDO] = {
        // Move-speed multiplier handled directly in getPlayerMoveSpeed()
        .iconIndex = BITMAP_RUNE_CRESCENDO,
        .hudIconIndex = BITMAP_RUNE_HUD_CRESCENDO,
        .primaryBaseValue   = 0.15f,  // +10% move speed at max ramp
        .secondaryBaseValue = 15.0f, // seconds of movement to reach max
        .maxLevel = 1
    },
    [RUNE_TYPE_FOCUS] = {
        // Damage multiplier handled directly in damageEnemy()
        .iconIndex = BITMAP_RUNE_FOCUS,
        .hudIconIndex = BITMAP_RUNE_HUD_FOCUS,
        .primaryBaseValue = 0.10f,
        .maxLevel = 1
    },
    [RUNE_TYPE_ROOTWALKER] = {
        // Root spawning handled directly in updateRuneEffects()
        .iconIndex = BITMAP_RUNE_ROOTWALKER,
        .maxLevel = 1,
        .classFilter = RUNE_CLASS_FILTER_NONE // Disabled for now
    },
    [RUNE_TYPE_RAPIER] = {
        // Move-speed multiplier handled directly in getPlayerMoveSpeed()
        .iconIndex = BITMAP_RUNE_RAPIER,
        .hudIconIndex = BITMAP_RUNE_HUD_RAPIER,
        .primaryBaseValue   = 0.15f, // +X% move speed at max ramp
        .primaryPerLevel = 0.1f,
        .secondaryBaseValue = 0.05f, // 5% ramp progress added per hit
        .maxLevel = 1,
        .triggerType = RUNE_TRIGGER_DAMAGE_ENEMY,
        .triggerBaseValue = 1.0f,
        .triggerCallback  = rapierCallback
    },
    [RUNE_TYPE_BULWARK] = {
        // First-hit reduction + HUD availability handled directly in damagePlayer();
        // shield refreshes each room via the ROOM_ENTER trigger
        .iconIndex = BITMAP_RUNE_BULWARK,
        .hudIconIndex = BITMAP_RUNE_HUD_BULWARK,
        .maxLevel = 1,
        .triggerType = RUNE_TRIGGER_ROOM_ENTER,
        .triggerBaseValue = 1.0f,
        .triggerCallback  = bulwarkCallback
    },
    [RUNE_TYPE_LIGHTNING_ROD] = {
        // Lightning arc handled directly in damagePlayer()
        .iconIndex = BITMAP_RUNE_LIGHTNING_ROD,
        .primaryBaseValue   = 5, // damage per arc
        .primaryPerLevel = 2,
        .secondaryBaseValue = 3,  // enemies struck
        .secondaryPerLevel = 1,
        .maxLevel = 3
    },
    [RUNE_TYPE_HEALING_POTION] = {
        .iconIndex = BITMAP_RUNE_HEALING_POTION,
        .primaryBaseValue = 0.05f, // drop chance
        .primaryPerLevel  = 0.05f,
        .secondaryBaseValue = 5, // heal amount on pickup
        .maxLevel = 3,
        .primaryModifyOperation = MODIFY_OP_MULTIPLY,
        .triggerType = RUNE_TRIGGER_KILL_ENEMY,
        .triggerBaseValue = 1.0f,
        .triggerCallback  = healingPotionCallback
    },
    [RUNE_TYPE_STONE_GOLEM] = {
        // Damage reduction + HUD handled directly in damagePlayer()
        .iconIndex = BITMAP_RUNE_STONE_GOLEM,
        .hudIconIndex = BITMAP_RUNE_HUD_STONE_GOLEM,
        .primaryBaseValue = 0.2f, // 20% less damage while still
        .maxLevel = 1,
        .primaryModifyOperation = MODIFY_OP_MULTIPLY
    },
    [RUNE_TYPE_ADRENALINE] = {
        // Cooldown reduction handled directly in updateSpells()
        .iconIndex = BITMAP_RUNE_ADRENALINE,
        .hudIconIndex = BITMAP_RUNE_HUD_ADRENALINE,
        .primaryBaseValue   = 0.25f, // cooldown reduction while active
        .secondaryBaseValue = 0.33f, // health fraction cutoff
        .maxLevel = 1,
        .primaryModifyOperation   = MODIFY_OP_MULTIPLY,
        .secondaryModifyOperation = MODIFY_OP_MULTIPLY
    },
    [RUNE_TYPE_BLACK_HOLE] = {
        .iconIndex = BITMAP_RUNE_BLACK_HOLE,
        .hudIconIndex = BITMAP_RUNE_HUD_BLACK_HOLE,
        .secondaryBaseValue = -0.5f, // pull strength, negative to draw enemies inward
        .maxLevel = 3,
        .triggerType = RUNE_TRIGGER_KILL_ENEMY,
        .triggerBaseValue = 3.0f, // enemies defeated between spawns
        .triggerPerLevel  = -1.0f, // 3 / 2 / 1 kills at levels 1-3
        .triggerCallback  = blackHoleCallback
    },
    [RUNE_TYPE_KNIGHTS_HELM] = {
        // Max health + move speed handled directly in playerStats.c
        .iconIndex = BITMAP_RUNE_KNIGHTS_HELM,
        .primaryBaseValue   = 30,     // +30 max health
        .secondaryBaseValue = -0.15f, // -15% move speed
        .maxLevel = 1,
        .secondaryModifyOperation = MODIFY_OP_MULTIPLY,
        .classFilter = RUNE_CLASS_FILTER_KNIGHT
    },
    [RUNE_TYPE_THUNDER_BOOTS] = {
        .iconIndex = BITMAP_RUNE_THUNDER_BOOTS,
        .primaryBaseValue   = 10, // damage per arc
        .primaryPerLevel    = 0,
        .secondaryBaseValue = 1,  // enemies struck
        .secondaryPerLevel  = 1,
        .maxLevel = 3,
        .triggerType = RUNE_TRIGGER_DASH,
        .triggerBaseValue = 1.0f,
        .triggerCallback  = thunderBootsCallback
    },
    [RUNE_TYPE_KUNAI] = {
        // Dash-through damage handled directly in updateRuneEffects()
        .iconIndex = BITMAP_RUNE_KUNAI,
        .primaryBaseValue = 5, // damage dealt when dashing through an enemy
        .maxLevel = 1
    },
    [RUNE_TYPE_FEATHER] = {
        // Dash distance multiplier handled directly in player.c
        .iconIndex = BITMAP_RUNE_FEATHER,
        .primaryBaseValue = 2.0f, // dash distance multiplier
        .maxLevel = 1,
        .primaryModifyOperation = MODIFY_OP_MULTIPLY
    },
    [RUNE_TYPE_HIGH_VOLTAGE] = {
        .iconIndex = BITMAP_RUNE_HIGH_VOLTAGE,
        .primaryBaseValue = 1, // additional chain targets
        .primaryPerLevel  = 1,
        .maxLevel = 3,
        .spellTarget = RUNE_SPELL_TARGET_SPECIAL,
        .primaryModifyOperation = MODIFY_OP_ADD,
        .applySpellModifierPrimary = modifyProjectileChainCount,
        .classFilter = RUNE_CLASS_FILTER_MAGE
    },
    [RUNE_TYPE_BOMB_ARROW] = {
        // Arming counter handled in bombArrowRegisterMaxChargeShot(); the bomb is
        // dropped by SPELL_BOMB_ARROW_DROP set as the armed arrow's onHitSpell.
        .iconIndex = BITMAP_RUNE_BOMB_ARROW,
        .hudIconIndex = BITMAP_RUNE_HUD_BOMB_ARROW,
        .maxLevel = 3,
        // Bomb drops every 5 / 4 / 3 max-charge arrows at level 1 / 2 / 3.
        .triggerBaseValue = 5.0f,
        .triggerPerLevel  = -1.0f,
        .classFilter = RUNE_CLASS_FILTER_RANGER
    },
    [RUNE_TYPE_QUICKDRAW] = {
        .iconIndex = BITMAP_RUNE_QUICKDRAW,
        .primaryBaseValue = -0.20f, // draw time reduction, -20%
        .primaryPerLevel  = -0.15f,
        .maxLevel = 3,
        .spellTarget = RUNE_SPELL_TARGET_BASIC,
        .primaryModifyOperation = MODIFY_OP_MULTIPLY,
        .applySpellModifierPrimary = modifyChargeTime,
        .classFilter = RUNE_CLASS_FILTER_RANGER
    },
    [RUNE_TYPE_REVIVE] = {
        // Death interception handled directly in damagePlayer()
        .iconIndex = BITMAP_RUNE_REVIVE,
        .hudIconIndex = BITMAP_RUNE_HUD_REVIVE,
        .primaryBaseValue = 0.25f, // health fraction restored on revive
        .maxLevel = 1,
        .primaryModifyOperation = MODIFY_OP_MULTIPLY
    }
};

// String id for each rune, used to build the localization.csv keys
// rune.<id>.name and rune.<id>.desc. Order matches the RuneType enum.
static const char *RUNE_LOC_IDS[RUNE_TYPE_COUNT] = {
    [RUNE_TYPE_VAMPIRISM] = "vampirism",
    [RUNE_TYPE_WINGED_BOOTS] = "winged_boots",
    [RUNE_TYPE_HEALTHY] = "healthy",
    [RUNE_TYPE_SWIFT] = "swift",
    [RUNE_TYPE_GLASS_DAGGER] = "glass_dagger",
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
    [RUNE_TYPE_AFTERIMAGE] = "afterimage",
    [RUNE_TYPE_FLYWHEEL] = "flywheel",
    [RUNE_TYPE_CRESCENDO] = "crescendo",
    [RUNE_TYPE_FOCUS] = "focus",
    [RUNE_TYPE_ROOTWALKER] = "rootwalker",
    [RUNE_TYPE_RAPIER] = "rapier",
    [RUNE_TYPE_BULWARK] = "bulwark",
    [RUNE_TYPE_LIGHTNING_ROD] = "lightning_rod",
    [RUNE_TYPE_HEALING_POTION] = "healing_potion",
    [RUNE_TYPE_STONE_GOLEM] = "stone_golem",
    [RUNE_TYPE_ADRENALINE] = "adrenaline",
    [RUNE_TYPE_BLACK_HOLE] = "black_hole",
    [RUNE_TYPE_KNIGHTS_HELM] = "knights_helm",
    [RUNE_TYPE_THUNDER_BOOTS] = "thunder_boots",
    [RUNE_TYPE_KUNAI] = "kunai",
    [RUNE_TYPE_FEATHER] = "feather",
    [RUNE_TYPE_HIGH_VOLTAGE] = "high_voltage",
    [RUNE_TYPE_BOMB_ARROW] = "bomb_arrow",
    [RUNE_TYPE_QUICKDRAW] = "quickdraw",
    [RUNE_TYPE_REVIVE] = "revive",
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

// Runes in the order they were first obtained this run.
static RuneType runeObtainOrder[RUNE_TYPE_COUNT];
static int runeObtainCount;

static void addToObtainOrder(RuneType type) {
    for (int i = 0; i < runeObtainCount; i++) {
        if (runeObtainOrder[i] == type) {
            return;
        }
    }
    runeObtainOrder[runeObtainCount++] = type;
}

static void removeFromObtainOrder(RuneType type) {
    for (int i = 0; i < runeObtainCount; i++) {
        if (runeObtainOrder[i] == type) {
            for (int j = i; j < runeObtainCount - 1; j++) {
                runeObtainOrder[j] = runeObtainOrder[j + 1];
            }
            runeObtainCount--;
            return;
        }
    }
}

void initRunes(void) {
    runeObtainCount = 0;
    for (int i = 0; i < RUNE_TYPE_COUNT; i++) {
        runeLevels[i] = 0;
        runeTriggerCounters[i] = 0.0f;
    }
    crescendoTime = 0.0f;
    rapierProgress = 0.0f;
    rapierWindow = 0.0f;
    rootwalkerTimer = 0.0f;
    bulwarkAvailable = true;
    reviveAvailable = true;
    kunaiWasDashing = false;
    clearBlackHole();

    // == Testing runes ==
    // setRuneLevel(RUNE_TYPE_VAMPIRISM, 1);
    // setRuneLevel(RUNE_TYPE_HEALTHY, 1);
    // setRuneLevel(RUNE_TYPE_SWIFT, 1);
    // setRuneLevel(RUNE_TYPE_GLASS_DAGGER, 1);
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
    // setRuneLevel(RUNE_TYPE_AFTERIMAGE, 1);
    // setRuneLevel(RUNE_TYPE_FLYWHEEL, 1);
    // setRuneLevel(RUNE_TYPE_CRESCENDO, 1);
    // setRuneLevel(RUNE_TYPE_FOCUS, 1);
    // setRuneLevel(RUNE_TYPE_ROOTWALKER, 1);
    // setRuneLevel(RUNE_TYPE_RAPIER, 1);
}

bool runeIsOwned(RuneType type) {
    return runeLevels[type] > 0;
}

void addRune(RuneType type) {
    if (runeIsMaxLevel(type)) {
        return;
    }

    int oldMaxHealth = getPlayerMaxHealth();
    bool wasOwned = runeLevels[type] > 0;
    runeLevels[type]++;
    if (!wasOwned) {
        addToObtainOrder(type);
    }

    recalculateRuneEffectsToSpells();
    applyMaxHealthChange(oldMaxHealth);
}

void setRuneLevel(RuneType type, int level) {
    if (level < 0) {
        level = 0;
    }
    if (level > RUNE_DATA[type].maxLevel) {
        level = RUNE_DATA[type].maxLevel;
    }
    int oldMaxHealth = getPlayerMaxHealth();
    bool wasOwned = runeLevels[type] > 0;
    runeLevels[type] = level;
    if (level == 0) {
        runeTriggerCounters[type] = 0.0f;
        if (wasOwned) {
            removeFromObtainOrder(type);
        }
    } else if (!wasOwned) {
        addToObtainOrder(type);
    }

    recalculateRuneEffectsToSpells();
    applyMaxHealthChange(oldMaxHealth);
}

int getObtainedRuneCount(void) {
    return runeObtainCount;
}

RuneType getObtainedRune(int index) {
    if (index < 0 || index >= runeObtainCount) {
        return RUNE_TYPE_NONE;
    }
    return runeObtainOrder[index];
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

static void replaceValueToken(char *buf, const char *token, float value, bool asPercent) {
    if (strstr(buf, token) == NULL) {
        return;
    }

    char *valueStr = NULL;
    if (asPercent) {
        pd->system->formatString(&valueStr, "%d", (int)roundf(value * 100));
    } else if ((float)(int)value == value) {
        pd->system->formatString(&valueStr, "%d", (int)value);
    } else {
        int tenths = (int)roundf(value * 10);
        pd->system->formatString(&valueStr, "%d.%d", tenths / 10, tenths % 10);
    }
    strReplaceFirst(buf, token, valueStr);
    pd->system->realloc(valueStr, 0);
}

char *getRuneDescription(RuneType type, Language language, int runeLevel) {
    const RuneData *rune = &RUNE_DATA[type];
    const char *tmpl = (language == LANGUAGE_EN) ? rune->descriptionEN : rune->descriptionJP;
    char *buf        = (language == LANGUAGE_EN) ? descBufferEN : descBufferJP;

    if (strchr(tmpl, '{') == NULL) {
        return (char *)tmpl;
    }

    strncpy(buf, tmpl, 255);
    buf[255] = '\0';

    if (runeLevel < 1) {
        runeLevel = 1;
    }

    float primaryValue   = fabsf(rune->primaryBaseValue + rune->primaryPerLevel * (runeLevel - 1));
    float secondaryValue = fabsf(rune->secondaryBaseValue + rune->secondaryPerLevel * (runeLevel - 1));
    float triggerValue   = fabsf(rune->triggerBaseValue + rune->triggerPerLevel * (runeLevel - 1));

    // {1P}/{2P} force percent formatting for runes whose modify operation isn't MULTIPLY
    replaceValueToken(buf, "{1P}", primaryValue, true);
    replaceValueToken(buf, "{2P}", secondaryValue, true);
    replaceValueToken(buf, "{1}", primaryValue, rune->primaryModifyOperation == MODIFY_OP_MULTIPLY);
    replaceValueToken(buf, "{2}", secondaryValue, rune->secondaryModifyOperation == MODIFY_OP_MULTIPLY);
    replaceValueToken(buf, "{T}", triggerValue, false);

    return buf;
}

int getRuneIconIndex(RuneType type) {
    return RUNE_DATA[type].iconIndex;
}

int getRuneHudIconIndex(RuneType type) {
    return RUNE_DATA[type].hudIconIndex;
}

RuneTriggerType getRuneTriggerType(RuneType type) {
    return RUNE_DATA[type].triggerType;
}

float getRuneTriggerProgress(RuneType type) {
    int level = runeLevels[type];
    if (level <= 0) {
        return 0.0f;
    }
    RuneData *rune = &RUNE_DATA[type];
    float threshold = rune->triggerBaseValue + rune->triggerPerLevel * (level - 1);
    if (threshold <= 0.0f) {
        return 0.0f;
    }
    float progress = runeTriggerCounters[type] / threshold;
    if (progress < 0.0f) {
        progress = 0.0f;
    }
    if (progress > 1.0f) {
        progress = 1.0f;
    }
    return progress;
}

RuneClassFilter getRuneClassFilter(RuneType type) {
    return RUNE_DATA[type].classFilter;
}

RuneRarity getRuneRarity(RuneType type) {
    return RUNE_DATA[type].rarity;
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
    signalRuneTriggerAtPosition(type, amount, playerX, playerY);
}

void signalRuneTriggerAtPosition(RuneTriggerType type, float amount, float x, float y) {
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
            rune->triggerCallback(primaryValue, secondaryValue, x, y);
        }
    }
}

float getCrescendoProgress(void) {
    float rampTime = getRuneSecondaryValue(RUNE_TYPE_CRESCENDO);
    if (rampTime <= 0.0f) {
        return 0.0f;
    }
    return crescendoTime / rampTime;
}

float getCrescendoSpeedMultiplier(void) {
    if (!runeIsOwned(RUNE_TYPE_CRESCENDO)) {
        return 1.0f;
    }
    return 1.0f + getCrescendoProgress() * getRuneValue(RUNE_TYPE_CRESCENDO);
}

float getRapierProgress(void) {
    return rapierProgress;
}

float getRapierSpeedMultiplier(void) {
    if (!runeIsOwned(RUNE_TYPE_RAPIER)) {
        return 1.0f;
    }
    return 1.0f + rapierProgress * getRuneValue(RUNE_TYPE_RAPIER);
}

void updateRuneEffects(float deltaTime) {
    bool moving = playerIsMoving();

    if (runeIsOwned(RUNE_TYPE_KUNAI)) {
        bool dashing = playerIsDashing();
        if (dashing) {
            if (!kunaiWasDashing) {
                for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
                    kunaiHit[i] = false;
                }
            }
            float damage = getRuneValue(RUNE_TYPE_KUNAI);
            float radiusSq = KUNAI_HIT_RADIUS * KUNAI_HIT_RADIUS;
            for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
                if (kunaiHit[i] || !enemies[i].active || enemies[i].health <= 0) {
                    continue;
                }
                float dx = enemies[i].x - playerX;
                float dy = enemies[i].y - playerY;
                if (dx * dx + dy * dy <= radiusSq) {
                    kunaiHit[i] = true;
                    damageEnemy(i, damage);
                }
            }
        }
        kunaiWasDashing = dashing;
    }

    if (runeIsOwned(RUNE_TYPE_CRESCENDO)) {
        float rampTime = getRuneSecondaryValue(RUNE_TYPE_CRESCENDO);
        if (moving) {
            crescendoTime += deltaTime;
            if (crescendoTime > rampTime) {
                crescendoTime = rampTime;
            }
        } else {
            crescendoTime -= (rampTime / CRESCENDO_DECAY_TIME) * deltaTime;
            if (crescendoTime < 0.0f) {
                crescendoTime = 0.0f;
            }
        }
    }

    if (runeIsOwned(RUNE_TYPE_ROOTWALKER)) {
        if (moving) {
            rootwalkerTimer += deltaTime;
            if (rootwalkerTimer >= ROOTWALKER_INTERVAL) {
                rootwalkerTimer -= ROOTWALKER_INTERVAL;
                spawnParticle(PARTICLE_TYPE_PLAYER_ROOT_ATTACK, playerX, playerY, playerX, playerY, PARTICLE_TYPE_NONE, false /*enemyParticle*/);
            }
        } else {
            rootwalkerTimer = 0.0f;
        }
    }

    if (runeIsOwned(RUNE_TYPE_RAPIER)) {
        if (rapierWindow > 0.0f) {
            rapierWindow -= deltaTime;
            if (rapierWindow < 0.0f) {
                rapierWindow = 0.0f;
            }
        } else if (rapierProgress > 0.0f) {
            rapierProgress -= (1.0f / RAPIER_DECAY_TIME) * deltaTime;
            if (rapierProgress < 0.0f) {
                rapierProgress = 0.0f;
            }
        }
    }

    if (blackHoleTime > 0.0f) {
        blackHoleTime -= deltaTime;
        blackHolePullTimer -= deltaTime;
        if (blackHolePullTimer <= 0.0f) {
            blackHolePullTimer += BLACK_HOLE_PULL_INTERVAL;
            knockbackEnemiesInArea(blackHoleX, blackHoleY, BLACK_HOLE_RADIUS, blackHoleX, blackHoleY,
                                   BLACK_HOLE_PULL_TIME, getRuneSecondaryValue(RUNE_TYPE_BLACK_HOLE),
                                   false /*awayFromPlayer*/);
        }
    }
}

bool bulwarkIsAvailable(void) {
    return runeIsOwned(RUNE_TYPE_BULWARK) && bulwarkAvailable;
}

void consumeBulwark(void) {
    playSound(SFX_SHIELD_BREAK);
    bulwarkAvailable = false;
}

bool reviveIsAvailable(void) {
    return runeIsOwned(RUNE_TYPE_REVIVE) && reviveAvailable;
}

void consumeRevive(void) {
    reviveAvailable = false;
}

bool adrenalineIsActive(void) {
    return runeIsOwned(RUNE_TYPE_ADRENALINE) && playerHealth < getPlayerMaxHealth() * getRuneSecondaryValue(RUNE_TYPE_ADRENALINE);
}

void clearBlackHole(void) {
    blackHoleTime = 0.0f;
    blackHolePullTimer = 0.0f;
}

float getBlackHoleProgress(void) {
    int level = runeLevels[RUNE_TYPE_BLACK_HOLE];
    if (level <= 0) {
        return 0.0f;
    }
    RuneData *rune = &RUNE_DATA[RUNE_TYPE_BLACK_HOLE];
    float threshold = rune->triggerBaseValue + rune->triggerPerLevel * (level - 1);
    // Fill completes one kill early so a full bar means the next kill spawns the hole.
    float span = threshold - 1.0f;
    if (span <= 0.0f) {
        return 1.0f;
    }
    float progress = runeTriggerCounters[RUNE_TYPE_BLACK_HOLE] / span;
    if (progress < 0.0f) {
        progress = 0.0f;
    }
    if (progress > 1.0f) {
        progress = 1.0f;
    }
    return progress;
}

float getBombArrowProgress(void) {
    int level = runeLevels[RUNE_TYPE_BOMB_ARROW];
    if (level <= 0) {
        return 0.0f;
    }
    RuneData *rune = &RUNE_DATA[RUNE_TYPE_BOMB_ARROW];
    float threshold = rune->triggerBaseValue + rune->triggerPerLevel * (level - 1);
    // Fill completes one shot early so a full bar means the next arrow is the bomb.
    float span = threshold - 1.0f;
    if (span <= 0.0f) {
        return 1.0f;
    }
    float progress = runeTriggerCounters[RUNE_TYPE_BOMB_ARROW] / span;
    if (progress < 0.0f) {
        progress = 0.0f;
    }
    if (progress > 1.0f) {
        progress = 1.0f;
    }
    return progress;
}

bool bombArrowRegisterMaxChargeShot(void) {
    int level = runeLevels[RUNE_TYPE_BOMB_ARROW];
    if (level <= 0) {
        return false;
    }
    RuneData *rune = &RUNE_DATA[RUNE_TYPE_BOMB_ARROW];
    float threshold = rune->triggerBaseValue + rune->triggerPerLevel * (level - 1);
    if (threshold <= 0.0f) {
        return false;
    }
    runeTriggerCounters[RUNE_TYPE_BOMB_ARROW] += 1.0f;
    if (runeTriggerCounters[RUNE_TYPE_BOMB_ARROW] >= threshold) {
        runeTriggerCounters[RUNE_TYPE_BOMB_ARROW] = 0.0f;
        return true;
    }
    return false;
}