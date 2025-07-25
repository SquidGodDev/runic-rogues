#pragma once
#include "pd_api.h"

#include "game/rendering/particles.h"
#include "game/rendering/textures.h"

#define SPELL_DECK_MAX 5

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
    SPELL_COUNT
} Spell;

typedef enum SpellType {
    SPELL_TYPE_INSTANT,
    SPELL_TYPE_REPEAT,
    SPELL_TYPE_CHANNEL,
    SPELL_TYPE_COUNT
} SpellType;

typedef struct RepeatData {
    int repeatCount;
    float activationTime;
} RepeatData;

typedef struct SpellProjectileData {
    int hitCount;
    float slowTime;
    float slowAmount;
    float speed;
    ParticleType hitParticle;
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
    float castTime;
    int damage;
    bool useInitialCastAngle;
    float playerSpeedModifier;
    RepeatData repeatData;
    SpellProjectileData projectileData;
} SpellData;

void initDeck(void);
void initSpells(void);
Spell *getSpellDeck(void);
bool spellInDeck(Spell spell);
void addSpellToDeck(Spell spell);
int getSpellPickupTextureIndex(Spell spell);
LCDBitmap* getSpellIcon(Spell spell);
Spell getRandomSpell(void);
Spell heldSpellsFull(void);
Spell getHeldSpell(int index);
int getMaxHeldSpells(void);
float getSpellCooldownPercentage(int index);
float getPlayerSpeedModifier(void);
float getManaPercentage(void);
int getMaxUtilitySpells(void);
int getUtilitySpellCount(void);
void rechargeUtilitySpell(void);
void castUtilitySpell(void);
void addHeldSpell(Spell spell);
int castHeldSpell(void);
int castSpell(int index);
void updateSpells(float deltaTime);

extern SpellData spellData[SPELL_COUNT];