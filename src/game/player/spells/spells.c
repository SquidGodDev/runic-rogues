#include "globals/globals.h"
#include "globals/helpers.h"

#include "game/entities/projectiles.h"
#include "game/UI/UI.h"
#include "game/player/player.h"
#include "game/player/playerStats.h"
#include "game/player/classes/classData.h"
#include "game/player/spells/spells.h"
#include "game/player/spells/spellData.h"
#include "game/systems/audioManager.h"
#include "game/systems/timerManager.h"
#include "game/systems/runeManager.h"
#include "game/enemies/enemies.h"
#include "game/map/map.h"

static Spell spellDeck[SPELL_EQUIP_MAX];
static SpellCastData spellCastData[SPELL_EQUIP_MAX];
float spellCooldowns[SPELL_EQUIP_MAX];
static int utilitySpellCount;
static float utilitySpellCharge;

SpellData spellData[SPELL_COUNT];

void initSpells(void) {
    for (int i = 0; i < SPELL_EQUIP_MAX; i++) {
        spellDeck[i] = SPELL_NONE;
    }

    const ClassData *classData = getCurrentClassData();
    spellDeck[BASIC_SPELL_INDEX] = classData->basicSpell;
    spellDeck[SPECIAL_SPELL_INDEX] = classData->specialSpell;
}

void resetSpellState(void) {
    for (int i = 0; i < SPELL_COUNT; i++) {
        spellData[i] = baseSpellData[i];
    }
    applyRuneEffectsToSpells();

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

void castSpellDirect(Spell spell, float x, float y, float angle) {
    SpellData *data = &spellData[spell];
    data->cast(spell, NULL, x, y, angle, true /* initialCast */);
}

void activateSpellEffect(int index, float angle, bool initialCast) {
    Spell spell = spellDeck[index];
    SpellCastData *castData = &spellCastData[index];
    SpellData *data = &spellData[spell];
    data->cast(spell, castData, playerX, playerY, angle, initialCast);
}

bool spellIsCastable(int index) {
    if (spellCooldowns[index] > 0.0f) {
        return false;
    }

    Spell spell = spellDeck[index];
    if (spell == SPELL_NONE) {
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
    if (data->type == SPELL_TYPE_REPEAT) {
        spellCastData[index] = (SpellCastData) {
            .spell = spell,
            .maxCooldown = data->cooldown,
            .castTime = data->repeatData.activationTime * data->repeatData.repeatCount,
            .activationTime = data->repeatData.activationTime,
            .activationCount = 0,
            .initialCastAngle = playerAngle
        };
    } else {
        spellCastData[index].spell = SPELL_NONE;
        spellCastData[index].maxCooldown = data->cooldown;
    }

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

// === Spell Modifier Functions ===

// SpellProjectileData modifiers
void modifyProjectileSpeed(SpellData *spell, float value) {
    spell->projectileData.speed += value;
}

void modifyProjectileAcceleration(SpellData *spell, float value) {
    spell->projectileData.acceleration += value;
}

void modifyProjectileHitCount(SpellData *spell, float value) {
    spell->projectileData.hitCount += (int)value;
}

void modifyProjectileSlowTime(SpellData *spell, float value) {
    spell->projectileData.slowTime += value;
}

void modifyProjectileSlowAmount(SpellData *spell, float value) {
    spell->projectileData.slowAmount += value;
}

void modifyProjectileSpreadCount(SpellData *spell, float value) {
    spell->projectileData.spreadCount += (int)value;
}

void modifyProjectileSpreadAngle(SpellData *spell, float value) {
    spell->projectileData.spreadAngle += value;
}

void modifyProjectileActiveTime(SpellData *spell, float value) {
    spell->projectileData.activeTime += value;
}

void setProjectileOnHitSpell(SpellData *spell, float value) {
    spell->projectileData.onHitSpell = (Spell)value;
}

void setProjectileHitParticle(SpellData *spell, float value) {
    spell->projectileData.hitParticle = (ParticleType)value;
}

void setProjectileTimeoutParticle(SpellData *spell, float value) {
    spell->projectileData.timeoutParticle = (ParticleType)value;
}

// SpellData modifiers
void modifySpellCooldown(SpellData *spell, float value) {
    spell->cooldown += value;
}

void modifySpellDamage(SpellData *spell, float value) {
    spell->damage += (int)value;
}

void modifySpellKnockbackTime(SpellData *spell, float value) {
    spell->knockbackTime += value;
}

void modifySpellKnockbackAmount(SpellData *spell, float value) {
    spell->knockbackAmount += value;
}

// RepeatData modifiers
void modifyRepeatCount(SpellData *spell, float value) {
    spell->repeatData.repeatCount += (int)value;
}

void modifyRepeatActivationTime(SpellData *spell, float value) {
    spell->repeatData.activationTime += value;
}