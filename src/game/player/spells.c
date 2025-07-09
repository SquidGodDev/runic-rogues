#include "globals.h"
#include "helpers.h"

#include "game/entities/projectiles.h"
#include "game/rendering/textures.h"
#include "game/player/player.h"
#include "game/player/spells.h"
#include "game/systems/audioManager.h"

// Spawn the projectile slightly to the right to match hand
#define FIRE_ANGLE_OFFSET PI / 8
#define FIRE_POSITION_OFFSET CELL_SIZE / 2

#define DASH_COUNT 3
#define MAX_HELD_SPELLS 3

static SpellData spellData[SPELL_COUNT];

Queue heldSpellsQueue;
static Spell currentSpell;
static float spellActiveTime;
static float spellActivationTime;
static int spellActivateCount;

static int utilitySpellCount;

void initSpellData(void) {
    spellData[SPELL_NONE] = (SpellData) {
        .pickupTexture = NULL,
        .spellTexture = NULL,
        .spellIcon = BITMAPS[BITMAP_SPELL_EMPTY_ICON],
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
        .spellIcon = BITMAPS[BITMAP_SPELL_FIREBALL_ICON],
        .type = SPELL_TYPE_INSTANT,
        .damage = 3,
        .speed = TARGET_FPS * 1.5f
    };
    spellData[SPELL_TRIPLE_SHOT] = (SpellData) {
        .pickupTexture = TEXTURES[TEXTURE_SPELL_TRIPLE_SHOT],
        .spellTexture = TEXTURES[TEXTURE_PROJECTILE],
        .spellIcon = BITMAPS[BITMAP_SPELL_TRIPLE_SHOT_ICON],
        .type = SPELL_TYPE_REPEAT,
        .damage = 1,
        .count = 3,
        .speed = TARGET_FPS * 3.0f,
        .activationTime = 0.3f
    };
    spellData[SPELL_SHOTGUN] = (SpellData) {
        .pickupTexture = TEXTURES[TEXTURE_SPELL_SHOTGUN],
        .spellTexture = TEXTURES[TEXTURE_PROJECTILE],
        .spellIcon = BITMAPS[BITMAP_SPELL_SHOTGUN_ICON],
        .type = SPELL_TYPE_INSTANT,
        .damage = 1,
        .speed = TARGET_FPS * 2.0f
    };
}

void initSpells(void) {
    queueInit(&heldSpellsQueue);
    currentSpell = SPELL_NONE;
    spellActiveTime = 0.0f;
    spellActivationTime = 0.0f;
    spellActivateCount = 0;

    utilitySpellCount = DASH_COUNT;
}

uint8_t* getSpellPickupTexture(Spell spell) {
    return spellData[spell].pickupTexture;
}

LCDBitmap* getSpellIcon(Spell spell) {
    return spellData[spell].spellIcon;
}

Spell heldSpellsFull(void) {
    return queueCount(&heldSpellsQueue) >= MAX_HELD_SPELLS;
}

Spell getHeldSpell(int index) {
    if (index >= queueCount(&heldSpellsQueue)) {
        return SPELL_NONE;
    }
    return queuePeek(&heldSpellsQueue, index);
}

int getMaxHeldSpells(void) {
    return MAX_HELD_SPELLS;
}

float getManaPercentage(void) {
    if (currentSpell == SPELL_NONE) {
        if (queueCount(&heldSpellsQueue) == 0) {
            return 0.0f;
        } else {
            return 1.0f;
        }
    } else {
        SpellData data = spellData[currentSpell];
        if (data.type == SPELL_TYPE_REPEAT) {
            return (float)(data.count - spellActivateCount) / (float)data.count;
        } else {
            return 1.0f;
        }
    }
}

int getMaxUtilitySpells(void) {
    return DASH_COUNT;
}

int getUtilitySpellCount(void) {
    return utilitySpellCount;
}

void rechargeUtilitySpell(void) {
    utilitySpellCount = DASH_COUNT;
}

void castUtilitySpell(void) {
    if (utilitySpellCount > 0) {
        utilitySpellCount -= 1;
        playerDash();
    }
}

void activateSpellEffect(Spell spell) {
    SpellData data = spellData[spell];
    float projectileX = playerX + cosf(playerAngle + FIRE_ANGLE_OFFSET) * FIRE_POSITION_OFFSET;
    float projectileY = playerY + sinf(playerAngle + FIRE_ANGLE_OFFSET) * FIRE_POSITION_OFFSET;
    float deltaX = cosf(playerAngle);
    float deltaY = sinf(playerAngle);
    float offsetAngle = PI / 8.0f;
    if (spell == SPELL_FIREBALL) {
        fireProjectile(projectileX, projectileY, deltaX, deltaY, data.speed, data.damage, data.spellTexture, 0 /*isEnemy*/);
    } else if (spell == SPELL_TRIPLE_SHOT) {
        fireProjectile(projectileX, projectileY, deltaX, deltaY, data.speed, data.damage, data.spellTexture, 0 /*isEnemy*/);
    } else if (spell == SPELL_SHOTGUN) {
        fireProjectile(projectileX, projectileY, deltaX, deltaY, data.speed, data.damage, data.spellTexture, 0 /*isEnemy*/);
        fireProjectile(projectileX, projectileY, cosf(playerAngle + offsetAngle), sinf(playerAngle + offsetAngle), data.speed, data.damage, data.spellTexture, 0 /*isEnemy*/);
        fireProjectile(projectileX, projectileY, cosf(playerAngle - offsetAngle), sinf(playerAngle - offsetAngle), data.speed, data.damage, data.spellTexture, 0 /*isEnemy*/);
    }
}

void addHeldSpell(Spell spell) {
    queuePush(&heldSpellsQueue, spell);
}

int castHeldSpell(void) {
    if (queueIsEmpty(&heldSpellsQueue)) {
        return 0;
    }

    currentSpell = queuePop(&heldSpellsQueue);

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