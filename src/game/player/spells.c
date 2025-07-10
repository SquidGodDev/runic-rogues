#include "globals.h"
#include "helpers.h"

#include "game/entities/projectiles.h"
#include "game/rendering/textures.h"
#include "game/rendering/particles.h"
#include "game/player/player.h"
#include "game/player/spells.h"
#include "game/systems/audioManager.h"
#include "game/enemies/enemies.h"
#include "game/map/map.h"

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
static float setAngle;

static int utilitySpellCount;

void initSpellData(void) {
    spellData[SPELL_NONE] = (SpellData) {
        .pickupTexture = NULL,
        .spellTexture = NULL,
        .spellIcon = BITMAPS[BITMAP_SPELL_EMPTY_ICON],
        .type = SPELL_TYPE_INSTANT,
        .damage = 0,
        .count = 0,
        .setAngle = 0,
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
        .setAngle = 0,
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
    spellData[SPELL_LASER] = (SpellData) {
        .pickupTexture = TEXTURES[TEXTURE_SPELL_LASER],
        .spellIcon = BITMAPS[BITMAP_SPELL_LASER_ICON],
        .type = SPELL_TYPE_REPEAT,
        .damage = 1,
        .activationTime = 0.1f,
        .count = 5,
        .setAngle = 1,
        .speed = TARGET_FPS * 6.0f
    };
    spellData[SPELL_ROOT_ATTACK] = (SpellData) {
        .pickupTexture = TEXTURES[TEXTURE_SPELL_ROOT_ATTACK],
        .spellIcon = BITMAPS[BITMAP_SPELL_ROOT_ATTACK_ICON],
        .type = SPELL_TYPE_INSTANT,
        .damage = 1,
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

void activateSpellEffect(Spell spell, float angle) {
    SpellData data = spellData[spell];
    float projectileX = playerX + cosf(angle + FIRE_ANGLE_OFFSET) * FIRE_POSITION_OFFSET;
    float projectileY = playerY + sinf(angle + FIRE_ANGLE_OFFSET) * FIRE_POSITION_OFFSET;
    float deltaX = cosf(angle);
    float deltaY = sinf(angle);
    float offsetAngle = PI / 8.0f;
    if (spell == SPELL_FIREBALL) {
        firePlayerProjectile(projectileX, projectileY, deltaX, deltaY, data.speed, data.damage, 1 /*hitCount*/);
    } else if (spell == SPELL_TRIPLE_SHOT) {
        firePlayerProjectile(projectileX, projectileY, deltaX, deltaY, data.speed, data.damage, 1 /*hitCount*/);
    } else if (spell == SPELL_SHOTGUN) {
        firePlayerProjectile(projectileX, projectileY, deltaX, deltaY, data.speed, data.damage, 1 /*hitCount*/);
        firePlayerProjectile(projectileX, projectileY, cosf(angle + offsetAngle), sinf(angle + offsetAngle), data.speed, data.damage, 1 /*hitCount*/);
        firePlayerProjectile(projectileX, projectileY, cosf(angle - offsetAngle), sinf(angle - offsetAngle), data.speed, data.damage, 1 /*hitCount*/);
    } else if (spell == SPELL_LASER) {
        firePlayerProjectile(projectileX, projectileY, deltaX, deltaY, data.speed, data.damage, 5 /*hitCount*/);
    } else if (spell == SPELL_ROOT_ATTACK) {
        int rootCount = 5;
        for (int root = 0; root < rootCount; root++) {
            float offsetX = deltaX * (root + 1) * CELL_SIZE;
            float offsetY = deltaY * (root + 1) * CELL_SIZE;
            spawnParticle(PARTICLE_TYPE_PLAYER_ROOT_ATTACK, playerX + offsetX, playerY + offsetY, PARTICLE_TYPE_NONE, false /* enemyParticle */);
        }
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
        activateSpellEffect(currentSpell, playerAngle);
        currentSpell = SPELL_NONE;
    } else if (data.type == SPELL_TYPE_REPEAT) {
        setAngle = playerAngle;
        activateSpellEffect(currentSpell, playerAngle);
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
            float angle = data.setAngle ? setAngle : playerAngle;
            activateSpellEffect(currentSpell, angle);
            spellActivationTime = data.activationTime;
            spellActivateCount++;
            if (spellActivateCount >= data.count) {
                currentSpell = SPELL_NONE;
            }
        }
    }
}