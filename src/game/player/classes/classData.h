#pragma once

#include "game/player/spells/spells.h"

typedef enum ClassType {
    CLASS_MAGE,
    CLASS_WARRIOR,
    CLASS_ROGUE,
    CLASS_COUNT
} ClassType;

typedef struct ClassData {
    ClassType classType;
    const char* name;
    const char* desc;
    const char* nameJP;
    const char* descJP;
    int baseHealth;
    int baseSpeed;
    Spell basicSpell;
    Spell specialSpell;
} ClassData;

extern const ClassData classData[CLASS_COUNT];