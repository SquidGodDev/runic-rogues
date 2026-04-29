#include "game/player/classes/classData.h"

const ClassData classData[CLASS_COUNT] = {
    [CLASS_MAGE] = {
        .classType = CLASS_MAGE,
        .name = "Mage",
        .desc = "",
        .nameJP = "",
        .descJP = "",
        .baseHealth = 100,
        .baseSpeed = 12,
        .basicSpell = SPELL_FIREBALL,
        .specialSpell = SPELL_CHAIN_LIGHTNING
    },
    [CLASS_WARRIOR] = {
        .classType = CLASS_WARRIOR,
        .name = "Warrior",
        .desc = "",
        .nameJP = "",
        .descJP = "",
        .baseHealth = 150,
        .baseSpeed = 10,
        .basicSpell = SPELL_SWORD,
        .specialSpell = SPELL_ROARING_SHOUT
    },
    [CLASS_ROGUE] = {
        .classType = CLASS_ROGUE,
        .name = "Rogue",
        .desc = "",
        .nameJP = "",
        .descJP = "",
        .baseHealth = 120,
        .baseSpeed = 15,
        .basicSpell = SPELL_KNIFE,
        .specialSpell = SPELL_FAN_OF_KNIVES
    }
};

ClassType currentClass = CLASS_WARRIOR;

ClassType getCurrentClassType(void) {
    return currentClass;
}

void setCurrentClass(ClassType type) {
    currentClass = type;
}

const ClassData* getCurrentClassData(void) {
    return &classData[currentClass];
}