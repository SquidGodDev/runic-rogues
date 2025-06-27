#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "game/rendering/textures.h"

uint8_t* TEXTURES[TEXTURE_COUNT];
const char *texturePathStrings[] = {
	"images/UI/minimap/minimapCampfire",
	"images/UI/minimap/minimapEnemy",
	"images/UI/minimap/minimapExit",
	"images/UI/minimap/minimapProjectile",
	"images/environment/echoingCaverns/echoingCavernsWall",
	"images/environment/echoingCaverns/echoingCavernsCeiling",
	"images/environment/echoingCaverns/echoingCavernsFloor",
	"images/environment/lostArchives/lostArchivesWall",
	"images/environment/lostArchives/lostArchivesCeiling",
	"images/environment/lostArchives/lostArchivesFloor",
	"images/environment/test/testWall",
	"images/environment/test/testCeiling",
	"images/environment/test/testFloor",
	"images/player/fireballTexture",
	"images/projectileBreak",
	"images/enemies/enemyProjectile",
	"images/enemies/bat",
	"images/enemies/slime",
	"images/enemies/smallSlime",
	"images/enemies/skeleton",
	"images/enemies/imp",
	"images/enemies/flyingTome",
	"images/enemies/livingArmor",
	"images/enemies/wraithBoss",
	"images/enemies/wraith",
	"images/interactables/portal",
	"images/interactables/campfire",
	"images/interactables/extinguishedCampfire",
};

LCDBitmap* BITMAPS[BITMAP_PATHS_COUNT];
const char *bitmapPathStrings[] = {
	"images/player/hand",
	"images/player/handOpen",
	"images/UI/hearts/full",
	"images/UI/hearts/half",
	"images/UI/hearts/empty",
	"images/UI/hearts/halfContainerEmpty",
	"images/UI/hearts/halfContainerFull",
	"images/UI/mana/manaBar",
	"images/UI/minimap/minimapWall",
	"images/UI/minimap/minimapBorder",
	"images/UI/minimap/minimapMask",
	"images/UI/prompts/AButton",
	"images/UI/prompts/exitPrompt",
	"images/UI/prompts/exitPrompt_JP",
	"images/UI/prompts/restPrompt",
	"images/UI/prompts/restPrompt_JP",
	"images/upgrades/icons/attackCooldown",
	"images/upgrades/icons/maxHealth",
	"images/upgrades/icons/maxMana",
	"images/upgrades/icons/manaRegen",
	"images/upgrades/icons/moveSpeed",
	"images/upgrades/icons/projectileSpeed",
	"images/upgrades/icons/vampirism"
};

LCDBitmapTable* BITMAP_TABLES[BITMAP_TABLE_PATHS_COUNT];
const char *bitmapTablePathStrings[] = {
	"images/UI/prompts/exitArrow",
};

LCDFont *gameFont;

uint8_t fogPatterns[FOG_LEVELS][8] = {{0x11, 0x00, 0x44, 0x00, 0x11, 0x00, 0x44, 0x00},
								  {0x55, 0x00, 0x44, 0x00, 0x55, 0x00, 0x44, 0x00},
								  {0x55, 0x00, 0x55, 0x00, 0x55, 0x00, 0x55, 0x00},
								  {0x55, 0x00, 0x55, 0x88, 0x55, 0x00, 0x55, 0x88},
								  {0x55, 0x22, 0x55, 0x88, 0x55, 0x22, 0x55, 0x88},
								  {0x55, 0xAA, 0x55, 0x88, 0x55, 0xAA, 0x55, 0x88},
								  {0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA},
								  {0x55, 0xAA, 0xDD, 0xAA, 0x55, 0xAA, 0xDD, 0xAA},
								  {0x77, 0xAA, 0xDD, 0xAA, 0x77, 0xAA, 0xDD, 0xAA},
								  {0xFF, 0xAA, 0xDD, 0xAA, 0xFF, 0xAA, 0xDD, 0xAA},
								  {0xFF, 0xAA, 0xFF, 0xAA, 0xFF, 0xAA, 0xFF, 0xAA},
								  {0xFF, 0xBB, 0xFF, 0xAA, 0xFF, 0xBB, 0xFF, 0xAA},
								  {0xFF, 0xBB, 0xFF, 0xEE, 0xFF, 0xBB, 0xFF, 0xEE},
								  {0xFF, 0xFF, 0xFF, 0xEE, 0xFF, 0xFF, 0xFF, 0xEE},
								  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
								  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

/* Lua program to print pattern
function getBinaryString(n)
    local binary = ""
    for i=1, 8 do
        binary = ((n % 2) == 1 and "O " or ". ") .. binary
        n = math.floor(n / 2)
    end
    return binary
end

function printPattern(patternTable)
	local printStr = ""
	for i=1, #patternTable do
		print(getBinaryString(patternTable[i]))
	end
end

for _, pattern in ipairs(patterns) do
	printPattern(pattern)
	print("")
end
*/

uint8_t* loadTexture(const char *texturePath) {
	const char *outErr = NULL;
	LCDBitmap *textureBitmap = pd->graphics->loadBitmap(texturePath, &outErr);
	if (!textureBitmap) {
		pd->system->error(outErr);
	}
	int width, height;
	pd->graphics->getBitmapData(textureBitmap, &width, &height, NULL, NULL, NULL);
	uint8_t *textureArray = pd->system->realloc(NULL, width * height * sizeof(uint8_t));
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			LCDColor drawColor = pd->graphics->getBitmapPixel(textureBitmap, x, y);
			textureArray[y * width + x] = (uint8_t)drawColor;
		}
	}
	return textureArray;
}

void initTextures(void)
{
	for (int i = 0; i < TEXTURE_COUNT; i++) {
		TEXTURES[i] = loadTexture(texturePathStrings[i]);
	}

	for (int i = 0; i < BITMAP_PATHS_COUNT; i++) {
		const char *outErr = NULL;
		LCDBitmap *loadedBitmap = pd->graphics->loadBitmap(bitmapPathStrings[i], &outErr);
		if (!loadedBitmap) {
			pd->system->error(outErr);
		}
		BITMAPS[i] = pd->graphics->loadBitmap(bitmapPathStrings[i], NULL);
	}

	for (int i = 0; i < BITMAP_TABLE_PATHS_COUNT; i++) {
		const char *outErr = NULL;
		BITMAP_TABLES[i] = pd->graphics->loadBitmapTable(bitmapTablePathStrings[i], &outErr);
		if (!BITMAP_TABLES[i]) {
			pd->system->error(outErr);
		}
	}

	gameFont = pd->graphics->loadFont("fonts/m5x7-12-outlined", NULL);
	pd->graphics->setFont(gameFont);
}