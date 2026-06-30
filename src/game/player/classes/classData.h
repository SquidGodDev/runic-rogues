#pragma once

#include "game/player/spells/spells.h"
#include "game/rendering/textures.h"

typedef enum ClassType {
    CLASS_MAGE,
    CLASS_KNIGHT,
    CLASS_ROGUE,
    CLASS_RANGER,
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

ClassType getCurrentClassType(void);
const ClassData* getCurrentClassData(void);
void setCurrentClass(ClassType type);
