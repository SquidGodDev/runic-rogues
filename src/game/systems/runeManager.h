#pragma once
#include "pd_api.h"

#include "game/settings/settings.h"

typedef enum RuneType {
    RUNE_TYPE_NONE,
    RUNE_TYPE_VAMPIRISM,
    RUNE_TYPE_WINGED_BOOTS,
    RUNE_TYPE_HEALTHY,
    RUNE_TYPE_SWIFT,
    RUNE_TYPE_THORNS,
    RUNE_TYPE_COUNT
} RuneType;

void initRunes(void);
bool runeIsOwned(RuneType type);
void addRune(RuneType type);
char *getRuneName(RuneType type, Language language);
char *getRuneDescription(RuneType type, Language language);
int getRuneIconIndex(RuneType type);
int getRuneTextureIndex(RuneType type);
float getRunePrimaryValue(RuneType type);
float getRuneSecondaryValue(RuneType type);