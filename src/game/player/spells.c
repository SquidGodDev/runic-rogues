#include "globals.h"
#include "helpers.h"

#include "game/entities/projectiles.h"
#include "game/UI/UI.h"
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

static Spell spellDeck[SPELL_DECK_MAX] = { 0 };
static Stack spellsToSpawn;
static Queue heldSpellsQueue;
static Spell currentSpell;
static float spellCastTime;
static float spellActivationTime;
static int spellActivateCount;
static float initialCastAngle;

static int utilitySpellCount;

void resetSpellsToSpawn(void) {
    stackClear(&spellsToSpawn);
    for (int i = 0; i < SPELL_DECK_MAX; i++) {
        if (spellDeck[i] != SPELL_NONE) {
            stackPush(&spellsToSpawn, spellDeck[i]);
        }
    }
}

void initDeck(void) {
    for (int i = 0; i < SPELL_DECK_MAX; i++) {
        spellDeck[i] = SPELL_NONE;
    }
    // For testing purposes, fill the spell deck with all spells
    // for (int i = 1; i < SPELL_COUNT; i++) {
    //     spellDeck[i] = (Spell)i;
    // }
    spellDeck[0] = SPELL_FIREBALL;
    spellDeck[1] = SPELL_SHOTGUN;
    spellDeck[2] = SPELL_TRIPLE_SHOT;
    // spellDeck[0] = SPELL_EXPLOSIVE_SHOT;
}

void initSpells(void) {
    queueInit(&heldSpellsQueue);
    currentSpell = SPELL_NONE;
    spellCastTime = 0.0f;
    spellActivationTime = 0.0f;
    spellActivateCount = 0;

    utilitySpellCount = DASH_COUNT;

    resetSpellsToSpawn();
}

Spell *getSpellDeck(void) {
    return spellDeck;
}

bool spellInDeck(Spell spell) {
    for (int i = 0; i < SPELL_DECK_MAX; i++) {
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
    for (int i = 0; i < SPELL_DECK_MAX; i++) {
        if (spellDeck[i] == SPELL_NONE) {
            spellDeck[i] = spell;
            break;
        }
    }
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
    firePlayerProjectile(projectileX, projectileY, cosf(angle), sinf(angle), data.damage, data.projectileData);
}

void activateSpellEffect(Spell spell, float angle) {
    if (spell == SPELL_FIREBALL) {
        fireSpellProjectile(spell, angle);
    } else if (spell == SPELL_TRIPLE_SHOT) {
        fireSpellProjectile(spell, angle);
    } else if (spell == SPELL_SHOTGUN) {
        float offsetAngle = PI / 16.0f;
        fireSpellProjectile(spell, angle);
        fireSpellProjectile(spell, angle + offsetAngle);
        fireSpellProjectile(spell, angle - offsetAngle);
    } else if (spell == SPELL_LASER) {
        fireSpellProjectile(spell, angle);
    } else if (spell == SPELL_ROOT_SURGE) {
        float offsetX = cosf(angle) * (spellActivateCount % 3 + 1) * CELL_SIZE;
        float offsetY = sinf(angle) * (spellActivateCount % 3 + 1) * CELL_SIZE;
        spawnParticle(PARTICLE_TYPE_PLAYER_ROOT_ATTACK, playerX + offsetX, playerY + offsetY, PARTICLE_TYPE_NONE, false /* enemyParticle */);
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
    } else if (spell == SPELL_CONE_OF_ICE) {
        float offsetAngle = PI / 32.0f;
        fireSpellProjectile(spell, angle);
        fireSpellProjectile(spell, angle + offsetAngle);
        fireSpellProjectile(spell, angle - offsetAngle);
    } else if (spell == SPELL_ARC_LIGHTNING) {
        float lightningX = playerX + cosf(angle) * FIRE_POSITION_OFFSET;
        float lightningY = playerY + sinf(angle) * FIRE_POSITION_OFFSET;
        int damageRadius = CELL_SIZE;
        damageEnemiesInArea(lightningX, lightningY, damageRadius * damageRadius, spellData[spell].damage);
    } else if (spell == SPELL_EXPLOSIVE_SHOT) {
        fireSpellProjectile(spell, angle);
    } else if (spell == SPELL_BOMBER) {
        float offsetX = cosf(angle) * CELL_SIZE / 2;
        float offsetY = sinf(angle) * CELL_SIZE / 2;
        spawnParticle(PARTICLE_TYPE_BOMB, playerX + offsetX, playerY + offsetY, PARTICLE_TYPE_NONE, false /* enemyParticle */);
    }

    spellActivateCount++;
}

void addHeldSpell(Spell spell) {
    queuePush(&heldSpellsQueue, spell);
}

int castHeldSpell(void) {
    if (queueIsEmpty(&heldSpellsQueue) || spellCastTime > 0) {
        return 0;
    }

    spellActivateCount = 0;
    currentSpell = queuePop(&heldSpellsQueue);

    playSound(SFX_SHOOT);
    SpellData data = spellData[currentSpell];
    spellCastTime = data.castTime;
    if (data.type == SPELL_TYPE_INSTANT) {
        activateSpellEffect(currentSpell, playerAngle);
    } else if (data.type == SPELL_TYPE_REPEAT) {
        initialCastAngle = playerAngle;
        activateSpellEffect(currentSpell, playerAngle);
        spellActivationTime = data.repeatData.activationTime;
    }

    if (currentSpell == SPELL_ARC_LIGHTNING) {
        setSpellOverlay(BITMAP_TABLE_ARC_LIGHTNING_OVERLAY, 0.2f /* frameTime */, 106 /* x */, 52 /* y */);
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
        clearSpellOverlay();
        return;
    }

    SpellData* data = &spellData[currentSpell];
    if (data->type == SPELL_TYPE_REPEAT) {
        spellActivationTime -= deltaTime;
        if (spellActivationTime <= 0.0f && spellActivateCount < data->repeatData.repeatCount) {
            float angle = data->useInitialCastAngle ? initialCastAngle : playerAngle;
            activateSpellEffect(currentSpell, angle);
            spellActivationTime = data->repeatData.activationTime;
        }
    }
}