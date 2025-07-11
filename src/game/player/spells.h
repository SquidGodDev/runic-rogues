#pragma once
#include "pd_api.h"

typedef enum Spell {
    SPELL_NONE,
    SPELL_FIREBALL,
    SPELL_TRIPLE_SHOT,
    SPELL_SHOTGUN,
    SPELL_LASER,
    SPELL_ROOT_ATTACK,
    SPELL_WAVE,
    SPELL_TRIPLE_WAVE,
    SPELL_COUNT
} Spell;

typedef enum SpellType {
    SPELL_TYPE_REPEAT,
    SPELL_TYPE_CHANNEL,
    SPELL_TYPE_INSTANT,
    SPELL_TYPE_COUNT
} SpellType;

typedef struct SpellData {
    uint8_t *pickupTexture;
    uint8_t *spellTexture;
    LCDBitmap *spellIcon;
    SpellType type;
    int damage;
    int count;
    int useInitialCastAngle;
    float speed;
    float castTime;
    float activationTime;
} SpellData;

void initSpellData(void);
uint8_t* getSpellPickupTexture(Spell spell);
LCDBitmap* getSpellIcon(Spell spell);
Spell getRandomSpell(void);
Spell heldSpellsFull(void);
Spell getHeldSpell(int index);
int getMaxHeldSpells(void);
float getManaPercentage(void);
int getMaxUtilitySpells(void);
int getUtilitySpellCount(void);
void rechargeUtilitySpell(void);
void castUtilitySpell(void);
void addHeldSpell(Spell spell);
int castHeldSpell(void);
void updateSpells(float deltaTime);