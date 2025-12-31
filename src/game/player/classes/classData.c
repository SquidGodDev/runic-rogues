#include "game/player/classes/classData.h"

const ClassData classData[CLASS_COUNT] = {
    {
        .classType = CLASS_MAGE,
        .name = "Mage",
        .desc = "",
        .nameJP = "",
        .descJP = "",
        .baseHealth = 100,
        .baseSpeed = 12,
        .basicSpell = SPELL_FIREBALL,
        .specialSpell = SPELL_LASER
    },
    {
        .classType = CLASS_WARRIOR,
        .name = "Warrior",
        .desc = "",
        .nameJP = "",
        .descJP = "",
        .baseHealth = 150,
        .baseSpeed = 10,
        .basicSpell = SPELL_SWORD,
        .specialSpell = SPELL_BASH
    },
    {
        .classType = CLASS_ROGUE,
        .name = "Rogue",
        .desc = "",
        .nameJP = "",
        .descJP = "",
        .baseHealth = 120,
        .baseSpeed = 15,
        .basicSpell = SPELL_KNIFE,
        .specialSpell = SPELL_DASH
    }
};