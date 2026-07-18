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
    RUNE_TYPE_SLINGSHOT,
    RUNE_TYPE_LONGBOW,
    RUNE_TYPE_BEAR_PAW,
    RUNE_TYPE_IRON_GAUNTLET,
    RUNE_TYPE_FROST_SHARD,
    RUNE_TYPE_ICE_LANCE,
    RUNE_TYPE_NEEDLE_AND_THREAD,
    RUNE_TYPE_HAMMER_AND_NAILS,
    RUNE_TYPE_MAGNIFYING_GLASS,
    RUNE_TYPE_QUARTZ_CLUSTER,
    RUNE_TYPE_ENCHANTED_AMULET,
    RUNE_TYPE_ARCANE_STAFF,
    RUNE_TYPE_THUNDERCLOUD,
    RUNE_TYPE_MEDITATION,
    RUNE_TYPE_THORNS,
    RUNE_TYPE_BRASS_KNUCKLES,
    RUNE_TYPE_BLUNDERBUSS,
    RUNE_TYPE_RICOCHET,
    RUNE_TYPE_BOMBER,
    RUNE_TYPE_DRAGONS_BREATH,
    RUNE_TYPE_ARMORED_SKIN,
    RUNE_TYPE_VIKING,
    RUNE_TYPE_SECOND_WIND,
    RUNE_TYPE_FLYWHEEL,
    RUNE_TYPE_COUNT
} RuneType;

// Forward declaration (spells.h)
struct SpellData;

typedef int RuneClassFilter;
#define RUNE_CLASS_FILTER_MAGE    (1 << CLASS_MAGE)
#define RUNE_CLASS_FILTER_KNIGHT  (1 << CLASS_KNIGHT)
#define RUNE_CLASS_FILTER_ROGUE   (1 << CLASS_ROGUE)
#define RUNE_CLASS_FILTER_RANGER  (1 << CLASS_RANGER)
#define RUNE_CLASS_FILTER_NONE    (1 << CLASS_COUNT)

typedef enum RuneSpellTarget {
    RUNE_SPELL_TARGET_NONE,
    RUNE_SPELL_TARGET_BASIC,
    RUNE_SPELL_TARGET_SPECIAL,
    RUNE_SPELL_TARGET_BOTH
} RuneSpellTarget;

typedef enum RuneTriggerType {
    RUNE_TRIGGER_NONE,
    RUNE_TRIGGER_TIME,
    RUNE_TRIGGER_BASIC_CAST,
    RUNE_TRIGGER_SPECIAL_CAST,
    RUNE_TRIGGER_DASH,
    RUNE_TRIGGER_DAMAGED,
    RUNE_TRIGGER_DAMAGE_ENEMY,
    RUNE_TRIGGER_KILL_ENEMY,
    RUNE_TRIGGER_EXPERIENCE,
    RUNE_TRIGGER_ROOM_CLEAR
} RuneTriggerType;

typedef struct RuneData {
    char *nameEN;
    char *nameJP;
    char *descriptionEN;
    char *descriptionJP;
    int iconIndex;
    float primaryBaseValue;
    float primaryPerLevel;
    float secondaryBaseValue;
    float secondaryPerLevel;
    float tertiaryBaseValue;
    float tertiaryPerLevel;
    int maxLevel;
    RuneSpellTarget spellTarget;
    ModifyOperation primaryModifyOperation;
    ModifyOperation secondaryModifyOperation;
    ModifyOperation tertiaryModifyOperation;
    void (*applySpellModifierPrimary)(struct SpellData *spell, float value, ModifyOperation operation);
    void (*applySpellModifierSecondary)(struct SpellData *spell, float value, ModifyOperation operation);
    void (*applySpellModifierTertiary)(struct SpellData *spell, float value, ModifyOperation operation);
    RuneClassFilter classFilter;
    RuneTriggerType triggerType;
    float triggerBaseValue;
    float triggerPerLevel;
    void (*triggerCallback)(float primaryValue, float secondaryValue);
} RuneData;

void initRunes(void);
// Loads rune names/descriptions from localization.csv into RUNE_DATA. Call once
// at boot after initLocalization().
void initRuneLocalization(void);
bool runeIsOwned(RuneType type);
void addRune(RuneType type);
void setRuneLevel(RuneType type, int level);
int getRuneLevel(RuneType type);
int getRuneMaxLevel(RuneType type);
bool runeIsMaxLevel(RuneType type);
char *getRuneName(RuneType type, Language language);
char *getRuneDescription(RuneType type, Language language, int runeLevel);
int getRuneIconIndex(RuneType type);
RuneClassFilter getRuneClassFilter(RuneType type);
float getRuneValue(RuneType type);
float getRuneSecondaryValue(RuneType type);
void applyRuneEffectsToSpells(void);
void signalRuneTrigger(RuneTriggerType type, float amount);