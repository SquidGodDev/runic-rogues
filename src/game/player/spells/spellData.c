#include "game/player/spells/spellData.h"
#include "game/player/player.h"
#include "game/entities/projectiles.h"
#include "game/enemies/enemies.h"
#include "game/UI/UI.h"
#include "game/systems/chainLightningManager.h"

// Spawn the projectile slightly to the right to match hand
#define FIRE_ANGLE_OFFSET PI / 8
#define FIRE_POSITION_OFFSET CELL_SIZE / 2

void projectileCast(Spell spell, SpellCastData *castData, float x, float y, float angle, bool initialCast) {
    SpellData data = spellData[spell];
    int spreadCount = data.projectileData.spreadCount;
    spreadCount = spreadCount < 1 ? 1 : spreadCount;
    float spreadAngle = data.projectileData.spreadAngle;
    float startAngle = angle - (spreadAngle * (spreadCount - 1) / 2);
    for (int i = 0; i < spreadCount; i++) {
        float castAngle = startAngle + spreadAngle * i;
        float projectileX = x + cosf(castAngle + FIRE_ANGLE_OFFSET) * FIRE_POSITION_OFFSET;
        float projectileY = y + sinf(castAngle + FIRE_ANGLE_OFFSET) * FIRE_POSITION_OFFSET;
        firePlayerProjectile(projectileX, projectileY, cosf(castAngle), sinf(castAngle), data.damage, data.projectileData);
    }
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
    knockbackEnemiesInArea(meleeX, meleeY, damageRadius, knockbackTime, knockbackAmount, false /*awayFromPlayer*/);
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
    knockbackEnemiesInArea(x, y, damageRadius, knockbackTime, knockbackAmount, false /*awayFromPlayer*/);
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
            .frameTime = 0.6f,
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
            .activeFrame = 5
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
    }
};