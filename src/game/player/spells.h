#pragma once
#include "pd_api.h"

typedef enum Spell {
    SPELL_NONE,
    SPELL_FIREBALL,
    SPELL_TRIPLE_SHOT,
    SPELL_SHOTGUN,
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
    SpellType type;
    int damage;
    int count;
    float speed;
    float activeTime;
    float activationTime;
} SpellData;

void initSpellData(void);
uint8_t* getSpellPickupTexture(Spell spell);
void setCurrentSpell(Spell spell);
void castCurrentSpell(void);
void updateSpells(float deltaTime);