#pragma once
#include "pd_api.h"

#include "game/rendering/particles.h"
#include "game/rendering/textures.h"

#define SPELL_EQUIP_MAX 2
#define SPELL_DECK_MAX 2

#define BASIC_SPELL_INDEX 0
#define SPECIAL_SPELL_INDEX 1

typedef enum Spell {
    SPELL_NONE,
    SPELL_ARCANE_BOLT,
    SPELL_LASER,
    SPELL_FIREBALL,
    SPELL_GREATER_FIREBALL,
    SPELL_FIREBALL_EXPLOSION,
    SPELL_SWORD,
    SPELL_ROARING_SHOUT,
    SPELL_KNIFE,
    SPELL_FAN_OF_KNIVES,
    SPELL_COUNT
} Spell;

typedef enum SpellType {
    SPELL_TYPE_INSTANT,
    SPELL_TYPE_REPEAT,
    SPELL_TYPE_COUNT
} SpellType;

typedef struct SpellCastData {
    Spell spell;
    float maxCooldown;
    float castTime;
    float activationTime;
    int activationCount;
    float initialCastAngle;
    Point spellPosition;
} SpellCastData;

typedef struct RepeatData {
    int repeatCount;
    float activationTime;
} RepeatData;

typedef struct SpellProjectileData {
    float speed;
    float acceleration;
    int hitCount;
    float slowTime;
    float slowAmount;
    int spreadCount;
    float spreadAngle;
    float activeTime;
    Spell onHitSpell;
    ParticleType hitParticle;
    ParticleType timeoutParticle;
    TexturePaths textureIndex;
    int frameCount;
	float frameTime;
} SpellProjectileData;

typedef struct MeleeData {
    float range;
} MeleeData;

typedef struct WeaponData {
    BitmapTablePath weaponBitmapTable;
    int drawX;
    int drawY;
    int activeOffsetX;
    int activeOffsetY;
    float frameTime;
    int activeFrame;
} WeaponData;

typedef struct SpellData {
    char* name;
    char* name_jp;
    char* description;
    char* description_jp;
    int pickupTextureIndex;
    int spellIconIndex;
    SpellType type;
    void (*cast)(Spell spell, SpellCastData *castData, float x, float y, float angle, bool initialCast);
    float cooldown;
    int damage;
    float knockbackTime;
    float knockbackAmount;
    bool useInitialCastAngle;
    float playerSpeedModifier;
    RepeatData repeatData;
    SpellProjectileData projectileData;
    MeleeData meleeData;
    WeaponData weaponData;
} SpellData;

// SpellProjectileData modifiers
void modifyProjectileSpeed(SpellData *spell, float value);
void modifyProjectileAcceleration(SpellData *spell, float value);
void modifyProjectileHitCount(SpellData *spell, float value);
void modifyProjectileSlowTime(SpellData *spell, float value);
void modifyProjectileSlowAmount(SpellData *spell, float value);
void modifyProjectileSpreadCount(SpellData *spell, float value);
void modifyProjectileSpreadAngle(SpellData *spell, float value);
void modifyProjectileActiveTime(SpellData *spell, float value);
void setProjectileOnHitSpell(SpellData *spell, float value);
void setProjectileHitParticle(SpellData *spell, float value);
void setProjectileTimeoutParticle(SpellData *spell, float value);

// SpellData modifiers
void modifySpellCooldown(SpellData *spell, float value);
void modifySpellDamage(SpellData *spell, float value);
void modifySpellKnockbackTime(SpellData *spell, float value);
void modifySpellKnockbackAmount(SpellData *spell, float value);

// RepeatData modifiers
void modifyRepeatCount(SpellData *spell, float value);
void modifyRepeatActivationTime(SpellData *spell, float value);

void initSpells(void);
void resetSpellState(void);
void recalculateRuneEffectsToSpells(void);
Spell *getSpellDeck(void);
bool spellInDeck(Spell spell);
void addSpellToDeck(Spell spell);
void swapSpells(int spellIndex1, int spellIndex2);
int getSpellPickupTextureIndex(Spell spell);
LCDBitmap* getSpellIcon(Spell spell);
Spell getDeckSpell(int index);
Spell getBasicSpell(void);
Spell getSpecialSpell(void);
float getSpellCooldownPercentage(int index);
float getPlayerSpeedModifier(void);
int getMaxUtilitySpells(void);
int getUtilitySpellCount(void);
float getUtilitySpellCharge(void);
void castUtilitySpell(void);
void castSpellDirect(Spell spell, float x, float y, float angle);
bool spellIsCastable(int index);
bool castSpell(int index);
bool castBasicSpell(void);
bool castSpecialSpell(void);
void updateSpells(float deltaTime);

extern SpellData spellData[SPELL_COUNT];