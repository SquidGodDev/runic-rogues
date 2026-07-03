#include "game/player/spells/spellData.h"
#include "game/player/player.h"
#include "game/entities/projectiles.h"
#include "game/enemies/enemies.h"
#include "game/UI/UI.h"
#include "game/systems/chainLightningManager.h"
#include "game/entities/turret.h"

// Spawn the projectile slightly to the right to match hand
#define FIRE_ANGLE_OFFSET PI / 8
#define FIRE_POSITION_OFFSET CELL_SIZE / 2

static void fireProjectileSpread(float x, float y, float angle, int damage, SpellProjectileData projectileData) {
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
    int damage = data.damage + level->damageBonus;

    fireProjectileSpread(x, y, angle, damage, projectile);
}

void swordCast(Spell spell, SpellCastData *castData, float x, float y, float angle, bool initialCast) {
    float meleeX = x + cosf(angle) * CELL_SIZE / 2;
    float meleeY = y + sinf(angle) * CELL_SIZE / 2;
    SpellData *data = &spellData[spell];
    int damageRadius = data->meleeData.range;
    int damage = data->damage;
    float knockbackTime = data->knockbackTime;
    float knockbackAmount = data->knockbackAmount;
    damageEnemiesInArea(meleeX, meleeY, damageRadius, damage);
    knockbackEnemiesInArea(meleeX, meleeY, damageRadius, x, y, knockbackTime, knockbackAmount, false /*awayFromPlayer*/);
    cancelEnemyAttacksInArea(meleeX, meleeY, damageRadius);
    if (data->meleeData.slowTime > 0) {
        slowEnemiesInArea(meleeX, meleeY, damageRadius, data->meleeData.slowTime, data->meleeData.slowAmount);
    }
}

void explosionCast(Spell spell, SpellCastData *castData, float x, float y, float angle, bool initialCast) {
    SpellData *data = &spellData[spell];
    int damageRadius = data->meleeData.range;
    int damage = data->damage;
    float knockbackTime = data->knockbackTime;
    float knockbackAmount = data->knockbackAmount;
    damageEnemiesInArea(x, y, damageRadius, damage);
    knockbackEnemiesInArea(x, y, damageRadius, x, y, knockbackTime, knockbackAmount, false /*awayFromPlayer*/);
    if (data->meleeData.slowTime > 0) {
        slowEnemiesInArea(x, y, damageRadius, data->meleeData.slowTime, data->meleeData.slowAmount);
    }
    screenShake(SCREEN_SHAKE_MEDIUM);
}

void lightningArcCast(Spell spell, SpellCastData *castData, float x, float y, float angle, bool initialCast) {
    // Damage random enemies in area and spawn a lightning particle at their locations

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
        .name = "None",
        .name_jp = "なし",
        .description = "Empty.",
        .spellIconIndex = BITMAP_SPELL_EMPTY_ICON,
    },
    [SPELL_FIREBALL] = {
        .name = "Fireball",
        .name_jp = "ファイアボール",
        .description = "Shoot a projectile that deals medium damage to one enemy.",
        .description_jp = "敵に中程度のダメージを与える弾を発射する。",
        .spellIconIndex = BITMAP_SPELL_FIREBALL_ICON,
        .cast = projectileCast,
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
        .name = "Lightning",
        .name_jp = "ライトニング",
        .description = "Fire a bolt that chains to up to 3 nearby enemies.",
        .description_jp = "",
        .spellIconIndex = BITMAP_SPELL_CHAIN_LIGHTNING_ICON,
        .cast = projectileCast,
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
        .name = "Chain Lightning Link",
        .cast = chainLightningLinkCast,
    },
    [SPELL_SWORD] = {
        .name = "Sword",
        .name_jp = "ソード",
        .description = "Perform a melee attack that deals damage to nearby enemies",
        .description_jp = "近くの敵にダメージを与える近接攻撃を行う",
        .spellIconIndex = BITMAP_SPELL_SWORD_ICON,
        .cast = swordCast,
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
        .name = "Roaring Shout",
        .name_jp = "轟音",
        .description = "Emit a powerful shout that damages and knocks back nearby enemies.",
        .description_jp = "",
        .spellIconIndex = BITMAP_SPELL_ROARING_SHOUT_ICON,
        .cast = explosionCast,
        .cooldown = 8.0f,
        .damage = 15,
        .knockbackTime = 0.6f,
        .knockbackAmount = 2.5f,
        .meleeData = (MeleeData) {
            .range = CELL_SIZE * 4.0f
        }
    },
    [SPELL_KNIFE] = {
        .name = "Knife",
        .name_jp = "ナイフ",
        .description = "",
        .description_jp = "",
        .spellIconIndex = BITMAP_SPELL_KNIFE_ICON,
        .cast = projectileCast,
        .cooldown = 0.3f,
        .damage = 5,
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
        .name = "Fan of Knives",
        .name_jp = "ナイフの扇",
        .description = "",
        .description_jp = "",
        .spellIconIndex = BITMAP_SPELL_FAN_OF_KNIVES_ICON,
        .cast = projectileCast,
        .cooldown = 4.0f,
        .damage = 5,
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
        .name = "Bow Shot",
        .name_jp = "ボウショット",
        .description = "Hold to draw the bow. The longer the charge, the more damage, speed, and pierce the arrow gains.",
        .description_jp = "長く引くほど威力・速度・貫通が増す矢を放つ。",
        .spellIconIndex = BITMAP_SPELL_BOW_SHOT_ICON,
        .cast = chargedProjectileCast,
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
            }
        }
    },
    [SPELL_TURRET] = {
        .name = "Turret",
        .name_jp = "タレット",
        .description = "Deploy a turret that fires a ring of arrows in all directions for a short time.",
        .description_jp = "一定時間、全方向に矢を放つタレットを設置する。",
        .spellIconIndex = BITMAP_SPELL_BOW_SHOT_ICON,
        .cast = turretCast,
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
    [SPELL_NET_TRAP] = {
        .name = "Net Trap",
        .name_jp = "ネットトラップ",
        .description = "Hold to charge, then fire a net that drags struck enemies toward you. Charging only speeds up the throw.",
        .description_jp = "溜めて放つと敵を引き寄せる網を投げる。溜めるほど投げる速度が上がる。",
        .spellIconIndex = BITMAP_SPELL_NET_TRAP_ICON,
        .cast = chargedProjectileCast,
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
    }
};