#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "helpers.h"
#include "game/systems/shopManager.h"
#include "game/settings/settings.h"
#include "game/rendering/textures.h"
#include "game/player/spells.h"

#define SHOP_OPTION_COUNT 3

Spell spellOptions[SHOP_OPTION_COUNT];
Stack spellOptionsStack;

int populateShopOptions(lua_State* L) {
    stackInit(&spellOptionsStack);
    for (int i = 1; i < SPELL_COUNT; i++) {
        if (spellInDeck((Spell)i)) {
            continue;
        }
        stackPush(&spellOptionsStack, i);
    }
    stackShuffle(&spellOptionsStack);

    return 0;
}

int getSpellOption(lua_State* L) {
    int option = pd->lua->getArgInt(1);
    Spell spell = SPELL_FIREBALL; // Default spell if no options available
    if (!stackIsEmpty(&spellOptionsStack)) {
        spell = (Spell)stackPop(&spellOptionsStack);
    }

    if (option < SHOP_OPTION_COUNT) {
        spellOptions[option] = spell;
    }

    // Return name, description, and icon
    if (SETTINGS.language == LANGUAGE_EN) {
        pd->lua->pushString(spellData[spell].name);
        pd->lua->pushString(spellData[spell].description);
    } else if (SETTINGS.language == LANGUAGE_JP) {
        pd->lua->pushString(spellData[spell].name_jp);
        pd->lua->pushString(spellData[spell].description_jp);
    }
    pd->lua->pushBitmap(BITMAPS[spellData[spell].spellIconIndex]);

    return 3;
}

int selectSpellOption(lua_State* L) {
    int option = pd->lua->getArgInt(1);

    if (option < 0 || option >= SHOP_OPTION_COUNT) {
        return 0;
    }
    addSpellToDeck(spellOptions[option]);

    return 0;
}

void initShopManager(void) {
    pd->lua->addFunction(populateShopOptions, "Shop.populateOptions", NULL);
    pd->lua->addFunction(getSpellOption, "Shop.getSpellOption", NULL);
    pd->lua->addFunction(selectSpellOption, "Shop.selectSpellOption", NULL);
}