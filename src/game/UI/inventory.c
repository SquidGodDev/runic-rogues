#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "game/UI/inventory.h"
#include "game/gameScene.h"
#include "game/player/spells.h"
#include "game/systems/runeManager.h"
#include "game/rendering/textures.h"
#include "game/settings/settings.h"

LCDSprite *backgroundSprite;

void openInventoryCallback(void* userdata) {
	if (scene != SCENE_INVENTORY) {
		pd->system->removeAllMenuItems();
		initInventory();
		setScene(SCENE_INVENTORY);
	}
}

void createInventoryMenuItem(void) {
    pd->system->removeAllMenuItems();
	pd->system->addMenuItem("Inventory", openInventoryCallback, NULL);
}

#define BASIC_SPELL_X 30
#define BASIC_SPELL_Y 24
#define LEFT_SPELL_X 14
#define LEFT_SPELL_Y 24
#define UP_SPELL_X 30
#define UP_SPELL_Y 6
#define RIGHT_SPELL_X 46
#define RIGHT_SPELL_Y 24
#define DOWN_SPELL_X 30
#define DOWN_SPELL_Y 42

#define INV_BASE_X 14
#define INV_BASE_Y 79
#define RUNE_BASE_X 77
#define INV_X_GAP 16
#define INV_Y_GAP 21
#define SPELL_ROW 2
#define SPELL_COL 3
#define RUNE_ROW 2
#define RUNE_COL 7

Point spellPositions[SPELL_DECK_MAX];

void initInventory(void) {
	backgroundSprite = pd->sprite->newSprite();
	pd->sprite->moveTo(backgroundSprite, 100, 60);
	pd->sprite->setImage(backgroundSprite, BITMAPS[BITMAP_INVENTORY_BACKGROUND], kBitmapUnflipped);
	pd->sprite->addSprite(backgroundSprite);
	spellPositions[0] = (Point){BASIC_SPELL_X, BASIC_SPELL_Y};
	spellPositions[1] = (Point){LEFT_SPELL_X, LEFT_SPELL_Y};
	spellPositions[2] = (Point){UP_SPELL_X, UP_SPELL_Y};
	spellPositions[3] = (Point){RIGHT_SPELL_X, RIGHT_SPELL_Y};
	spellPositions[4] = (Point){DOWN_SPELL_X, DOWN_SPELL_Y};
	for (int i = SPELL_EQUIP_MAX; i < SPELL_DECK_MAX; i++) {
		int invX = INV_BASE_X + ((i - SPELL_EQUIP_MAX) % SPELL_COL) * INV_X_GAP;
		int invY = INV_BASE_Y + ((i - SPELL_EQUIP_MAX) / SPELL_COL) * INV_Y_GAP;
		spellPositions[i] = (Point){invX, invY};
	}
}

void updateInventory(float deltaTime) {
	pd->sprite->updateAndDrawSprites();

	for (int i = 0; i < SPELL_DECK_MAX; i++) {
		Spell spell = getDeckSpell(i);
		if (spell == SPELL_NONE) {
			pd->graphics->drawBitmap(BITMAPS[BITMAP_SPELL_EMPTY_ICON], spellPositions[i].x, spellPositions[i].y, kBitmapUnflipped);
		} else {
			pd->graphics->drawBitmap(getSpellIcon(spell), spellPositions[i].x, spellPositions[i].y, kBitmapUnflipped);
		}
	}

	for (int i = SPELL_EQUIP_MAX; i < SPELL_DECK_MAX; i++) {
		Spell spell = getDeckSpell(i);
		int invX = INV_BASE_X + ((i - SPELL_EQUIP_MAX) % SPELL_COL) * INV_X_GAP;
		int invY = INV_BASE_Y + ((i - SPELL_EQUIP_MAX) / SPELL_COL) * INV_Y_GAP;
		if (spell == SPELL_NONE) {
			pd->graphics->drawBitmap(BITMAPS[BITMAP_SPELL_EMPTY_ICON], invX, invY, kBitmapUnflipped);
		} else {
			pd->graphics->drawBitmap(getSpellIcon(spell), invX, invY, kBitmapUnflipped);
		}
	}

	for (int i = 0; i < MAX_RUNES; i++) {
		int runeX = RUNE_BASE_X + (i % RUNE_COL) * INV_X_GAP;
		int runeY = INV_BASE_Y + (i / RUNE_COL) * INV_Y_GAP;
		if (getRune(i) == RUNE_TYPE_NONE) {
			pd->graphics->drawBitmap(BITMAPS[BITMAP_RUNE_EMPTY], runeX, runeY, kBitmapUnflipped);
		} else {
			pd->graphics->drawBitmap(BITMAPS[getRuneIconIndex(getRune(i))], runeX, runeY, kBitmapUnflipped);
		}
	}

	PDButtons pushed;
	pd->system->getButtonState(NULL, &pushed, NULL);
	if (pushed & kButtonB) {
		pd->sprite->removeAllSprites();
		pd->sprite->freeSprite(backgroundSprite);
		setScene(SCENE_GAME);
		createInventoryMenuItem();
		return;
	}
}