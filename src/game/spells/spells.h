#pragma once
#include "pd_api.h"

#include "game/rendering/particles.h"
#include "game/rendering/textures.h"
#include "game/spells/spellData.h"

#define SPELL_EQUIP_MAX 5
#define SPELL_DECK_MAX 11

void initDeck(void);
void initSpells(void);
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
bool hasActiveChanneledSpell(void);
void clearAllChannelledSpells(void);
bool castSpell(int index);
void updateSpells(float deltaTime);