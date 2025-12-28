#pragma once

#include "game/rendering/particles.h"

typedef enum SpellType {
    SPELL_TYPE_INSTANT,
    SPELL_TYPE_REPEAT,
    SPELL_TYPE_CHANNEL,
    SPELL_TYPE_COUNT
} SpellType;

typedef struct RepeatData {
    float castTime;
    int repeatCount;
    float activationTime;
} RepeatData;

typedef struct SpellProjectileData {
    int hitCount;
    float slowTime;
    float slowAmount;
    float speed;
    float activeTime;
    ParticleType hitParticle;
    ParticleType timeoutParticle;
} SpellProjectileData;

typedef struct SpellData {
    char* name;
    char* name_jp;
    char* description;
    char* description_jp;
    int pickupTextureIndex;
    int spellTextureIndex;
    int spellIconIndex;
    SpellType type;
    float cooldown;
    float manaCost;
    int damage;
    float knockbackTime;
    float knockbackAmount;
    bool useInitialCastAngle;
    float playerSpeedModifier;
    RepeatData repeatData;
    SpellProjectileData projectileData;
} SpellData;

typedef enum Spell {
    SPELL_NONE,
    SPELL_FIREBALL,
    SPELL_TRIPLE_SHOT,
    SPELL_SHOTGUN,
    SPELL_LASER,
    SPELL_ROOT_SURGE,
    SPELL_WAVE,
    SPELL_TRIPLE_WAVE,
    SPELL_CONE_OF_ICE,
    SPELL_ARC_LIGHTNING,
    SPELL_EXPLOSIVE_SHOT,
    SPELL_BOMBER,
    SPELL_TELEPORT,
    SPELL_BLACK_HOLE,
    SPELL_COUNT
} Spell;

extern SpellData spellData[SPELL_COUNT];