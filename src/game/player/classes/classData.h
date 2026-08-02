#pragma once

#include <stdbool.h>
#include "game/player/spells/spells.h"
#include "game/rendering/textures.h"

// Ordered to match the class-select screen and the unlock chain: each class is
// unlocked by winning a run with the one before it (Mage is always available).
typedef enum ClassType {
    CLASS_MAGE,
    CLASS_ROGUE,
    CLASS_KNIGHT,
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
    int starterRune;
} ClassData;

extern ClassData classData[CLASS_COUNT];

// Loads class names/descriptions from localization.csv into classData. Call once
// at boot after initLocalization().
void initClassLocalization(void);
ClassType getCurrentClassType(void);
const ClassData* getCurrentClassData(void);
void setCurrentClass(ClassType type);

// Class unlock progression. Mage is always available; each subsequent class in
// select order unlocks by winning a run with the class before it. In debug mode
// every class is unlocked.
bool isClassUnlocked(ClassType classType);
// Stored unlock flag ignoring the debug override, used when writing the save.
bool getClassUnlockedStored(ClassType classType);
// Sets the stored unlock flag. Ignores Mage (always unlocked) and out-of-range
// indices. Called on run win and when loading the save.
void setClassUnlocked(ClassType classType, bool unlocked);
