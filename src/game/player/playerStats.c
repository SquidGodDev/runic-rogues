#include "game/player/playerStats.h"
#include "game/systems/runeManager.h"

#define BASE_MAX_HEALTH 100
#define BASE_HEALTH_REGEN_RATE 0.0f
#define BASE_UTILITY_SPELL_COUNT 3
#define BASE_UTILITY_SPELL_CHARGE_RATE 0.2f
#define BASE_MOVE_SPEED 40.0f

int getPlayerMaxHealth(void) {
    int maxHealth = BASE_MAX_HEALTH;
    if (runeIsOwned(RUNE_TYPE_HEALTHY)) {
        maxHealth += getRuneValue(RUNE_TYPE_HEALTHY);
    }
    if (runeIsOwned(RUNE_TYPE_GLASS_DAGGER)) {
        maxHealth -= getRuneSecondaryValue(RUNE_TYPE_GLASS_DAGGER);
    }
    if (runeIsOwned(RUNE_TYPE_BROADSWORD)) {
        maxHealth -= getRuneSecondaryValue(RUNE_TYPE_BROADSWORD);
    }
    return maxHealth;
}

float getPlayerHealthRegenRate(void) {
    return BASE_HEALTH_REGEN_RATE;
}

int getPlayerUtilitySpellCount(void) {
    int utilitySpellCount = BASE_UTILITY_SPELL_COUNT;
    if (runeIsOwned(RUNE_TYPE_WINGED_BOOTS)) {
        utilitySpellCount += getRuneSecondaryValue(RUNE_TYPE_WINGED_BOOTS);
    }
    return utilitySpellCount;
}

float getPlayerUtilitySpellChargeRate(void) {
    float chargeRate = BASE_UTILITY_SPELL_CHARGE_RATE;
    if (runeIsOwned(RUNE_TYPE_WINGED_BOOTS)) {
        chargeRate *= (1.0f + getRuneValue(RUNE_TYPE_WINGED_BOOTS));
    }
    return chargeRate;
}

float getPlayerMoveSpeed(void) {
    float moveSpeed = BASE_MOVE_SPEED;
    if (runeIsOwned(RUNE_TYPE_SWIFT)) {
        moveSpeed *= (1.0f + getRuneValue(RUNE_TYPE_SWIFT));
    }
    if (runeIsOwned(RUNE_TYPE_RAPIER)) {
        moveSpeed *= getRapierSpeedMultiplier();
    }
    if (runeIsOwned(RUNE_TYPE_CRESCENDO)) {
        moveSpeed *= getCrescendoSpeedMultiplier();
    }
    return moveSpeed;
}