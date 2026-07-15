#include "game/player/classes/classData.h"
#include "game/systems/runeManager.h"
#include "globals/globals.h"
#include "globals/localization.h"

// Name and description strings are loaded from localization.csv into the
// name/nameJP/desc/descJP fields by initClassLocalization(), keyed off
// CLASS_LOC_IDS below (class.<id>.name and class.<id>.desc).
ClassData classData[CLASS_COUNT] = {
    [CLASS_MAGE] = {
        .classType = CLASS_MAGE,
        .baseHealth = 100,
        .baseSpeed = 12,
        .basicSpell = SPELL_FIREBALL,
        .specialSpell = SPELL_CHAIN_LIGHTNING,
        .starterRune = RUNE_TYPE_MEDITATION
    },
    [CLASS_KNIGHT] = {
        .classType = CLASS_KNIGHT,
        .baseHealth = 150,
        .baseSpeed = 10,
        .basicSpell = SPELL_SWORD,
        .specialSpell = SPELL_ROARING_SHOUT,
        .starterRune = RUNE_TYPE_MEDITATION
    },
    [CLASS_ROGUE] = {
        .classType = CLASS_ROGUE,
        .baseHealth = 120,
        .baseSpeed = 15,
        .basicSpell = SPELL_KNIFE,
        .specialSpell = SPELL_FAN_OF_KNIVES,
        .starterRune = RUNE_TYPE_WINGED_BOOTS
    },
    [CLASS_RANGER] = {
        .classType = CLASS_RANGER,
        .baseHealth = 100,
        .baseSpeed = 12,
        .basicSpell = SPELL_BOW_SHOT,
        .specialSpell = SPELL_NET_TRAP,
        .starterRune = RUNE_TYPE_WINGED_BOOTS
    }
};

static const char *CLASS_LOC_IDS[CLASS_COUNT] = {
    [CLASS_MAGE] = "mage",
    [CLASS_KNIGHT] = "knight",
    [CLASS_ROGUE] = "rogue",
    [CLASS_RANGER] = "ranger",
};

void initClassLocalization(void) {
    for (int i = 0; i < CLASS_COUNT; i++) {
        const char *id = CLASS_LOC_IDS[i];
        if (id == NULL) {
            continue;
        }
        char *nameKey = NULL;
        char *descKey = NULL;
        pd->system->formatString(&nameKey, "class.%s.name", id);
        pd->system->formatString(&descKey, "class.%s.desc", id);
        classData[i].name = getLocText(nameKey, LANGUAGE_EN);
        classData[i].nameJP = getLocText(nameKey, LANGUAGE_JP);
        classData[i].desc = getLocText(descKey, LANGUAGE_EN);
        classData[i].descJP = getLocText(descKey, LANGUAGE_JP);
        pd->system->realloc(nameKey, 0);
        pd->system->realloc(descKey, 0);
    }
}

ClassType currentClass = CLASS_KNIGHT;

ClassType getCurrentClassType(void) {
    return currentClass;
}

void setCurrentClass(ClassType type) {
    currentClass = type;
}

const ClassData* getCurrentClassData(void) {
    return &classData[currentClass];
}