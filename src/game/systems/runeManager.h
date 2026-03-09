#pragma once
#include "pd_api.h"

#include "game/settings/settings.h"

#define MAX_OWNED_RUNES 14

typedef enum RuneType {
    RUNE_TYPE_NONE,
    RUNE_TYPE_VAMPIRISM,
    RUNE_TYPE_WINGED_BOOTS,
    RUNE_TYPE_HEALTHY,
    RUNE_TYPE_SWIFT,
    RUNE_TYPE_COUNT
} RuneType;

// Forward declaration (spells.h)
struct SpellData;

typedef struct RuneData {
    char *nameEN;
    char *nameJP;
    char *descriptionEN;
    char *descriptionJP;
    int iconIndex;
    int textureIndex;
    int maxLevel;
    float primaryValue;
    float secondaryValue;
    void (*applySpellModifier)(struct SpellData *spell, const struct RuneData *rune);  // NULL = no spell effect
} RuneData;

void initRunes(void);
bool runeIsOwned(RuneType type);
void addRune(RuneType type, int level);
char *getRuneName(RuneType type, Language language);
char *getRuneDescription(RuneType type, Language language);
int getRuneIconIndex(RuneType type);
int getRuneTextureIndex(RuneType type);
float getRunePrimaryValue(RuneType type);
float getRuneSecondaryValue(RuneType type);
float getRuneValue(RuneType type);
void applyRuneEffectsToSpells(void);