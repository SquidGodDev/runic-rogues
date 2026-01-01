#include "globals/globals.h"
#include "globals/helpers.h"

#include "game/entities/projectiles.h"
#include "game/UI/UI.h"
#include "game/player/player.h"
#include "game/player/playerStats.h"
#include "game/player/classes/classData.h"
#include "game/player/spells/spells.h"
#include "game/systems/audioManager.h"
#include "game/systems/timerManager.h"
#include "game/enemies/enemies.h"
#include "game/map/map.h"

// Spawn the projectile slightly to the right to match hand
#define FIRE_ANGLE_OFFSET PI / 8
#define FIRE_POSITION_OFFSET CELL_SIZE / 2

static Spell spellDeck[SPELL_EQUIP_MAX];
static SpellCastData spellCastData[SPELL_EQUIP_MAX];
float spellCooldowns[SPELL_EQUIP_MAX];
static int utilitySpellCount;
static float utilitySpellCharge;

void fireSpellProjectile(Spell spell, float angle) {
    SpellData data = spellData[spell];
    float projectileX = playerX + cosf(angle + FIRE_ANGLE_OFFSET) * FIRE_POSITION_OFFSET;
    float projectileY = playerY + sinf(angle + FIRE_ANGLE_OFFSET) * FIRE_POSITION_OFFSET;
    firePlayerProjectile(projectileX, projectileY, cosf(angle), sinf(angle), data.damage, data.projectileData);
}

void fireballCast(Spell spell, SpellCastData *castData, float angle, bool initialCast) {
    fireSpellProjectile(spell, angle);
}

void laserRepeatCallback(int repeatCount, void *data) {
    SpellCastData *castData = (SpellCastData *)data;
    fireSpellProjectile(castData->spell, castData->initialCastAngle);
}

void laserCast(Spell spell, SpellCastData *castData, float angle, bool initialCast) {
    fireSpellProjectile(spell, angle);
    if (initialCast) {
        addTimer(castData->activationTime, castData->activationCount - 1, laserRepeatCallback, (void *)castData);
    }
}

void swordCast(Spell spell, SpellCastData *castData, float angle, bool initialCast) {
    float meleeX = playerX + cosf(angle) * CELL_SIZE / 2;
    float meleeY = playerY + sinf(angle) * CELL_SIZE / 2;
    SpellData *data = &spellData[spell];
    int damageRadius = data->meleeData.range;
    int damage = data->damage;
    float knockbackTime = data->knockbackTime;
    float knockbackAmount = data->knockbackAmount;
    damageEnemiesInArea(meleeX, meleeY, damageRadius, damage);
    knockbackEnemiesInArea(meleeX, meleeY, damageRadius, knockbackTime, knockbackAmount);
}

SpellData baseSpellData[SPELL_COUNT] = {
    [SPELL_NONE] = {
        .name = "None",
        .name_jp = "なし",
        .description = "Empty.",
        .spellIconIndex = BITMAP_SPELL_EMPTY_ICON,
    },
    [SPELL_ARCANE_BOLT] = {
        .name = "Arcane Bolt",
        .name_jp = "アーケインボルト",
        .description = "Shoot a projectile that deals medium damage to one enemy.",
        .description_jp = "敵に中程度のダメージを与える弾を発射する。",
        .pickupTextureIndex = TEXTURE_SPELL_FIREBALL,
        .spellTextureIndex = TEXTURE_PROJECTILE,
        .spellIconIndex = BITMAP_SPELL_FIREBALL_ICON,
        .type = SPELL_TYPE_INSTANT,
        .cast = fireballCast,
        .cooldown = 0.5f,
        .damage = 10,
        .projectileData = (SpellProjectileData) {
            .speed = 75.0f
        },
        .weaponData = (WeaponData) {
            .weaponBitmapTable = BITMAP_TABLE_HAND,
            .drawX = 126,
            .drawY = 75,
            .activeOffsetX = 0,
            .activeOffsetY = -7,
            .frameTime = 0.6f,
            .activeFrame = 0
        }
    },
    [SPELL_FIREBALL] = {
        .name = "Fireball",
        .name_jp = "ファイアボール",
        .description = "Shoot a projectile that deals medium damage to one enemy.",
        .description_jp = "敵に中程度のダメージを与える弾を発射する。",
        .pickupTextureIndex = TEXTURE_SPELL_FIREBALL,
        .spellTextureIndex = TEXTURE_PROJECTILE,
        .spellIconIndex = BITMAP_SPELL_FIREBALL_ICON,
        .type = SPELL_TYPE_INSTANT,
        .cast = fireballCast,
        .cooldown = 0.5f,
        .damage = 10,
        .projectileData = (SpellProjectileData) {
            .speed = 75.0f
        }
    },
    [SPELL_LASER] = {
        .name = "Laser",
        .name_jp = "レーザー",
        .description = "Fire a continuous shot of projectiles in a straight line.",
        .description_jp = "直線上に弾を連続で発射する。",
        .pickupTextureIndex = TEXTURE_SPELL_LASER,
        .spellIconIndex = BITMAP_SPELL_LASER_ICON,
        .type = SPELL_TYPE_REPEAT,
        .cast = laserCast,
        .cooldown = 6.0f,
        .manaCost = 20.0f,
        .damage = 10,
        .playerSpeedModifier = -0.7f,
        .useInitialCastAngle = true,
        .repeatData = (RepeatData){
            .activationTime = 0.1f,
            .repeatCount = 5
        },
        .projectileData = (SpellProjectileData) {
            .hitCount = 3,
            .speed = 300.0f
        }
    },
    [SPELL_SWORD] = {
        .name = "Melee",
        .name_jp = "近接攻撃",
        .description = "Perform a melee attack that deals damage to nearby enemies",
        .description_jp = "近くの敵にダメージを与える近接攻撃を行う",
        .pickupTextureIndex = TEXTURE_SPELL_ARC_LIGHTNING, // TEMP
        .spellIconIndex = BITMAP_SPELL_ARC_LIGHTNING_ICON, // TEMP
        .type = SPELL_TYPE_INSTANT,
        .cast = swordCast,
        .cooldown = 0.5f,
        .damage = 15,
        .knockbackTime = 0.5f,
        .knockbackAmount = 8.0f,
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
    }
};

SpellData spellData[SPELL_COUNT];

void initSpells(void) {
    for (int i = 0; i < SPELL_COUNT; i++) {
        spellData[i] = baseSpellData[i];
    }

    for (int i = 0; i < SPELL_EQUIP_MAX; i++) {
        spellDeck[i] = SPELL_NONE;
    }

    const ClassData *classData = getCurrentClassData();
    spellDeck[BASIC_SPELL_INDEX] = classData->basicSpell;
    spellDeck[SPECIAL_SPELL_INDEX] = classData->specialSpell;
}

void resetSpellState(void) {
    for (int i = 0; i < SPELL_EQUIP_MAX; i++) {
        spellCastData[i] = (SpellCastData) {
            .spell = SPELL_NONE,
            .maxCooldown = 0.0f,
            .castTime = 0.0f,
            .activationTime = 0.0f,
            .activationCount = 0,
            .initialCastAngle = 0.0f
        };
    };

    for (int i = 0; i < SPELL_EQUIP_MAX; i++) {
        spellCooldowns[i] = 0.0f;
    }

    utilitySpellCount = getPlayerUtilitySpellCount();
    utilitySpellCharge = 0.0f;
}

Spell *getSpellDeck(void) {
    return spellDeck;
}

bool spellInDeck(Spell spell) {
    for (int i = 0; i < SPELL_EQUIP_MAX; i++) {
        if (spellDeck[i] == SPELL_NONE) {
            continue;
        }
        if (spellDeck[i] == spell) {
            return true;
        }
    }
    return false;
}

void addSpellToDeck(Spell spell) {
    for (int i = 0; i < SPELL_EQUIP_MAX; i++) {
        if (spellDeck[i] == SPELL_NONE) {
            spellDeck[i] = spell;
            break;
        }
    }
}

void swapSpells(int spellIndex1, int spellIndex2) {
    if (spellIndex1 < 0 || spellIndex1 >= SPELL_EQUIP_MAX || spellIndex2 < 0 || spellIndex2 >= SPELL_EQUIP_MAX) {
        return;
    }
    Spell temp = spellDeck[spellIndex1];
    spellDeck[spellIndex1] = spellDeck[spellIndex2];
    spellDeck[spellIndex2] = temp;
}

int getSpellPickupTextureIndex(Spell spell) {
    return spellData[spell].pickupTextureIndex;
}

LCDBitmap* getSpellIcon(Spell spell) {
    return BITMAPS[spellData[spell].spellIconIndex];
}

Spell getDeckSpell(int index) {
    return spellDeck[index];
}

Spell getBasicSpell(void) {
    return spellDeck[BASIC_SPELL_INDEX];
}

Spell getSpecialSpell(void) {
    return spellDeck[SPECIAL_SPELL_INDEX];
}

float getSpellCooldownPercentage(int index) {
    float maxCooldown = spellCastData[index].maxCooldown;
    if (spellCooldowns[index] <= 0.0f || maxCooldown <= 0.0f) {
        return 0.0f;
    }

    return spellCooldowns[index] / maxCooldown;
}

float getPlayerSpeedModifier(void) {
    float totalSpeedModifier = 1.0f;
    for (int i = 0; i < SPELL_EQUIP_MAX; i++) {
        SpellCastData *castData = &spellCastData[i];
        Spell spell = castData->spell;
        if (spell == SPELL_NONE) {
            continue;
        }
        SpellData *data = &spellData[spell];
        float speedModifier = data->playerSpeedModifier;
        if (speedModifier != 0) {
            totalSpeedModifier *= 1.0f + speedModifier;
        }
    }
    return totalSpeedModifier;
}

int getMaxUtilitySpells(void) {
    return getPlayerUtilitySpellCount();
}

int getUtilitySpellCount(void) {
    return utilitySpellCount;
}

float getUtilitySpellCharge(void) {
    return utilitySpellCharge;
}

void castUtilitySpell(void) {
    if (utilitySpellCount > 0) {
        utilitySpellCount -= 1;
        playerDash();
    }
}

void activateSpellEffect(int index, float angle, bool initialCast) {
    Spell spell = spellDeck[index];
    SpellCastData *castData = &spellCastData[index];
    SpellData *data = &spellData[spell];
    data->cast(spell, castData, angle, initialCast);
}

bool spellIsCastable(int index) {
    if (spellCooldowns[index] > 0.0f) {
        return false;
    }

    Spell spell = spellDeck[index];
    if (spell == SPELL_NONE) {
        return false;
    }

    SpellData *data = &spellData[spell];
    if (!spendMana(data->manaCost)) {
        return false;
    }

    return true;
}

bool castSpell(int index) {
    if (!spellIsCastable(index)) {
        return false;
    }

    Spell spell = spellDeck[index];
    SpellData *data = &spellData[spell];

    spellCooldowns[index] = data->cooldown;
    spellCastData[index] = (SpellCastData) {
        .spell = spell,
        .maxCooldown = data->cooldown,
        .castTime = data->repeatData.activationTime * data->repeatData.repeatCount,
        .activationTime = data->repeatData.activationTime,
        .activationCount = 0,
        .initialCastAngle = playerAngle
    };

    playSound(SFX_SHOOT);
    activateSpellEffect(index, playerAngle, true /* initialCast */);

    return true;
}

bool castBasicSpell(void) {
    return castSpell(BASIC_SPELL_INDEX);
}

bool castSpecialSpell(void) {
    return castSpell(SPECIAL_SPELL_INDEX);
}

void updateSpells(float deltaTime) {
    if (utilitySpellCount < getPlayerUtilitySpellCount()) {
        utilitySpellCharge += getPlayerUtilitySpellChargeRate() * deltaTime;
        if (utilitySpellCharge > 1.0f) {
            utilitySpellCharge = 0.0f;
            utilitySpellCount++;
        }
    }

    for (int i = 0; i < SPELL_EQUIP_MAX; i++) {
        if (spellCooldowns[i] > 0.0f) {
            spellCooldowns[i] -= deltaTime;
            if (spellCooldowns[i] < 0.0f) {
                spellCooldowns[i] = 0.0f;
            }
        }

        SpellCastData *castData = &spellCastData[i];
        if (castData->spell == SPELL_NONE) {
            continue;
        }

        SpellData *data = &spellData[castData->spell];
        if (data->type == SPELL_TYPE_REPEAT) {
            if (castData->castTime > 0.0f) {
                castData->castTime -= deltaTime;
                if (castData->castTime <= 0.0f) {
                    castData->castTime = 0.0f;
                    castData->spell = SPELL_NONE;
                    clearSpellOverlay(i);
                    continue;   
                }
            }

            castData->activationTime -= deltaTime;
            if (castData->activationTime <= 0.0f && castData->activationCount < data->repeatData.repeatCount) {
                float angle = data->useInitialCastAngle ? castData->initialCastAngle : playerAngle;
                activateSpellEffect(i, angle, false /* initialCast */);
                castData->activationTime = data->repeatData.activationTime;
            }
        } else {
            castData->spell = SPELL_NONE;
            clearSpellOverlay(i);
        }
    }    
}