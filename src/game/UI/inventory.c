#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "helpers.h"
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

#define SPELL_TOP_LEFT_OFFSET_X -2
#define SPELL_TOP_LEFT_OFFSET_Y -2
#define SPELL_TOP_RIGHT_OFFSET_X 9
#define SPELL_TOP_RIGHT_OFFSET_Y -2
#define SPELL_BOTTOM_LEFT_OFFSET_X -2
#define SPELL_BOTTOM_LEFT_OFFSET_Y 13
#define SPELL_BOTTOM_RIGHT_OFFSET_X 9
#define SPELL_BOTTOM_RIGHT_OFFSET_Y 13

#define RUNE_TOP_LEFT_OFFSET_X -3
#define RUNE_TOP_LEFT_OFFSET_Y -1
#define RUNE_TOP_RIGHT_OFFSET_X 10
#define RUNE_TOP_RIGHT_OFFSET_Y -1
#define RUNE_BOTTOM_LEFT_OFFSET_X -3
#define RUNE_BOTTOM_LEFT_OFFSET_Y 12
#define RUNE_BOTTOM_RIGHT_OFFSET_X 10
#define RUNE_BOTTOM_RIGHT_OFFSET_Y 12

#define SELECT_OFFSET_X 3
#define SELECT_OFFSET_Y -3

#define LERP_SPEED 15

typedef enum buttonDir {
	DIR_LEFT,
	DIR_UP,
	DIR_RIGHT,
	DIR_DOWN,
	DIR_COUNT
} buttonDir;

int navigateTarget[SPELL_EQUIP_MAX][DIR_COUNT] = {
	{1, 2, 3, 4},
	{-1, 2, 3, 4},
	{1, -1, 3, 4},
	{1, 2, -1, 4},
	{1, 2, 3, 6}
};

int selectedIndex;
FPoint selectedPos;
int cursorIndex;
FPoint cursorTopLeft;
FPoint cursorTopRight;
FPoint cursorBottomLeft;
FPoint cursorBottomRight;
FPoint cursorTargetTopLeft;
FPoint cursorTargetTopRight;
FPoint cursorTargetBottomLeft;
FPoint cursorTargetBottomRight;
Point spellPositions[SPELL_DECK_MAX];

void initInventory(void) {
	selectedIndex = -1;
	cursorIndex = 1;

	cursorTopLeft = (FPoint){spellPositions[cursorIndex].x + SPELL_TOP_LEFT_OFFSET_X, spellPositions[cursorIndex].y + SPELL_TOP_LEFT_OFFSET_Y};
	cursorTopRight = (FPoint){spellPositions[cursorIndex].x + SPELL_TOP_RIGHT_OFFSET_X, spellPositions[cursorIndex].y + SPELL_TOP_RIGHT_OFFSET_Y};
	cursorBottomLeft = (FPoint){spellPositions[cursorIndex].x + SPELL_BOTTOM_LEFT_OFFSET_X, spellPositions[cursorIndex].y + SPELL_BOTTOM_LEFT_OFFSET_Y};
	cursorBottomRight = (FPoint){spellPositions[cursorIndex].x + SPELL_BOTTOM_RIGHT_OFFSET_X, spellPositions[cursorIndex].y + SPELL_BOTTOM_RIGHT_OFFSET_Y};

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
		if (spell == SPELL_NONE || selectedIndex == i) {
			pd->graphics->drawBitmap(BITMAPS[BITMAP_SPELL_EMPTY_ICON], spellPositions[i].x, spellPositions[i].y, kBitmapUnflipped);
		} else {
			pd->graphics->drawBitmap(getSpellIcon(spell), spellPositions[i].x, spellPositions[i].y, kBitmapUnflipped);
		}
	}

	for (int i = SPELL_EQUIP_MAX; i < SPELL_DECK_MAX; i++) {
		Spell spell = getDeckSpell(i);
		int invX = INV_BASE_X + ((i - SPELL_EQUIP_MAX) % SPELL_COL) * INV_X_GAP;
		int invY = INV_BASE_Y + ((i - SPELL_EQUIP_MAX) / SPELL_COL) * INV_Y_GAP;
		if (spell == SPELL_NONE || selectedIndex == i) {
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

	if (cursorIndex < SPELL_DECK_MAX) {
		int spellX = spellPositions[cursorIndex].x;
		int spellY = spellPositions[cursorIndex].y;
		cursorTargetTopLeft = (FPoint){spellX + SPELL_TOP_LEFT_OFFSET_X, spellY + SPELL_TOP_LEFT_OFFSET_Y};
		cursorTargetTopRight = (FPoint){spellX + SPELL_TOP_RIGHT_OFFSET_X, spellY + SPELL_TOP_RIGHT_OFFSET_Y};
		cursorTargetBottomLeft = (FPoint){spellX + SPELL_BOTTOM_LEFT_OFFSET_X, spellY + SPELL_BOTTOM_LEFT_OFFSET_Y};
		cursorTargetBottomRight = (FPoint){spellX + SPELL_BOTTOM_RIGHT_OFFSET_X, spellY + SPELL_BOTTOM_RIGHT_OFFSET_Y};
	} else {
		int runeIndex = cursorIndex - SPELL_DECK_MAX;
		int runeX = RUNE_BASE_X + (runeIndex % RUNE_COL) * INV_X_GAP;
		int runeY = INV_BASE_Y + (runeIndex / RUNE_COL) * INV_Y_GAP;
		cursorTargetTopLeft = (FPoint){runeX + RUNE_TOP_LEFT_OFFSET_X, runeY + RUNE_TOP_LEFT_OFFSET_Y};
		cursorTargetTopRight = (FPoint){runeX + RUNE_TOP_RIGHT_OFFSET_X, runeY + RUNE_TOP_RIGHT_OFFSET_Y};
		cursorTargetBottomLeft = (FPoint){runeX + RUNE_BOTTOM_LEFT_OFFSET_X, runeY + RUNE_BOTTOM_LEFT_OFFSET_Y};
		cursorTargetBottomRight = (FPoint){runeX + RUNE_BOTTOM_RIGHT_OFFSET_X, runeY + RUNE_BOTTOM_RIGHT_OFFSET_Y};
	}

	cursorTopLeft = lerpFPoint(cursorTopLeft, cursorTargetTopLeft, LERP_SPEED * deltaTime);
	cursorTopRight = lerpFPoint(cursorTopRight, cursorTargetTopRight, LERP_SPEED * deltaTime);
	cursorBottomLeft = lerpFPoint(cursorBottomLeft, cursorTargetBottomLeft, LERP_SPEED * deltaTime);
	cursorBottomRight = lerpFPoint(cursorBottomRight, cursorTargetBottomRight, LERP_SPEED * deltaTime);

	if (selectedIndex != -1) {
		int selectedTargetX = spellPositions[cursorIndex].x + SELECT_OFFSET_X;
		int selectedTargetY = spellPositions[cursorIndex].y + SELECT_OFFSET_Y;
		selectedPos = lerpFPoint(selectedPos, (FPoint){selectedTargetX, selectedTargetY}, LERP_SPEED * deltaTime);
		Spell spell = getDeckSpell(selectedIndex);
		if (spell != SPELL_NONE) {
			pd->graphics->drawBitmap(getSpellIcon(spell), selectedPos.x, selectedPos.y, kBitmapUnflipped);
		}
	} else {
		pd->graphics->drawBitmap(BITMAPS[BITMAP_CURSOR_TOP_LEFT], cursorTopLeft.x, cursorTopLeft.y, kBitmapUnflipped);
		pd->graphics->drawBitmap(BITMAPS[BITMAP_CURSOR_TOP_RIGHT], cursorTopRight.x, cursorTopRight.y, kBitmapUnflipped);
		pd->graphics->drawBitmap(BITMAPS[BITMAP_CURSOR_BOTTOM_LEFT], cursorBottomLeft.x, cursorBottomLeft.y, kBitmapUnflipped);
		pd->graphics->drawBitmap(BITMAPS[BITMAP_CURSOR_BOTTOM_RIGHT], cursorBottomRight.x, cursorBottomRight.y, kBitmapUnflipped);
	}

	PDButtons pushed;
	pd->system->getButtonState(NULL, &pushed, NULL);
	if (pushed & kButtonB) {
		if (selectedIndex == -1 ) {
			pd->sprite->removeAllSprites();
			pd->sprite->freeSprite(backgroundSprite);
			setScene(SCENE_GAME);
			createInventoryMenuItem();
			return;
		} else {
			cursorIndex = selectedIndex;
			selectedIndex = -1;
		}
	}

	if (pushed & kButtonA) {
		if (selectedIndex != -1) {
			swapSpells(selectedIndex, cursorIndex);
			selectedIndex = -1;
		} else if (cursorIndex < SPELL_DECK_MAX) {
			Spell spell = getDeckSpell(cursorIndex);
			if (spell != SPELL_NONE) {
				selectedIndex = cursorIndex;
				selectedPos = (FPoint){spellPositions[cursorIndex].x + SELECT_OFFSET_X, spellPositions[cursorIndex].y + SELECT_OFFSET_Y};
			}
		}
	}

	if (cursorIndex < SPELL_EQUIP_MAX) {
		int target = cursorIndex;
		if (pushed & kButtonLeft) {
			target = navigateTarget[cursorIndex][DIR_LEFT];
		} else if (pushed & kButtonUp) {
			target = navigateTarget[cursorIndex][DIR_UP];
		} else if (pushed & kButtonDown) {
			target = navigateTarget[cursorIndex][DIR_DOWN];
		} else if (pushed & kButtonRight) {
			target = navigateTarget[cursorIndex][DIR_RIGHT];
		}
		if (target > 0) {
			cursorIndex = target;
		}
	} else if (cursorIndex < SPELL_DECK_MAX) {
		int curCol = (cursorIndex - SPELL_EQUIP_MAX) % SPELL_COL;
		int curRow = (cursorIndex - SPELL_EQUIP_MAX) / SPELL_COL;
		if (pushed & kButtonLeft) {
			if (curCol > 0) {
				cursorIndex--;
			}
		} else if (pushed & kButtonUp) {
			if (curRow > 0) {
				cursorIndex -= SPELL_COL;
			} else if (curCol == 0) {
				cursorIndex = 1; // Left equipped spell index
			} else if (curCol == 1) {
				cursorIndex = 4; // Down equipped spell index
			} else if (curCol == 2) {
				cursorIndex = 3; // Right equipped spell index
			}
		} else if (pushed & kButtonDown) {
			if (curRow < 1) {
				cursorIndex += SPELL_COL;
			}
		} else if (pushed & kButtonRight) {
			if (curCol < SPELL_COL - 1) {
				cursorIndex++;
			} else if (selectedIndex == -1) {
				if (curRow == 0) {
				cursorIndex = SPELL_DECK_MAX;
				} else if (curRow == 1) {
					cursorIndex = SPELL_DECK_MAX + RUNE_COL;
				}
			}
		}
	} else {
		int curCol = (cursorIndex - SPELL_DECK_MAX) % RUNE_COL;
		int curRow = (cursorIndex - SPELL_DECK_MAX) / RUNE_COL;
		if (pushed & kButtonLeft) {
			if (curCol > 0) {
				cursorIndex--;
			} else if (curRow == 0) {
				cursorIndex = SPELL_DECK_MAX - SPELL_COL - 1;
			} else if (curRow == 1) {
				cursorIndex = SPELL_DECK_MAX - 1;
			}
		} else if (pushed & kButtonUp) {
			if (curRow > 0) {
				cursorIndex -= RUNE_COL;
			}
		} else if (pushed & kButtonDown) {
			if (curRow < 1) {
				cursorIndex += RUNE_COL;
			}
		} else if (pushed & kButtonRight) {
			if (curCol < RUNE_COL - 1) {
				cursorIndex++;
			}
		}
	}
}