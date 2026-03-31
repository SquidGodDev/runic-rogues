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
    recalculateRuneEffectsToSpells();

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

void recalculateRuneEffectsToSpells(void) {
    for (int i = 0; i < SPELL_COUNT; i++) {
        // Reset spell data to base values before reapplying rune effects
        spellData[i] = baseSpellData[i];
    }
    applyRuneEffectsToSpells();
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
    if (data->repeatData.repeatCount > 0) {
        spellCastData[index] = (SpellCastData) {
            .spell = spell,
            .maxCooldown = data->cooldown,
            .castTime = data->repeatData.activationTime * (data->repeatData.repeatCount + 1),
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
    bool cast = castSpell(BASIC_SPELL_INDEX);
    if (cast) {
        signalRuneTrigger(RUNE_TRIGGER_BASIC_CAST, 1.0f);
    }
    return cast;
}

bool castSpecialSpell(void) {
    bool cast = castSpell(SPECIAL_SPELL_INDEX);
    if (cast) {
        signalRuneTrigger(RUNE_TRIGGER_SPECIAL_CAST, 1.0f);
    }
    return cast;
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
        if (data->repeatData.repeatCount > 0) {
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
void modifyProjectileSpeed(SpellData *spell, float value, ModifyOperation operation) {
    switch (operation) {
        case MODIFY_OP_ADD:
            spell->projectileData.speed += value;
            break;
        case MODIFY_OP_MULTIPLY:
            spell->projectileData.speed *= 1.0f + value;
            break;
        case MODIFY_OP_REPLACE_IF_ZERO:
            if (spell->projectileData.speed == 0.0f) {
                spell->projectileData.speed = value;
            }
            break;
    }
}

void modifyProjectileAcceleration(SpellData *spell, float value, ModifyOperation operation) {
    switch (operation) {
        case MODIFY_OP_ADD:
            spell->projectileData.acceleration += value;
            break;
        case MODIFY_OP_MULTIPLY:
            spell->projectileData.acceleration *= 1.0f + value;
            break;
        case MODIFY_OP_REPLACE_IF_ZERO:
            if (spell->projectileData.acceleration == 0.0f) {
                spell->projectileData.acceleration = value;
            }
            break;
    }
}

void modifyProjectilePierceCount(SpellData *spell, float value, ModifyOperation operation) {
    switch (operation) {
        case MODIFY_OP_ADD:
            spell->projectileData.pierceCount += (int)value;
            break;
        case MODIFY_OP_MULTIPLY:
            spell->projectileData.pierceCount = (int)(spell->projectileData.pierceCount * value);
            break;
        case MODIFY_OP_REPLACE_IF_ZERO:
            if (spell->projectileData.pierceCount == 0) {
                spell->projectileData.pierceCount = (int)value;
            }
            break;
    }
}

void modifySlowTime(SpellData *spell, float value, ModifyOperation operation) {
    switch (operation) {
        case MODIFY_OP_ADD:
            spell->projectileData.slowTime += value;
            spell->meleeData.slowTime += value;
            break;
        case MODIFY_OP_MULTIPLY:
            spell->projectileData.slowTime *= value;
            spell->meleeData.slowTime *= value;
            break;
        case MODIFY_OP_REPLACE_IF_ZERO:
            if (spell->projectileData.slowTime == 0.0f) {
                spell->projectileData.slowTime = value;
            }
            if (spell->meleeData.slowTime == 0.0f) {
                spell->meleeData.slowTime = value;
            }
            break;
    }
}

void modifySlowAmount(SpellData *spell, float value, ModifyOperation operation) {
    switch (operation) {
        case MODIFY_OP_ADD:
            spell->projectileData.slowAmount += value;
            spell->meleeData.slowAmount += value;
            break;
        case MODIFY_OP_MULTIPLY:
            spell->projectileData.slowAmount *= value;
            spell->meleeData.slowAmount *= value;
            break;
        case MODIFY_OP_REPLACE_IF_ZERO:
            if (spell->projectileData.slowAmount == 0.0f) {
                spell->projectileData.slowAmount = value;
            }
            if (spell->meleeData.slowAmount == 0.0f) {
                spell->meleeData.slowAmount = value;
            }
            break;
    }
}

void modifyProjectileSpreadCount(SpellData *spell, float value, ModifyOperation operation) {
    switch (operation) {
        case MODIFY_OP_ADD:
            if (spell->projectileData.spreadCount == 0) {
                spell->projectileData.spreadCount = 1;
            }
            spell->projectileData.spreadCount += (int)value;
            break;
        case MODIFY_OP_MULTIPLY:
            if (spell->projectileData.spreadCount == 0) {
                spell->projectileData.spreadCount = 1;
            }
            spell->projectileData.spreadCount = (int)(spell->projectileData.spreadCount * value);
            break;
        case MODIFY_OP_REPLACE_IF_ZERO:
            if (spell->projectileData.spreadCount == 0) {
                spell->projectileData.spreadCount = (int)value;
            }
            break;
    }
}

void modifyProjectileSpreadAngle(SpellData *spell, float value, ModifyOperation operation) {
    switch (operation) {
        case MODIFY_OP_ADD:
            spell->projectileData.spreadAngle += value;
            break;
        case MODIFY_OP_MULTIPLY:
            spell->projectileData.spreadAngle *= value;
            break;
        case MODIFY_OP_REPLACE_IF_ZERO:
            if (spell->projectileData.spreadAngle == 0.0f) {
                spell->projectileData.spreadAngle = value;
            }
            break;
    }
}

void modifyProjectileActiveTime(SpellData *spell, float value, ModifyOperation operation) {
    switch (operation) {
        case MODIFY_OP_ADD:
            spell->projectileData.activeTime += value;
            break;
        case MODIFY_OP_MULTIPLY:
            spell->projectileData.activeTime *= value;
            break;
        case MODIFY_OP_REPLACE_IF_ZERO:
            if (spell->projectileData.activeTime == 0.0f) {
                spell->projectileData.activeTime = value;
            }
            break;
    }
}

void setProjectileOnHitSpell(SpellData *spell, float value, ModifyOperation operation) {
    switch (operation) {
        case MODIFY_OP_ADD:
        case MODIFY_OP_MULTIPLY:
            spell->projectileData.onHitSpell = (Spell)value;
            break;
        case MODIFY_OP_REPLACE_IF_ZERO:
            if (spell->projectileData.onHitSpell == 0) {
                spell->projectileData.onHitSpell = (Spell)value;
            }
            break;
    }
}

void setProjectileHitParticle(SpellData *spell, float value, ModifyOperation operation) {
    switch (operation) {
        case MODIFY_OP_ADD:
        case MODIFY_OP_MULTIPLY:
            spell->projectileData.hitParticle = (ParticleType)value;
            break;
        case MODIFY_OP_REPLACE_IF_ZERO:
            if (spell->projectileData.hitParticle == 0) {
                spell->projectileData.hitParticle = (ParticleType)value;
            }
            break;
    }
}

void setProjectileTimeoutParticle(SpellData *spell, float value, ModifyOperation operation) {
    switch (operation) {
        case MODIFY_OP_ADD:
        case MODIFY_OP_MULTIPLY:
            spell->projectileData.timeoutParticle = (ParticleType)value;
            break;
        case MODIFY_OP_REPLACE_IF_ZERO:
            if (spell->projectileData.timeoutParticle == 0) {
                spell->projectileData.timeoutParticle = (ParticleType)value;
            }
            break;
    }
}

// SpellData modifiers
void modifySpellCooldown(SpellData *spell, float value, ModifyOperation operation) {
    switch (operation) {
        case MODIFY_OP_ADD:
            spell->cooldown += value;
            break;
        case MODIFY_OP_MULTIPLY:
            spell->cooldown *= 1.0f + value;
            break;
        case MODIFY_OP_REPLACE_IF_ZERO:
            if (spell->cooldown == 0.0f) {
                spell->cooldown = value;
            }
            break;
    }
}

void modifySpellDamage(SpellData *spell, float value, ModifyOperation operation) {
    switch (operation) {
        case MODIFY_OP_ADD:
            spell->damage += (int)value;
            break;
        case MODIFY_OP_MULTIPLY:
            spell->damage = (int)(spell->damage * value);
            break;
        case MODIFY_OP_REPLACE_IF_ZERO:
            if (spell->damage == 0) {
                spell->damage = (int)value;
            }
            break;
    }
}

void modifySpellKnockbackTime(SpellData *spell, float value, ModifyOperation operation) {
    switch (operation) {
        case MODIFY_OP_ADD:
            spell->knockbackTime += value;
            spell->projectileData.knockbackTime += value;
            break;
        case MODIFY_OP_MULTIPLY:
            spell->knockbackTime *= value;
            spell->projectileData.knockbackTime *= value;
            break;
        case MODIFY_OP_REPLACE_IF_ZERO:
            if (spell->knockbackTime == 0.0f) {
                spell->knockbackTime = value;
            }
            if (spell->projectileData.knockbackTime == 0.0f) {
                spell->projectileData.knockbackTime = value;
            }
            break;
    }
}

void modifySpellKnockbackAmount(SpellData *spell, float value, ModifyOperation operation) {
    switch (operation) {
        case MODIFY_OP_ADD:
            spell->knockbackAmount += value;
            spell->projectileData.knockbackAmount += value;
            break;
        case MODIFY_OP_MULTIPLY:
            spell->knockbackAmount *= value;
            spell->projectileData.knockbackAmount *= value;
            break;
        case MODIFY_OP_REPLACE_IF_ZERO:
            if (spell->knockbackAmount == 0.0f) {
                spell->knockbackAmount = value;
            }
            if (spell->projectileData.knockbackAmount == 0.0f) {
                spell->projectileData.knockbackAmount = value;
            }
            break;
    }
}

// RepeatData modifiers
void modifyRepeatCount(SpellData *spell, float value, ModifyOperation operation) {
    switch (operation) {
        case MODIFY_OP_ADD:
            spell->repeatData.repeatCount += (int)value;
            break;
        case MODIFY_OP_MULTIPLY:
            spell->repeatData.repeatCount = (int)(spell->repeatData.repeatCount * value);
            break;
        case MODIFY_OP_REPLACE_IF_ZERO:
            if (spell->repeatData.repeatCount == 0) {
                spell->repeatData.repeatCount = (int)value;
            }
            break;
    }
}

void modifyRepeatActivationTime(SpellData *spell, float value, ModifyOperation operation) {
    switch (operation) {
        case MODIFY_OP_ADD:
            spell->repeatData.activationTime += value;
            break;
        case MODIFY_OP_MULTIPLY:
            spell->repeatData.activationTime *= value;
            break;
        case MODIFY_OP_REPLACE_IF_ZERO:
            if (spell->repeatData.activationTime == 0.0f) {
                spell->repeatData.activationTime = value;
            }
            break;
    }
}