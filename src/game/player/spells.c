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
#define SPELL_DECK_MAX 30

static Spell spellDeck[SPELL_DECK_MAX];
static Stack spellsToSpawn;
static Queue heldSpellsQueue;
static Spell currentSpell;
static float spellCastTime;
static float spellActivationTime;
static int spellActivateCount;
static float initialCastAngle;

static int utilitySpellCount;

static SpellData spellData[SPELL_COUNT] = {
    [SPELL_NONE] = {
        .spellIconIndex = BITMAP_SPELL_EMPTY_ICON,
    },
    [SPELL_FIREBALL] = {
        .pickupTextureIndex = TEXTURE_SPELL_FIREBALL,
        .spellTextureIndex = TEXTURE_PROJECTILE,
        .spellIconIndex = BITMAP_SPELL_FIREBALL_ICON,
        .type = SPELL_TYPE_INSTANT,
        .castTime = 0.4f,
        .damage = 3,
        .hitCount = 1,
        .speed = 75.0f
    },
    [SPELL_TRIPLE_SHOT] = {
        .pickupTextureIndex = TEXTURE_SPELL_TRIPLE_SHOT,
        .spellTextureIndex = TEXTURE_PROJECTILE,
        .spellIconIndex = BITMAP_SPELL_TRIPLE_SHOT_ICON,
        .type = SPELL_TYPE_REPEAT,
        .castTime = 1.0f,
        .damage = 1,
        .repeatCount = 3,
        .hitCount = 1,
        .useInitialCastAngle = false,
        .speed = 150.0f,
        .activationTime = 0.3f
    },
    [SPELL_SHOTGUN] = {
        .pickupTextureIndex = TEXTURE_SPELL_SHOTGUN,
        .spellTextureIndex = TEXTURE_PROJECTILE,
        .spellIconIndex = BITMAP_SPELL_SHOTGUN_ICON,
        .type = SPELL_TYPE_INSTANT,
        .castTime = 0.4f,
        .damage = 1,
        .hitCount = 1,
        .speed = 100.0f
    },
    [SPELL_LASER] = {
        .pickupTextureIndex = TEXTURE_SPELL_LASER,
        .spellIconIndex = BITMAP_SPELL_LASER_ICON,
        .type = SPELL_TYPE_REPEAT,
        .castTime = 0.6f,
        .damage = 1,
        .activationTime = 0.1f,
        .repeatCount = 5,
        .hitCount = 3,
        .playerSpeedModifier = -0.7f,
        .useInitialCastAngle = true,
        .speed = 300.0f
    },
    [SPELL_ROOT_ATTACK] = {
        .pickupTextureIndex = TEXTURE_SPELL_ROOT_ATTACK,
        .spellIconIndex = BITMAP_SPELL_ROOT_ATTACK_ICON,
        .type = SPELL_TYPE_INSTANT,
        .castTime = 0.6f,
        .damage = 1,
        .hitCount = 1,
    },
    [SPELL_WAVE] = {
        .pickupTextureIndex = TEXTURE_SPELL_WAVE,
        .spellTextureIndex = TEXTURE_PROJECTILE,
        .spellIconIndex = BITMAP_SPELL_WAVE_ICON,
        .type = SPELL_TYPE_INSTANT,
        .castTime = 0.4f,
        .damage = 1,
        .hitCount = 1,
        .speed = 100.0f
    },
    [SPELL_TRIPLE_WAVE] = {
        .pickupTextureIndex = TEXTURE_SPELL_TRIPLE_WAVE,
        .spellTextureIndex = TEXTURE_PROJECTILE,
        .spellIconIndex = BITMAP_SPELL_TRIPLE_WAVE_ICON,
        .type = SPELL_TYPE_REPEAT,
        .castTime = 1.0f,
        .damage = 1,
        .repeatCount = 3,
        .hitCount = 1,
        .useInitialCastAngle = false,
        .speed = 150.0f,
        .activationTime = 0.3f
    },
    [SPELL_SLOW_SHOT] = {
        .pickupTextureIndex = TEXTURE_SPELL_SLOW_SHOT,
        .spellTextureIndex = TEXTURE_PROJECTILE,
        .spellIconIndex = BITMAP_SPELL_SLOW_SHOT_ICON,
        .type = SPELL_TYPE_INSTANT,
        .castTime = 0.4f,
        .damage = 0,
        .hitCount = 5,
        .slowTime = 4.0f,
        .speed = 100.0f
    },
    [SPELL_SLOW_WAVE] = {
        .pickupTextureIndex = TEXTURE_SPELL_SLOW_WAVE,
        .spellTextureIndex = TEXTURE_PROJECTILE,
        .spellIconIndex = BITMAP_SPELL_SLOW_WAVE_ICON,
        .type = SPELL_TYPE_INSTANT,
        .castTime = 0.4f,
        .damage = 0,
        .hitCount = 5,
        .slowTime = 4.0f,
        .speed = 100.0f
    }
};

void resetSpellsToSpawn(void) {
    stackClear(&spellsToSpawn);
    for (int i = 0; i < SPELL_DECK_MAX; i++) {
        if (spellDeck[i] != SPELL_NONE) {
            stackPush(&spellsToSpawn, spellDeck[i]);
        }
    }
}

void initSpells(void) {
    queueInit(&heldSpellsQueue);
    currentSpell = SPELL_NONE;
    spellCastTime = 0.0f;
    spellActivationTime = 0.0f;
    spellActivateCount = 0;

    utilitySpellCount = DASH_COUNT;

    for (int i = 0; i < SPELL_DECK_MAX; i++) {
        spellDeck[i] = SPELL_NONE;
    }

    // For testing purposes, fill the spell deck with all spells
    for (int i = 1; i < SPELL_COUNT; i++) {
        spellDeck[i] = (Spell)i;
    }
    // spellDeck[0] = SPELL_LASER;
    // spellDeck[1] = SPELL_SLOW_WAVE;

    resetSpellsToSpawn();
}

int getSpellPickupTextureIndex(Spell spell) {
    return spellData[spell].pickupTextureIndex;
}

LCDBitmap* getSpellIcon(Spell spell) {
    return BITMAPS[spellData[spell].spellIconIndex];
}

Spell getRandomSpell(void) {
    if (stackIsEmpty(&spellsToSpawn)) {
        resetSpellsToSpawn();
    }
    return stackPop(&spellsToSpawn);
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

float getPlayerSpeedModifier(void) {
    if (currentSpell == SPELL_NONE) {
        return 1.0f;
    }
    SpellData data = spellData[currentSpell];
    return 1.0f + data.playerSpeedModifier;
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
        return 1.0f - (data.castTime - spellCastTime) / data.castTime;
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

void fireSpellProjectile(Spell spell, float angle) {
    SpellData data = spellData[spell];
    float projectileX = playerX + cosf(angle + FIRE_ANGLE_OFFSET) * FIRE_POSITION_OFFSET;
    float projectileY = playerY + sinf(angle + FIRE_ANGLE_OFFSET) * FIRE_POSITION_OFFSET;
    firePlayerProjectile(projectileX, projectileY, cosf(angle), sinf(angle), data.speed, data.damage, data.hitCount, data.slowTime);
}

void activateSpellEffect(Spell spell, float angle) {
    if (spell == SPELL_FIREBALL) {
        fireSpellProjectile(spell, angle);
    } else if (spell == SPELL_TRIPLE_SHOT) {
        fireSpellProjectile(spell, angle);
    } else if (spell == SPELL_SHOTGUN) {
        float offsetAngle = PI / 8.0f;
        fireSpellProjectile(spell, angle);
        fireSpellProjectile(spell, angle + offsetAngle);
        fireSpellProjectile(spell, angle - offsetAngle);
    } else if (spell == SPELL_LASER) {
        fireSpellProjectile(spell, angle);
    } else if (spell == SPELL_ROOT_ATTACK) {
        int rootCount = 5;
        float deltaX = cosf(angle);
        float deltaY = sinf(angle);
        for (int root = 0; root < rootCount; root++) {
            float offsetX = deltaX * (root + 1) * CELL_SIZE;
            float offsetY = deltaY * (root + 1) * CELL_SIZE;
            spawnParticle(PARTICLE_TYPE_PLAYER_ROOT_ATTACK, playerX + offsetX, playerY + offsetY, PARTICLE_TYPE_NONE, false /* enemyParticle */);
        }
    } else if (spell == SPELL_WAVE) {
        float offsetAngle = PI / 16.0f;
        fireSpellProjectile(spell, angle);
        fireSpellProjectile(spell, angle + offsetAngle);
        fireSpellProjectile(spell, angle + offsetAngle * 2);
        fireSpellProjectile(spell, angle - offsetAngle);
        fireSpellProjectile(spell, angle - offsetAngle * 2);
    } else if (spell == SPELL_TRIPLE_WAVE) {
        float offsetAngle = PI / 16.0f;
        fireSpellProjectile(spell, angle);
        fireSpellProjectile(spell, angle + offsetAngle);
        fireSpellProjectile(spell, angle + offsetAngle * 2);
        fireSpellProjectile(spell, angle - offsetAngle);
        fireSpellProjectile(spell, angle - offsetAngle * 2);
    } else if (spell == SPELL_SLOW_SHOT) {
        fireSpellProjectile(spell, angle);
    } else if (spell == SPELL_SLOW_WAVE) {
        float offsetAngle = PI / 16.0f;
        fireSpellProjectile(spell, angle);
        fireSpellProjectile(spell, angle + offsetAngle);
        fireSpellProjectile(spell, angle + offsetAngle * 2);
        fireSpellProjectile(spell, angle - offsetAngle);
        fireSpellProjectile(spell, angle - offsetAngle * 2);
    }
}

void addHeldSpell(Spell spell) {
    queuePush(&heldSpellsQueue, spell);
}

int castHeldSpell(void) {
    if (queueIsEmpty(&heldSpellsQueue) || spellCastTime > 0) {
        return 0;
    }

    currentSpell = queuePop(&heldSpellsQueue);

    playSound(SFX_SHOOT);
    SpellData data = spellData[currentSpell];
    spellCastTime = data.castTime;
    if (data.type == SPELL_TYPE_INSTANT) {
        activateSpellEffect(currentSpell, playerAngle);
    } else if (data.type == SPELL_TYPE_REPEAT) {
        initialCastAngle = playerAngle;
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

    spellCastTime -= deltaTime;
    if (spellCastTime <= 0.0f) {
        spellCastTime = 0.0f;
        currentSpell = SPELL_NONE;
        return;
    }

    SpellData* data = &spellData[currentSpell];
    if (data->type == SPELL_TYPE_REPEAT) {
        spellActivationTime -= deltaTime;
        if (spellActivationTime <= 0.0f && spellActivateCount < data->repeatCount) {
            float angle = data->useInitialCastAngle ? initialCastAngle : playerAngle;
            activateSpellEffect(currentSpell, angle);
            spellActivationTime = data->activationTime;
            spellActivateCount++;
        }
    }
}