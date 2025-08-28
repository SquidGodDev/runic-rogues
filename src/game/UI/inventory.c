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

#define BASIC_SPELL_X 24
#define BASIC_SPELL_Y 24
#define LEFT_SPELL_X BASIC_SPELL_X - 16
#define LEFT_SPELL_Y BASIC_SPELL_Y
#define UP_SPELL_X BASIC_SPELL_X
#define UP_SPELL_Y BASIC_SPELL_Y - 18
#define RIGHT_SPELL_X BASIC_SPELL_X + 16
#define RIGHT_SPELL_Y BASIC_SPELL_Y
#define DOWN_SPELL_X BASIC_SPELL_X
#define DOWN_SPELL_Y BASIC_SPELL_Y + 18

#define INV_BASE_X 8
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

float descriptionOffset;
int maxDescriptionOffset;

#define HEADER_X 58
#define HEADER_Y 4
#define DESC_X 62
#define DESC_Y 40
#define DESC_MAX_WIDTH 124
#define DESC_BOX_HEIGHT 34
#define SCROLLBAR_LENGTH 28
#define SCROLLBAR_SENSITIVITY 0.2f

typedef struct InfoPageData {
	LCDBitmap *headerBitmap;
	LCDBitmap *descBitmap;
	int descHeight;
} InfoPageData;

#define MAX_INVENTORY_SLOTS SPELL_DECK_MAX + MAX_RUNES
InfoPageData spellInfoPages[SPELL_COUNT];
InfoPageData runeInfoPages[RUNE_TYPE_COUNT];

InfoPageData createInfoPage(char *name, char *desc, char* type, LCDBitmap *icon, Language language) {
    LCDFont *fontTitle = fontTitleEN;
    LCDFont *fontSub = fontSubEN;
    LCDBitmapDrawMode drawMode = kDrawModeCopy;
    int titleTracking = -1;
    int tracking = -2;
    int leading = 0;
	int subheadingDrawY = language == LANGUAGE_JP ? 19 : 20;
    if (language == LANGUAGE_JP) {
        fontTitle = fontTitleJP;
		titleTracking = 0;
        fontSub = fontSubJP;
        drawMode = kDrawModeFillWhite;
        tracking = 1;
        leading = 2;
    }

    LCDBitmap *infoHeaderBitmap = pd->graphics->newBitmap(140, 40, kColorClear);
    pd->graphics->pushContext(infoHeaderBitmap);
        pd->graphics->drawScaledBitmap(icon, 0, 0, 2.0f, 2.0f);
        pd->graphics->setDrawMode(drawMode);
        pd->graphics->setFont(fontTitle);
        pd->graphics->setTextTracking(titleTracking);
        pd->graphics->drawText(name, strlen(name), kUTF8Encoding, 25, 2);
        pd->graphics->setFont(fontSub);
        pd->graphics->setTextTracking(tracking);
        pd->graphics->setTextLeading(leading);
        pd->graphics->drawText(type, strlen(type), kUTF8Encoding, 26, subheadingDrawY);
    pd->graphics->popContext();

	int descHeight = pd->graphics->getTextHeightForMaxWidth(fontSub, desc, strlen(desc), DESC_MAX_WIDTH, kUTF8Encoding, kWrapWord, tracking, leading);
	LCDBitmap *infoDescBitmap = pd->graphics->newBitmap(DESC_MAX_WIDTH, descHeight, kColorClear);
	pd->graphics->pushContext(infoDescBitmap);
		pd->graphics->setDrawMode(drawMode);
		pd->graphics->setFont(fontSub);
		pd->graphics->setTextTracking(tracking);
		pd->graphics->setTextLeading(leading);
		pd->graphics->drawTextInRect(desc, strlen(desc), kUTF8Encoding, 0, 0, DESC_MAX_WIDTH, descHeight, kWrapWord, kAlignTextLeft);
	pd->graphics->popContext();

	return (InfoPageData){
		.headerBitmap = infoHeaderBitmap,
		.descBitmap = infoDescBitmap,
		.descHeight = descHeight
	};
}

InfoPageData createSpellInfoPage(Spell spell, Language language) {
    SpellData curSpellData = spellData[spell];
    char *name = curSpellData.name;
    char *desc = curSpellData.description;
    char *type = "Spell";
    if (language == LANGUAGE_JP) {
        name = curSpellData.name_jp;
        desc = curSpellData.description_jp;
        type = "スペル";
    }
    return createInfoPage(name, desc, type, BITMAPS[curSpellData.spellIconIndex], language);
}

InfoPageData createRuneInfoPage(RuneType runeType, Language language) {
    char *name = getRuneName(runeType, language);
    char *desc = getRuneDescription(runeType, language);
    char *type = language == LANGUAGE_JP ? "ルーン" : "Rune";
    return createInfoPage(name, desc, type, BITMAPS[getRuneIconIndex(runeType)], language);
}

void drawInfoPage(InfoPageData *pageData) {
    pd->graphics->drawBitmap(pageData->headerBitmap, HEADER_X, HEADER_Y, kBitmapUnflipped);
	pd->graphics->pushContext(NULL);
		pd->graphics->setClipRect(DESC_X, DESC_Y, DESC_MAX_WIDTH, DESC_BOX_HEIGHT);
		pd->graphics->drawBitmap(pageData->descBitmap, DESC_X, DESC_Y - descriptionOffset, kBitmapUnflipped);
	pd->graphics->popContext();

	pd->graphics->drawRect(DESC_X - 3, DESC_Y - 3, DESC_MAX_WIDTH + 12, DESC_BOX_HEIGHT + 6, kColorWhite);
	float scrollbarPercentage = (float)DESC_BOX_HEIGHT / pageData->descHeight;
	if (scrollbarPercentage < 1.0f) {
		int scrollbarX = DESC_X + DESC_MAX_WIDTH + 1;
		int scrollbarY = DESC_Y;
		pd->graphics->drawRoundRect(scrollbarX, scrollbarY, 5, DESC_BOX_HEIGHT, 1, 1, kColorWhite);
		int scrollbarLength = SCROLLBAR_LENGTH * scrollbarPercentage;
		int scrollbarOffset = SCROLLBAR_LENGTH * ((float)descriptionOffset / pageData->descHeight);
		int scrollbarLineX = scrollbarX + 2;
		int scrollbarLineY = scrollbarY + 3 + scrollbarOffset;
		pd->graphics->drawLine(scrollbarLineX, scrollbarLineY, scrollbarLineX, scrollbarLineY + scrollbarLength, 1, kColorWhite);

		maxDescriptionOffset = pageData->descHeight - DESC_BOX_HEIGHT > 0 ? pageData->descHeight - DESC_BOX_HEIGHT : 0;
	} else {
		maxDescriptionOffset = 0;
	}
}

void initInventory(void) {
	selectedIndex = -1;
	cursorIndex = 1;
	descriptionOffset = 0;
	maxDescriptionOffset = 0;

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

	if (cursorIndex < SPELL_DECK_MAX) {
		Spell spell = getDeckSpell(cursorIndex);
		if (spell != SPELL_NONE) {
			InfoPageData pageData = spellInfoPages[spell];
			if (pageData.headerBitmap == NULL) {
				pageData = createSpellInfoPage(spell, SETTINGS.language);
				spellInfoPages[spell] = pageData;
			}
			drawInfoPage(&pageData);
		}
	} else {
		RuneType runeType = getRune(cursorIndex - SPELL_DECK_MAX);
		if (runeType != RUNE_TYPE_NONE) {
			InfoPageData pageData = runeInfoPages[runeType];
			if (pageData.headerBitmap == NULL) {
				pageData = createRuneInfoPage(runeType, SETTINGS.language);
				runeInfoPages[runeType] = pageData;
			}
			drawInfoPage(&pageData);
		}
	}

	float crankChange = pd->system->getCrankChange() * SCROLLBAR_SENSITIVITY;
	descriptionOffset += crankChange;
	if (descriptionOffset < 0) {
		descriptionOffset = 0;
	} else if (descriptionOffset > maxDescriptionOffset) {
		descriptionOffset = maxDescriptionOffset;
	}

	int prevCursorIndex = cursorIndex;

	PDButtons pushed;
	pd->system->getButtonState(NULL, &pushed, NULL);
	if (pushed & kButtonB) {
		if (selectedIndex == -1 ) {
			pd->sprite->removeAllSprites();
			pd->sprite->freeSprite(backgroundSprite);
			for (int i = 0; i < SPELL_COUNT; i++) {
				if (spellInfoPages[i].headerBitmap != NULL) {
					pd->graphics->freeBitmap(spellInfoPages[i].headerBitmap);
				}
				if (spellInfoPages[i].descBitmap != NULL) {
					pd->graphics->freeBitmap(spellInfoPages[i].descBitmap);
				}
				spellInfoPages[i] = (InfoPageData){0};
			}
			for (int i = 0; i < RUNE_TYPE_COUNT; i++) {
				if (runeInfoPages[i].headerBitmap != NULL) {
					pd->graphics->freeBitmap(runeInfoPages[i].headerBitmap);
				}
				if (runeInfoPages[i].descBitmap != NULL) {
					pd->graphics->freeBitmap(runeInfoPages[i].descBitmap);
				}
				runeInfoPages[i] = (InfoPageData){0};
			}
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

	if (prevCursorIndex != cursorIndex) {
		descriptionOffset = 0;
	}
}