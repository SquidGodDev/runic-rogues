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
    SPELL_FIREBALL,
    SPELL_CHAIN_LIGHTNING,
    SPELL_CHAIN_LIGHTNING_LINK,
    SPELL_LASER,
    SPELL_SWORD,
    SPELL_ROARING_SHOUT,
    SPELL_KNIFE,
    SPELL_FAN_OF_KNIVES,
    SPELL_BOW_SHOT,
    SPELL_TURRET,
    SPELL_NET_TRAP,
    SPELL_COUNT
} Spell;

typedef struct SpellCastData {
    Spell spell;
    float maxCooldown;
    float castTime;
    float activationTime;
    int activationCount;
    float initialCastAngle;
    int attackId;
    int chargeLevel;
    Point spellPosition;
} SpellCastData;

#define CHARGE_LEVEL_MAX 3

// Per-level bonuses applied additively on top of the spell's (rune-modified) base stats
typedef struct ChargeLevelData {
    int damageBonus;
    float speedBonus;
    int pierceBonus;
    float playerSpeedModifier; // negative slows the player while charging at this level
} ChargeLevelData;

typedef struct ChargeData {
    int levelCount;          // 0 means the spell does not charge
    float timePerLevel;      // seconds held before advancing to the next level
    bool jiggleAtMax;        // continuous weapon jiggle once fully charged
    ChargeLevelData levels[CHARGE_LEVEL_MAX];
} ChargeData;

typedef struct RepeatData {
    int repeatCount;
    float activationTime;
} RepeatData;

typedef struct SpellProjectileData {
    float speed;
    float acceleration;
    int pierceCount;
    float slowTime;
    float slowAmount;
    float knockbackTime;
    float knockbackAmount;
    bool knockbackFromProjectile;
    float knockbackRadius; // knockback area radius in cells; 0 = only the hit enemy
    int spreadCount;
    float spreadAngle;
    float activeTime;
    float primaryData;
    float secondaryData;
    Spell onHitSpell;
    ParticleType hitParticle;
    ParticleType timeoutParticle;
    TexturePaths textureIndex;
    int frameCount;
	float frameTime;
    bool holdLastFrame; // play the frame animation once, then hold the final frame instead of looping
} SpellProjectileData;

typedef struct MeleeData {
    float range;
    float slowTime;
    float slowAmount;
} MeleeData;

typedef enum TurretFireMode {
    TURRET_FIRE_RING,     // fire a full ring of projectiles in all directions
    TURRET_FIRE_NEAREST,  // aim a spread at the nearest enemy
} TurretFireMode;

typedef struct TurretData {
    TurretFireMode fireMode;
    float lifetime;              // seconds the turret stays active
    float fireInterval;          // seconds between volleys
    float range;                 // detection range for TURRET_FIRE_NEAREST
    int ringProjectileCount;     // projectiles per volley in ring mode, spread evenly around a full circle
    int targetedProjectileCount; // projectiles per volley in nearest mode (center shot plus side shots)
    float targetedSpreadAngle;   // radians between adjacent shots in nearest mode
} TurretData;

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
    int spellIconIndex;
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
    ChargeData chargeData;
    TurretData turretData;
} SpellData;

typedef enum ModifyOperation {
    MODIFY_OP_ADD,
    MODIFY_OP_MULTIPLY,
    MODIFY_OP_REPLACE_IF_ZERO,
} ModifyOperation;

// SpellProjectileData modifiers
void modifyProjectileSpeed(SpellData *spell, float value, ModifyOperation operation);
void modifyProjectileAcceleration(SpellData *spell, float value, ModifyOperation operation);
void modifyProjectilePierceCount(SpellData *spell, float value, ModifyOperation operation);
void modifySlowTime(SpellData *spell, float value, ModifyOperation operation);
void modifySlowAmount(SpellData *spell, float value, ModifyOperation operation);
void modifyProjectileSpreadCount(SpellData *spell, float value, ModifyOperation operation);
void modifyProjectileSpreadAngle(SpellData *spell, float value, ModifyOperation operation);
void modifyProjectileActiveTime(SpellData *spell, float value, ModifyOperation operation);
void setProjectileOnHitSpell(SpellData *spell, float value, ModifyOperation operation);
void setProjectileHitParticle(SpellData *spell, float value, ModifyOperation operation);
void setProjectileTimeoutParticle(SpellData *spell, float value, ModifyOperation operation);

// SpellData modifiers
void modifySpellCooldown(SpellData *spell, float value, ModifyOperation operation);
void modifySpellDamage(SpellData *spell, float value, ModifyOperation operation);
void modifySpellKnockbackTime(SpellData *spell, float value, ModifyOperation operation);
void modifySpellKnockbackAmount(SpellData *spell, float value, ModifyOperation operation);

// RepeatData modifiers
void modifyRepeatCount(SpellData *spell, float value, ModifyOperation operation);
void modifyRepeatActivationTime(SpellData *spell, float value, ModifyOperation operation);

void initSpells(void);
void resetSpellState(void);
void recalculateRuneEffectsToSpells(void);
Spell *getSpellDeck(void);
bool spellInDeck(Spell spell);
void addSpellToDeck(Spell spell);
void swapSpells(int spellIndex1, int spellIndex2);
LCDBitmap* getSpellIcon(Spell spell);
Spell getDeckSpell(int index);
Spell getBasicSpell(void);
Spell getSpecialSpell(void);
float getSpellCooldownPercentage(int index);
float getPlayerSpeedModifier(void);
const ChargeData *getBasicSpellChargeData(void);
const ChargeData *getSpecialSpellChargeData(void);
bool spellIsCharged(Spell spell);
bool castBasicSpellCharged(int chargeLevel);
bool castSpecialSpellCharged(int chargeLevel);
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