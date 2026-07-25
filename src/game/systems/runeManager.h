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
    RUNE_TYPE_GLASS_DAGGER,
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
    RUNE_TYPE_AFTERIMAGE,
    RUNE_TYPE_FLYWHEEL,
    RUNE_TYPE_CRESCENDO,
    RUNE_TYPE_FOCUS,
    RUNE_TYPE_ROOTWALKER,
    RUNE_TYPE_RAPIER,
    RUNE_TYPE_BULWARK,
    RUNE_TYPE_LIGHTNING_ROD,
    RUNE_TYPE_HEALING_POTION,
    RUNE_TYPE_STONE_GOLEM,
    RUNE_TYPE_ADRENALINE,
    RUNE_TYPE_BLACK_HOLE,
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

typedef enum RuneRarity {
    RUNE_RARITY_COMMON,
    RUNE_RARITY_RARE
} RuneRarity;

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
    RUNE_TRIGGER_ROOM_CLEAR,
    RUNE_TRIGGER_ROOM_ENTER
} RuneTriggerType;

typedef struct RuneData {
    char *nameEN;
    char *nameJP;
    char *descriptionEN;
    char *descriptionJP;
    int iconIndex;
    int hudIconIndex;
    float primaryBaseValue;
    float primaryPerLevel;
    float secondaryBaseValue;
    float secondaryPerLevel;
    float tertiaryBaseValue;
    float tertiaryPerLevel;
    int maxLevel;
    RuneRarity rarity;
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
    void (*triggerCallback)(float primaryValue, float secondaryValue, float x, float y);
} RuneData;

void initRunes(void);
// Loads rune names/descriptions from localization.csv into RUNE_DATA. Call once
// at boot after initLocalization().
void initRuneLocalization(void);
bool runeIsOwned(RuneType type);
void addRune(RuneType type);
void setRuneLevel(RuneType type, int level);
// Runes in the order they were first obtained this run.
int getObtainedRuneCount(void);
RuneType getObtainedRune(int index);
int getRuneLevel(RuneType type);
int getRuneMaxLevel(RuneType type);
bool runeIsMaxLevel(RuneType type);
char *getRuneName(RuneType type, Language language);
char *getRuneDescription(RuneType type, Language language, int runeLevel);
int getRuneIconIndex(RuneType type);
int getRuneHudIconIndex(RuneType type);
RuneTriggerType getRuneTriggerType(RuneType type);
// Progress (0..1) of a RUNE_TRIGGER_TIME rune toward its next trigger.
float getRuneTriggerProgress(RuneType type);
RuneClassFilter getRuneClassFilter(RuneType type);
RuneRarity getRuneRarity(RuneType type);
float getRuneValue(RuneType type);
float getRuneSecondaryValue(RuneType type);
void applyRuneEffectsToSpells(void);
void signalRuneTrigger(RuneTriggerType type, float amount);
void signalRuneTriggerAtPosition(RuneTriggerType type, float amount, float x, float y);

// Per-frame update for runes whose effect ramps with movement/combat state
// (Crescendo, Rootwalker, Rapier). Call once per gameplay frame.
void updateRuneEffects(float deltaTime);
// Crescendo: ramp progress (0..1) and the resulting move-speed multiplier.
float getCrescendoProgress(void);
float getCrescendoSpeedMultiplier(void);
// Rapier: ramp progress (0..1) and the resulting move-speed multiplier.
float getRapierProgress(void);
float getRapierSpeedMultiplier(void);

// Bulwark: shield is available when owned and not yet consumed this room.
bool bulwarkIsAvailable(void);
void consumeBulwark(void);
// Adrenaline: active while owned and below 33% health.
bool adrenalineIsActive(void);
// Black Hole: only one is active at a time; cleared with the particles it drives.
void clearBlackHole(void);