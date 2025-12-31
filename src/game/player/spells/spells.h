#pragma once
#include "pd_api.h"

#include "game/rendering/particles.h"
#include "game/rendering/textures.h"

#define SPELL_EQUIP_MAX 2
#define SPELL_DECK_MAX 2

typedef enum Spell {
    SPELL_NONE,
    SPELL_ARCANE_BOLT,
    SPELL_LASER,
    SPELL_FIREBALL,
    SPELL_SWORD,
    SPELL_BASH,
    SPELL_KNIFE,
    SPELL_DASH,
    SPELL_COUNT
} Spell;

typedef enum SpellType {
    SPELL_TYPE_INSTANT,
    SPELL_TYPE_REPEAT,
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

typedef struct SpellCastData {
    Spell spell;
    float maxCooldown;
    float castTime;
    float activationTime;
    int activationCount;
    float initialCastAngle;
    Point spellPosition;
} SpellCastData;

typedef struct SpellData {
    char* name;
    char* name_jp;
    char* description;
    char* description_jp;
    int pickupTextureIndex;
    int spellTextureIndex;
    int spellIconIndex;
    SpellType type;
    void (*cast)(Spell spell, SpellCastData *castData, float angle, bool initialCast);
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

void initSpells(void);
void resetSpellState(void);
Spell *getSpellDeck(void);
bool spellInDeck(Spell spell);
void addSpellToDeck(Spell spell);
void swapSpells(int spellIndex1, int spellIndex2);
int getSpellPickupTextureIndex(Spell spell);
LCDBitmap* getSpellIcon(Spell spell);
Spell getDeckSpell(int index);
int getMaxHeldSpells(void);
float getSpellCooldownPercentage(int index);
float getPlayerSpeedModifier(void);
int getMaxUtilitySpells(void);
int getUtilitySpellCount(void);
float getUtilitySpellCharge(void);
void castUtilitySpell(void);
bool castSpell(int index);
void updateSpells(float deltaTime);

extern SpellData spellData[SPELL_COUNT];