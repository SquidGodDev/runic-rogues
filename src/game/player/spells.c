#include "globals.h"
#include "helpers.h"

#include "game/entities/projectiles.h"
#include "game/rendering/textures.h"
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
        .damage = 4,
        .projectileData = (SpellProjectileData) {
            .speed = 75.0f
        }
    },
    [SPELL_TRIPLE_SHOT] = {
        .pickupTextureIndex = TEXTURE_SPELL_TRIPLE_SHOT,
        .spellTextureIndex = TEXTURE_PROJECTILE,
        .spellIconIndex = BITMAP_SPELL_TRIPLE_SHOT_ICON,
        .type = SPELL_TYPE_REPEAT,
        .castTime = 1.0f,
        .damage = 1,
        .repeatData = (RepeatData){
            .repeatCount = 3,
            .activationTime = 0.3f
        },
        .projectileData = (SpellProjectileData){
            .speed = 150.0f,
        }
    },
    [SPELL_SHOTGUN] = {
        .pickupTextureIndex = TEXTURE_SPELL_SHOTGUN,
        .spellTextureIndex = TEXTURE_PROJECTILE,
        .spellIconIndex = BITMAP_SPELL_SHOTGUN_ICON,
        .type = SPELL_TYPE_INSTANT,
        .castTime = 0.4f,
        .damage = 1,
        .projectileData = (SpellProjectileData){
            .speed = 100.0f
        }
    },
    [SPELL_LASER] = {
        .pickupTextureIndex = TEXTURE_SPELL_LASER,
        .spellIconIndex = BITMAP_SPELL_LASER_ICON,
        .type = SPELL_TYPE_REPEAT,
        .castTime = 0.6f,
        .damage = 1,
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
    [SPELL_ROOT_ATTACK] = {
        .pickupTextureIndex = TEXTURE_SPELL_ROOT_ATTACK,
        .spellIconIndex = BITMAP_SPELL_ROOT_ATTACK_ICON,
        .type = SPELL_TYPE_REPEAT,
        .castTime = 1.0f,
        .damage = 4,
        .playerSpeedModifier = -0.5f,
        .useInitialCastAngle = true,
        .repeatData = (RepeatData){
            .activationTime = 0.2f,
            .repeatCount = 5
        }
    },
    [SPELL_WAVE] = {
        .pickupTextureIndex = TEXTURE_SPELL_WAVE,
        .spellTextureIndex = TEXTURE_PROJECTILE,
        .spellIconIndex = BITMAP_SPELL_WAVE_ICON,
        .type = SPELL_TYPE_INSTANT,
        .castTime = 0.4f,
        .damage = 1,
        .projectileData = (SpellProjectileData) {
            .speed = 100.0f
        }
    },
    [SPELL_TRIPLE_WAVE] = {
        .pickupTextureIndex = TEXTURE_SPELL_TRIPLE_WAVE,
        .spellTextureIndex = TEXTURE_PROJECTILE,
        .spellIconIndex = BITMAP_SPELL_TRIPLE_WAVE_ICON,
        .type = SPELL_TYPE_REPEAT,
        .castTime = 1.0f,
        .damage = 1,
        .useInitialCastAngle = false,
        .repeatData = (RepeatData){
            .activationTime = 0.3f,
            .repeatCount = 3
        },
        .projectileData = (SpellProjectileData) {
            .speed = 150.0f
        }
    },
    [SPELL_CONE_OF_ICE] = {
        .pickupTextureIndex = TEXTURE_SPELL_CONE_OF_ICE,
        .spellTextureIndex = TEXTURE_PROJECTILE,
        .spellIconIndex = BITMAP_SPELL_CONE_OF_ICE_ICON,
        .type = SPELL_TYPE_INSTANT,
        .castTime = 0.4f,
        .damage = 1,
        .useInitialCastAngle = false,
        .projectileData = (SpellProjectileData) {
            .hitCount = 5,
            .slowTime = 2.0f,
            .slowAmount = 0.8f,
            .speed = 100.0f
        }
    },
    [SPELL_ARC_LIGHTNING] = {
        .pickupTextureIndex = TEXTURE_SPELL_ARC_LIGHTNING,
        .spellIconIndex = BITMAP_SPELL_ARC_LIGHTNING_ICON,
        .type = SPELL_TYPE_REPEAT,
        .castTime = 1.5f,
        .damage = 3,
        .repeatData = (RepeatData){
            .activationTime = 0.3f,
            .repeatCount = 5
        }
    },
    [SPELL_EXPLOSIVE_SHOT] = {
        .pickupTextureIndex = TEXTURE_SPELL_EXPLOSIVE_SHOT,
        .spellTextureIndex = TEXTURE_PROJECTILE,
        .spellIconIndex = BITMAP_SPELL_EXPLOSIVE_SHOT_ICON,
        .type = SPELL_TYPE_INSTANT,
        .castTime = 0.4f,
        .damage = 2,
        .projectileData = (SpellProjectileData) {
            .speed = 70.0f,
            .hitParticle = PARTICLE_TYPE_BOMB
        }
    },
    [SPELL_BOMBER] = {
        .pickupTextureIndex = TEXTURE_SPELL_BOMBER,
        .spellIconIndex = BITMAP_SPELL_BOMBER_ICON,
        .type = SPELL_TYPE_INSTANT,
        .castTime = 0.3f,
    },
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
    // spellDeck[0] = SPELL_BOMBER;
    // spellDeck[1] = SPELL_EXPLOSIVE_SHOT;

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
    } else if (spell == SPELL_ROOT_ATTACK) {
        float offsetX = cosf(angle) * (spellActivateCount + 1) * CELL_SIZE;
        float offsetY = sinf(angle) * (spellActivateCount + 1) * CELL_SIZE;
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