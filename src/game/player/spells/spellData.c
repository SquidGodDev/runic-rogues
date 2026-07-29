#include "game/player/spells/spellData.h"
#include "game/player/player.h"
#include "game/entities/projectiles.h"
#include "game/enemies/enemies.h"
#include "game/UI/UI.h"
#include "game/systems/chainLightningManager.h"
#include "game/systems/runeManager.h"
#include "game/entities/turret.h"
#include "game/systems/audioManager.h"
#include "globals/globals.h"
#include "globals/localization.h"

// Spawn the projectile slightly to the right to match hand
#define FIRE_ANGLE_OFFSET PI / 8
#define FIRE_POSITION_OFFSET CELL_SIZE / 2

static void fireProjectileSpread(float x, float y, float angle, float damage, SpellProjectileData projectileData) {
    int spreadCount = projectileData.spreadCount;
    spreadCount = spreadCount < 1 ? 1 : spreadCount;
    float spreadAngle = projectileData.spreadAngle;
    float startAngle = angle - (spreadAngle * (spreadCount - 1) / 2);
    for (int i = 0; i < spreadCount; i++) {
        float castAngle = startAngle + spreadAngle * i;
        float projectileX = x + cosf(castAngle + FIRE_ANGLE_OFFSET) * FIRE_POSITION_OFFSET;
        float projectileY = y + sinf(castAngle + FIRE_ANGLE_OFFSET) * FIRE_POSITION_OFFSET;
        firePlayerProjectile(projectileX, projectileY, cosf(castAngle), sinf(castAngle), damage, projectileData);
    }
}

void projectileCast(Spell spell, SpellCastData *castData, float x, float y, float angle, bool initialCast) {
    SpellData data = spellData[spell];
    fireProjectileSpread(x, y, angle, data.damage, data.projectileData);
}

void chargedProjectileCast(Spell spell, SpellCastData *castData, float x, float y, float angle, bool initialCast) {
    SpellData data = spellData[spell];
    const ChargeData *charge = &data.chargeData;

    int chargeLevel = castData ? castData->chargeLevel : 1;
    if (chargeLevel < 1) {
        chargeLevel = 1;
    }
    if (chargeLevel > charge->levelCount) {
        chargeLevel = charge->levelCount;
    }
    const ChargeLevelData *level = &charge->levels[chargeLevel - 1];

    SpellProjectileData projectile = data.projectileData;
    projectile.speed += level->speedBonus;
    projectile.pierceCount += level->pierceBonus;
    float damage = data.damage + level->damageBonus;

    // Bomb Arrow: every Nth max-charge bow shot is armed to drop a bomb on its
    // first enemy hit (via SPELL_BOMB_ARROW_DROP as the projectile's onHitSpell).
    if (spell == SPELL_BOW_SHOT && chargeLevel >= charge->levelCount && runeIsOwned(RUNE_TYPE_BOMB_ARROW)) {
        if (bombArrowRegisterMaxChargeShot()) {
            projectile.onHitSpell = SPELL_BOMB_ARROW_DROP;
        }
    }

    fireProjectileSpread(x, y, angle, damage, projectile);
}

void bombArrowDropCast(Spell spell, SpellCastData *castData, float x, float y, float angle, bool initialCast) {
    (void)spell;
    (void)castData;
    (void)angle;
    (void)initialCast;
    playSound(SFX_BOMB_DROP);
    spawnParticle(PARTICLE_TYPE_BOMB, x, y, x, y, PARTICLE_TYPE_NONE, false /*enemyParticle*/);
}

void swordCast(Spell spell, SpellCastData *castData, float x, float y, float angle, bool initialCast) {
    float meleeX = x + cosf(angle) * CELL_SIZE / 2;
    float meleeY = y + sinf(angle) * CELL_SIZE / 2;
    SpellData *data = &spellData[spell];
    int damageRadius = data->meleeData.range;
    float damage = data->damage;
    float knockbackTime = data->knockbackTime;
    float knockbackAmount = data->knockbackAmount;
    damageEnemiesInArea(meleeX, meleeY, damageRadius, damage);
    knockbackEnemiesInArea(meleeX, meleeY, damageRadius, x, y, knockbackTime, knockbackAmount, false /*awayFromPlayer*/);
    // Disable cancel enemy attacks for balance - a little too strong maybe
    // cancelEnemyAttacksInArea(meleeX, meleeY, damageRadius);
    if (data->meleeData.slowTime > 0) {
        slowEnemiesInArea(meleeX, meleeY, damageRadius, data->meleeData.slowTime, data->meleeData.slowAmount);
    }
}

void explosionCast(Spell spell, SpellCastData *castData, float x, float y, float angle, bool initialCast) {
    SpellData *data = &spellData[spell];
    int damageRadius = data->meleeData.range;
    float damage = data->damage;
    float knockbackTime = data->knockbackTime;
    float knockbackAmount = data->knockbackAmount;
    damageEnemiesInArea(x, y, damageRadius, damage);
    knockbackEnemiesInArea(x, y, damageRadius, x, y, knockbackTime, knockbackAmount, false /*awayFromPlayer*/);
    if (data->meleeData.slowTime > 0) {
        slowEnemiesInArea(x, y, damageRadius, data->meleeData.slowTime, data->meleeData.slowAmount);
    }
    screenShake(SCREEN_SHAKE_MEDIUM);
}

void chainLightningLinkCast(Spell spell, SpellCastData *castData, float x, float y, float angle, bool initialCast) {
    SpellData *data = &spellData[SPELL_CHAIN_LIGHTNING];
    startChainLightning(x, y, data->damage, (int)data->projectileData.primaryData);
}

void turretCast(Spell spell, SpellCastData *castData, float x, float y, float angle, bool initialCast) {
    spawnTurret();
}

SpellData baseSpellData[SPELL_COUNT] = {
    [SPELL_NONE] = {
        .spellIconIndex = BITMAP_SPELL_EMPTY_ICON,
        .sfx = SFX_NONE,
        .secondarySfx = SFX_NONE,
    },
    [SPELL_FIREBALL] = {
        .spellIconIndex = BITMAP_SPELL_FIREBALL_ICON,
        .cast = projectileCast,
        .sfx = SFX_FIREBALL,
        .secondarySfx = SFX_NONE,
        .cooldown = 0.5f,
        .damage = 10,
        .projectileData = (SpellProjectileData) {
            .speed = 75.0f,
            .textureIndex = TEXTURE_FIREBALL,
            .frameCount = 4,
            .frameTime = 0.09f
        },
        .weaponData = (WeaponData) {
            .weaponBitmapTable = BITMAP_TABLE_HAND,
            .drawX = 126,
            .drawY = 71,
            .activeOffsetX = 0,
            .activeOffsetY = -7,
            .frameTime = 0.3f,
            .activeFrame = 0
        }
    },
    [SPELL_CHAIN_LIGHTNING] = {
        .spellIconIndex = BITMAP_SPELL_CHAIN_LIGHTNING_ICON,
        .cast = projectileCast,
        .sfx = SFX_LIGHTNING,
        .secondarySfx = SFX_NONE,
        .cooldown = 4.0f,
        .damage = 15,
        .projectileData = (SpellProjectileData) {
            .speed        = 75.0f,
            .pierceCount  = 0,
            .activeTime   = 1.0f,
            .onHitSpell   = SPELL_CHAIN_LIGHTNING_LINK,
            .primaryData  = 3, // chain count
            .textureIndex = TEXTURE_LIGHTNING_BALL,
            .frameCount   = 4,
            .frameTime    = 0.1f
        },
        .weaponData = (WeaponData) {
            .weaponBitmapTable = BITMAP_TABLE_HAND,
            .drawX = 126,
            .drawY = 71,
            .activeOffsetX = 0,
            .activeOffsetY = -7,
            .frameTime = 0.6f,
            .activeFrame = 0
        }
    },
    [SPELL_CHAIN_LIGHTNING_LINK] = {
        // Internal: stepped by chainLightningManager, not directly cast by player
        .cast = chainLightningLinkCast,
        .sfx = SFX_NONE,
        .secondarySfx = SFX_NONE,
    },
    [SPELL_SWORD] = {
        .spellIconIndex = BITMAP_SPELL_SWORD_ICON,
        .cast = swordCast,
        .sfx = SFX_SWORD,
        .secondarySfx = SFX_NONE,
        .cooldown = 0.5f,
        .damage = 15,
        .knockbackTime = 0.3f,
        .knockbackAmount = 3.0f,
        .meleeData = (MeleeData) {
            .range = CELL_SIZE * 1.5f
        },
        .weaponData = (WeaponData) {
            .weaponBitmapTable = BITMAP_TABLE_SWORD,
            .drawX = 61,
            .drawY = 8,
            .frameTime = 0.075f,
            .activeFrame = 8
        }
    },
    [SPELL_ROARING_SHOUT] = {
        .spellIconIndex = BITMAP_SPELL_ROARING_SHOUT_ICON,
        .cast = explosionCast,
        .sfx = SFX_SHOUT,
        .secondarySfx = SFX_NONE,
        .cooldown = 8.0f,
        .damage = 15,
        .knockbackTime = 0.6f,
        .knockbackAmount = 2.5f,
        .meleeData = (MeleeData) {
            .range = CELL_SIZE * 4.0f
        }
    },
    [SPELL_KNIFE] = {
        .spellIconIndex = BITMAP_SPELL_KNIFE_ICON,
        .cast = projectileCast,
        .sfx = SFX_KNIFE,
        .secondarySfx = SFX_NONE,
        .cooldown = 0.3f,
        .damage = 8,
        .projectileData = (SpellProjectileData) {
            .speed = 100.0f,
            .textureIndex = TEXTURE_KNIFE,
            .frameCount = 8,
            .frameTime = 0.07f
        },
        .weaponData = (WeaponData) {
            .weaponBitmapTable = BITMAP_TABLE_KNIFE_HAND,
            .drawX = 123,
            .drawY = 45,
            .activeOffsetX = 0,
            .activeOffsetY = 0,
            .frameTime = 0.1f,
            .activeFrame = 2
        }
    },
    [SPELL_FAN_OF_KNIVES] = {
        .spellIconIndex = BITMAP_SPELL_FAN_OF_KNIVES_ICON,
        .cast = projectileCast,
        .sfx = SFX_FAN_OF_KNIVES,
        .secondarySfx = SFX_NONE,
        .cooldown = 4.0f,
        .damage = 10,
        .repeatData = (RepeatData){
            .activationTime = 0.2f,
            .repeatCount = 0
        },
        .projectileData = (SpellProjectileData) {
            .speed = 125.0f,
            .pierceCount = 2,
            .spreadCount = 5,
            .spreadAngle = 10.0f * (PI / 180),
            .textureIndex = TEXTURE_KNIFE,
            .frameCount = 8,
            .frameTime = 0.07f
        }
    },
    [SPELL_BOW_SHOT] = {
        .spellIconIndex = BITMAP_SPELL_BOW_SHOT_ICON,
        .cast = chargedProjectileCast,
        .sfx = SFX_BOW_SHOOT, // arrow release
        .secondarySfx = SFX_NONE,
        .cooldown = 0.5f,
        .damage = 10,
        // Base stats represent charge level 1; higher levels add the bonuses below.
        .projectileData = (SpellProjectileData) {
            .speed = 40.0f,
            .pierceCount = 0,
            .textureIndex = TEXTURE_BOW_ARROW,
            .frameCount = 1,
            .frameTime = 0.09f
        },
        .weaponData = (WeaponData) {
            .weaponBitmapTable = BITMAP_TABLE_BOW,
            .drawX = 0,
            .drawY = 0,
            .activeOffsetX = 0,
            .activeOffsetY = 0,
            .frameTime = 0.15f,
            .activeFrame = 0
        },
        .chargeData = (ChargeData) {
            .levelCount = 3,
            .timePerLevel = 0.5f,
            .jiggleAtMax = true,
            .levels = {
                // Level 1 (instant draw)
                { .damageBonus = 0,  .speedBonus = 0.0f,  .pierceBonus = 0, .playerSpeedModifier = -0.50f },
                // Level 2
                { .damageBonus = 8,  .speedBonus = 30.0f, .pierceBonus = 1, .playerSpeedModifier = -0.60f },
                // Level 3 (max)
                { .damageBonus = 18, .speedBonus = 80.0f, .pierceBonus = 2, .playerSpeedModifier = -0.70f }
            },
            .sfx = SFX_BOW_DRAW,
            .secondarySfx = SFX_BOW_DRAW_BG
        }
    },
    [SPELL_NET_TRAP] = {
        .spellIconIndex = BITMAP_SPELL_NET_TRAP_ICON,
        .cast = chargedProjectileCast,
        .sfx = SFX_NET,
        .secondarySfx = SFX_NET_BG,
        .cooldown = 4.0f,
        .damage = 10,
        // Base stats represent charge level 1; higher levels add the speed bonuses below.
        .projectileData = (SpellProjectileData) {
            .speed = 100.0f,
            .acceleration = 0.0f,
            .knockbackTime = 0.2f,
            .knockbackAmount = -3.0f,
            .knockbackFromProjectile = true,
            .knockbackRadius = 3.0f,
            .textureIndex = TEXTURE_NET,
            .frameCount = 5,
            .frameTime = 0.05f,
            .holdLastFrame = true
        }
    },
    [SPELL_TURRET] = {
        .spellIconIndex = BITMAP_SPELL_BOW_SHOT_ICON,
        .cast = turretCast,
        .sfx = SFX_FIREBALL,
        .secondarySfx = SFX_NONE,
        .cooldown = 10.0f,
        .damage = 10, // matches Fireball
        // Fireball projectile stats, rendered with the arrow texture
        .projectileData = (SpellProjectileData) {
            .speed = 50.0f,
            .textureIndex = TEXTURE_BOW_ARROW,
            .frameCount = 1,
            .frameTime = 0.09f,
            .hitParticle = PARTICLE_TYPE_BLANK
        },
        .turretData = (TurretData) {
            .fireMode = TURRET_FIRE_RING,
            .lifetime = 7.0f,
            .fireInterval = 1.5f,
            .range = 150.0f,
            .ringProjectileCount = 16,
            .targetedProjectileCount = 1,
            .targetedSpreadAngle = 15.0f * (PI / 180.0f)
        }
    },
    [SPELL_BOMB_ARROW_DROP] = {
        // Internal: cast as the onHitSpell of an armed Bomb Arrow, not equippable
        .spellIconIndex = BITMAP_SPELL_EMPTY_ICON,
        .cast = bombArrowDropCast,
        .sfx = SFX_NONE,
        .secondarySfx = SFX_NONE,
    }
};

// String id for each spell, used to build the localization.csv keys
// spell.<id>.name and spell.<id>.desc. Order matches the Spell enum.
static const char *SPELL_LOC_IDS[SPELL_COUNT] = {
    [SPELL_NONE] = "none",
    [SPELL_FIREBALL] = "fireball",
    [SPELL_CHAIN_LIGHTNING] = "chain_lightning",
    [SPELL_CHAIN_LIGHTNING_LINK] = "chain_lightning_link",
    [SPELL_LASER] = "laser",
    [SPELL_SWORD] = "sword",
    [SPELL_ROARING_SHOUT] = "roaring_shout",
    [SPELL_KNIFE] = "knife",
    [SPELL_FAN_OF_KNIVES] = "fan_of_knives",
    [SPELL_BOW_SHOT] = "bow_shot",
    [SPELL_TURRET] = "turret",
    [SPELL_NET_TRAP] = "net_trap",
    [SPELL_BOMB_ARROW_DROP] = NULL, // internal, no localization
};

void initSpellLocalization(void) {
    for (int i = 0; i < SPELL_COUNT; i++) {
        const char *id = SPELL_LOC_IDS[i];
        if (id == NULL) {
            continue;
        }
        char *nameKey = NULL;
        char *descKey = NULL;
        pd->system->formatString(&nameKey, "spell.%s.name", id);
        pd->system->formatString(&descKey, "spell.%s.desc", id);
        baseSpellData[i].name = (char *)getLocText(nameKey, LANGUAGE_EN);
        baseSpellData[i].name_jp = (char *)getLocText(nameKey, LANGUAGE_JP);
        baseSpellData[i].description = (char *)getLocText(descKey, LANGUAGE_EN);
        baseSpellData[i].description_jp = (char *)getLocText(descKey, LANGUAGE_JP);
        pd->system->realloc(nameKey, 0);
        pd->system->realloc(descKey, 0);
    }
}