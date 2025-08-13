#include "game/player/playerStats.h"

#define BASE_MAX_HEALTH 100
#define BASE_MAX_MANA 10
#define BASE_MANA_REGEN_RATE 1.5f
#define BASE_UTILITY_SPELL_CHARGE_RATE 0.2f
#define BASE_MOVE_SPEED 40.0f

int getPlayerMaxHealth(void) {
    return BASE_MAX_HEALTH;
}

int getPlayerMaxMana(void) {
    return BASE_MAX_MANA;
}

float getPlayerManaRegenRate(void) {
    return BASE_MANA_REGEN_RATE;
}

float getPlayerUtilitySpellChargeRate(void) {
    return BASE_UTILITY_SPELL_CHARGE_RATE;
}

float getPlayerMoveSpeed(void) {
    return BASE_MOVE_SPEED;
}