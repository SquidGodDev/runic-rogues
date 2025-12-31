#pragma once

#include "game/player/spells/spells.h"

typedef enum Upgrade {
    UPGRADE_NONE,
    UPGRADE_SPELL_DAMAGE,
    UPGRADE_ARCANE_BOLT_REPEAT,
    UPGRADE_FIREBALL_RADIUS,
    UPGRADE_RUNE_SPEED,
    UPGRADE_RUNE_HEALTH,
    UPGRADE_RUNE_REGEN,
    UPGRADE_COUNT
} Upgrade;

typedef enum UpgradeType {
    UPGRADE_TYPE_RUNE,
    UPGRADE_TYPE_BASIC_ABILITY,
    UPGRADE_TYPE_SPECIAL_ABILITY
} UpgradeType;

typedef struct UpgradeData {
    const char* name;
    const char* nameJP;
    const char* description;
    const char* descriptionJP;
    UpgradeType type;
    void (*onUpgrade)(int level);
    int maxLevel;
    Spell associatedSpell;
} UpgradeData;

extern const UpgradeData UPGRADE_DATA[UPGRADE_COUNT];
extern int playerUpgradeLevels[UPGRADE_COUNT];

void initUpgrades(void);