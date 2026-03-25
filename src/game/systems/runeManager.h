#pragma once
#include "pd_api.h"

#include "game/settings/settings.h"
#include "game/player/classes/classData.h"


typedef enum RuneType {
    RUNE_TYPE_NONE,
    RUNE_TYPE_VAMPIRISM,
    RUNE_TYPE_WINGED_BOOTS,
    RUNE_TYPE_HEALTHY,
    RUNE_TYPE_SWIFT,
    RUNE_TYPE_DAGGER,
    RUNE_TYPE_BROADSWORD,
    RUNE_TYPE_POCKET_WATCH,
    RUNE_TYPE_HOURGLASS,
    RUNE_TYPE_SLING,
    RUNE_TYPE_LONGBOW,
    RUNE_TYPE_COPPER_PAN,
    RUNE_TYPE_IRON_GAUNTLET,
    RUNE_TYPE_FROST_SHARD,
    RUNE_TYPE_ICE_LANCE,
    RUNE_TYPE_NEEDLE_AND_THREAD,
    RUNE_TYPE_HAMMER_AND_NAILS,
    RUNE_TYPE_QUARTZ_SHARD,
    RUNE_TYPE_QUARTZ_CLUSTER,
    RUNE_TYPE_ENCHANTED_BRANCH,
    RUNE_TYPE_ARCANE_STAFF,
    RUNE_TYPE_COUNT
} RuneType;

// Forward declaration (spells.h)
struct SpellData;

typedef int RuneClassFilter;
#define RUNE_CLASS_FILTER_MAGE    (1 << CLASS_MAGE)
#define RUNE_CLASS_FILTER_WARRIOR (1 << CLASS_WARRIOR)
#define RUNE_CLASS_FILTER_ROGUE   (1 << CLASS_ROGUE)

typedef enum RuneSpellTarget {
    RUNE_SPELL_TARGET_NONE,
    RUNE_SPELL_TARGET_BASIC,
    RUNE_SPELL_TARGET_SPECIAL,
    RUNE_SPELL_TARGET_BOTH
} RuneSpellTarget;

typedef struct RuneData {
    char *nameEN;
    char *nameJP;
    char *descriptionEN;
    char *descriptionJP;
    int iconIndex;
    int textureIndex;
    float primaryBaseValue;
    float primaryPerLevel;
    float secondaryBaseValue;
    float secondaryPerLevel;
    int maxLevel;
    RuneSpellTarget spellTarget;
    ModifyOperation primaryModifyOperation;
    ModifyOperation secondaryModifyOperation;
    void (*applySpellModifierPrimary)(struct SpellData *spell, float value, ModifyOperation operation);
    void (*applySpellModifierSecondary)(struct SpellData *spell, float value, ModifyOperation operation);
    RuneClassFilter classFilter;
} RuneData;

void initRunes(void);
bool runeIsOwned(RuneType type);
void addRune(RuneType type);
void setRuneLevel(RuneType type, int level);
int getRuneLevel(RuneType type);
int getRuneMaxLevel(RuneType type);
bool runeIsMaxLevel(RuneType type);
char *getRuneName(RuneType type, Language language);
char *getRuneDescription(RuneType type, Language language, int runeLevel);
int getRuneIconIndex(RuneType type);
int getRuneTextureIndex(RuneType type);
RuneClassFilter getRuneClassFilter(RuneType type);
float getRuneValue(RuneType type);
float getRuneSecondaryValue(RuneType type);
void applyRuneEffectsToSpells(void);