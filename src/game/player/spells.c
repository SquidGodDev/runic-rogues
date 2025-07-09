#include "globals.h"

#include "game/entities/projectiles.h"
#include "game/rendering/textures.h"
#include "game/player/player.h"
#include "game/player/spells.h"
#include "game/systems/audioManager.h"

// Spawn the projectile slightly to the right to match hand
#define FIRE_ANGLE_OFFSET PI / 8
#define FIRE_POSITION_OFFSET CELL_SIZE / 2

static SpellData spellData[SPELL_COUNT];

static Spell heldSpell;
static Spell currentSpell;
static float spellActiveTime;
static float spellActivationTime;
static int spellActivateCount;

void initSpellData(void) {
    spellData[SPELL_NONE] = (SpellData) {
        .pickupTexture = NULL,
        .spellTexture = NULL,
        .type = SPELL_TYPE_INSTANT,
        .damage = 0,
        .count = 0,
        .speed = 0,
        .activeTime = 0,
        .activationTime = 0
    };
    spellData[SPELL_FIREBALL] = (SpellData) {
        .pickupTexture = TEXTURES[TEXTURE_SPELL_FIREBALL],
        .spellTexture = TEXTURES[TEXTURE_PROJECTILE],
        .type = SPELL_TYPE_INSTANT,
        .damage = 3,
        .speed = TARGET_FPS * 1.5f
    };
    spellData[SPELL_TRIPLE_SHOT] = (SpellData) {
        .pickupTexture = TEXTURES[TEXTURE_SPELL_TRIPLE_SHOT],
        .spellTexture = TEXTURES[TEXTURE_PROJECTILE],
        .type = SPELL_TYPE_REPEAT,
        .damage = 1,
        .count = 3,
        .speed = TARGET_FPS * 3.0f,
        .activationTime = 1.0f
    };
    spellData[SPELL_SHOTGUN] = (SpellData) {
        .pickupTexture = TEXTURES[TEXTURE_SPELL_SHOTGUN],
        .spellTexture = TEXTURES[TEXTURE_PROJECTILE],
        .type = SPELL_TYPE_INSTANT,
        .damage = 1,
        .speed = TARGET_FPS * 2.0f
    };
}

void initSpells(void) {
    heldSpell = SPELL_NONE;
    currentSpell = SPELL_NONE;
    spellActiveTime = 0.0f;
    spellActivationTime = 0.0f;
    spellActivateCount = 0;
}

uint8_t* getSpellPickupTexture(Spell spell) {
    return spellData[spell].pickupTexture;
}

float getManaPercentage(void) {
    if (currentSpell != SPELL_NONE) {
        SpellData data = spellData[currentSpell];
        if (data.type == SPELL_TYPE_REPEAT) {
            return (float)(data.count - spellActivateCount) / (float)data.count;
        } else {
            return 1.0f;
        }
    } else if (heldSpell != SPELL_NONE) {
        return 1.0f;
    } else {
        return 0.0f;
    }
}

void activateSpellEffect(Spell spell) {
    SpellData data = spellData[spell];
    float projectileX = playerX + cosf(playerAngle + FIRE_ANGLE_OFFSET) * FIRE_POSITION_OFFSET;
    float projectileY = playerY + sinf(playerAngle + FIRE_ANGLE_OFFSET) * FIRE_POSITION_OFFSET;
    float deltaX = cosf(playerAngle);
    float deltaY = sinf(playerAngle);
    float offsetAngle = PI / 8.0f;
    switch (spell) {
        case SPELL_FIREBALL:
            fireProjectile(projectileX, projectileY, deltaX, deltaY, data.speed, data.damage, data.spellTexture, 0 /*isEnemy*/);
            break;
        case SPELL_TRIPLE_SHOT:
            fireProjectile(projectileX, projectileY, deltaX, deltaY, data.speed, data.damage, data.spellTexture, 0 /*isEnemy*/);
            break;
        case SPELL_SHOTGUN:
            fireProjectile(projectileX, projectileY, deltaX, deltaY, data.speed, data.damage, data.spellTexture, 0 /*isEnemy*/);
            fireProjectile(projectileX, projectileY, cosf(playerAngle + offsetAngle), sinf(playerAngle + offsetAngle), data.speed, data.damage, data.spellTexture, 0 /*isEnemy*/);
            fireProjectile(projectileX, projectileY, cosf(playerAngle - offsetAngle), sinf(playerAngle - offsetAngle), data.speed, data.damage, data.spellTexture, 0 /*isEnemy*/);
            break;
        default:
            break;
    }
}

void setHeldSpell(Spell spell) {
    heldSpell = spell;
}

int castHeldSpell(void) {
    if (heldSpell == SPELL_NONE) {
        return 0;
    }

    currentSpell = heldSpell;
    heldSpell = SPELL_NONE;

    playSound(SFX_SHOOT);
    SpellData data = spellData[currentSpell];
    if (data.type == SPELL_TYPE_INSTANT) {
        activateSpellEffect(currentSpell);
        currentSpell = SPELL_NONE;
    } else if (data.type == SPELL_TYPE_REPEAT) {
        activateSpellEffect(currentSpell);
        spellActivateCount = 1;
        spellActivationTime = data.activationTime;
    }

    return 1;
}

void updateSpells(float deltaTime) {
    if (currentSpell == SPELL_NONE) {
        return;
    }

    SpellData data = spellData[currentSpell];
    if (data.type == SPELL_TYPE_REPEAT) {
        spellActivationTime -= deltaTime;
        if (spellActivationTime <= 0.0f) {
            activateSpellEffect(currentSpell);
            spellActivationTime = data.activationTime;
            spellActivateCount++;
            if (spellActivateCount >= data.count) {
                currentSpell = SPELL_NONE;
            }
        }
    }
}