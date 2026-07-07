#include "game/player/classes/classData.h"
#include "game/systems/runeManager.h"

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
        .specialSpell = SPELL_CHAIN_LIGHTNING,
        .starterRune = RUNE_TYPE_NONE
    },
    [CLASS_KNIGHT] = {
        .classType = CLASS_KNIGHT,
        .name = "Knight",
        .desc = "",
        .nameJP = "",
        .descJP = "",
        .baseHealth = 150,
        .baseSpeed = 10,
        .basicSpell = SPELL_SWORD,
        .specialSpell = SPELL_ROARING_SHOUT,
        .starterRune = RUNE_TYPE_NONE
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
        .specialSpell = SPELL_FAN_OF_KNIVES,
        .starterRune = RUNE_TYPE_NONE
    },
    [CLASS_RANGER] = {
        .classType = CLASS_RANGER,
        .name = "Ranger",
        .desc = "",
        .nameJP = "",
        .descJP = "",
        .baseHealth = 100,
        .baseSpeed = 12,
        .basicSpell = SPELL_BOW_SHOT,
        .specialSpell = SPELL_NET_TRAP,
        .starterRune = RUNE_TYPE_NONE
    }
};

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