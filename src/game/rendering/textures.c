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
	"images/UI/minimap/minimapWarning",
	"images/UI/minimap/minimapCard",
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
	"images/particles/projectileBreak",
	"images/particles/warning",
	"images/particles/rootAttack",
	"images/enemies/enemyProjectile",
	"images/enemies/bat",
	"images/enemies/slime",
	"images/enemies/smallSlime",
	"images/enemies/skeleton",
	"images/enemies/elderBoss",
	"images/enemies/imp",
	"images/enemies/flyingTome",
	"images/enemies/livingArmor",
	"images/enemies/wraithBoss",
	"images/enemies/wraith",
	"images/interactables/portal",
	"images/interactables/campfire",
	"images/interactables/extinguishedCampfire"
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
	"images/spells/icons/dashIndicatorEmpty",
	"images/spells/icons/dashIndicatorFull",
	"images/UI/minimap/minimapWall",
	"images/UI/minimap/minimapBorder",
	"images/UI/minimap/minimapMask",
	"images/UI/prompts/AButton",
	"images/UI/prompts/exitPrompt",
	"images/UI/prompts/exitPrompt_JP",
	"images/UI/prompts/restPrompt",
	"images/UI/prompts/restPrompt_JP",
	"images/UI/prompts/bossHealthBar",
	"images/UI/prompts/elderBossTitle",
	"images/UI/prompts/elderBossTitle_JP",
	"images/UI/prompts/wraithBossTitle",
	"images/UI/prompts/wraithBossTitle_JP",
	"images/upgrades/icons/attackCooldown",
	"images/upgrades/icons/maxHealth",
	"images/upgrades/icons/maxMana",
	"images/upgrades/icons/manaRegen",
	"images/upgrades/icons/moveSpeed",
	"images/upgrades/icons/projectileSpeed",
	"images/upgrades/icons/vampirism",
	"images/spells/icons/emptyIcon",
	"images/spells/icons/fireballIcon",
	"images/spells/icons/tripleShotIcon",
	"images/spells/icons/shotgunIcon",
	"images/spells/icons/laserIcon",
	"images/spells/icons/rootAttackIcon",
	"images/spells/icons/waveIcon",
	"images/spells/icons/tripleWaveIcon"
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

uint8_t* bitmapToTextureArray(LCDBitmap *bitmap) {
	int width, height;
	pd->graphics->getBitmapData(bitmap, &width, &height, NULL, NULL, NULL);
	uint8_t *textureArray = pd->system->realloc(NULL, width * height * sizeof(uint8_t));
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			LCDColor drawColor = pd->graphics->getBitmapPixel(bitmap, x, y);
			textureArray[y * width + x] = (uint8_t)drawColor;
		}
	}
	return textureArray;
}

uint8_t* loadTexture(const char *texturePath) {
	const char *outErr = NULL;
	LCDBitmap *textureBitmap = pd->graphics->loadBitmap(texturePath, &outErr);
	if (!textureBitmap) {
		pd->system->error(outErr);
	}
	return bitmapToTextureArray(textureBitmap);
}

void initTextures(void)
{
	for (int i = 0; i < TEXTURE_COUNT; i++) {
		if (i < TEXTURE_SPELL_NONE) {
			TEXTURES[i] = loadTexture(texturePathStrings[i]);
		}
	}

	// Check if the number of bitmap paths matches the defined count
	if (sizeof(bitmapPathStrings) / sizeof(bitmapPathStrings[0]) != BITMAP_PATHS_COUNT) {
		pd->system->error("Bitmap path count does not match BITMAP_PATHS_COUNT");
	}
	for (int i = 0; i < BITMAP_PATHS_COUNT; i++) {
		const char *outErr = NULL;
		LCDBitmap *loadedBitmap = pd->graphics->loadBitmap(bitmapPathStrings[i], &outErr);
		if (!loadedBitmap) {
			pd->system->error(outErr);
		}
		BITMAPS[i] = pd->graphics->loadBitmap(bitmapPathStrings[i], NULL);
		if (i >= BITMAP_SPELL_EMPTY_ICON) {
			int frames = 8;
			LCDBitmap *spellBitmap = pd->graphics->newBitmap(32, 32 * frames, kColorClear);
			int baseX = 10, baseY = 8;
			pd->graphics->pushContext(spellBitmap);
				pd->graphics->drawBitmap(loadedBitmap, baseX, baseY, kBitmapUnflipped);
				pd->graphics->drawBitmap(loadedBitmap, baseX, baseY + 32 + 1, kBitmapUnflipped);
				pd->graphics->drawBitmap(loadedBitmap, baseX, baseY + 32 * 2 + 2, kBitmapUnflipped);
				pd->graphics->drawBitmap(loadedBitmap, baseX, baseY + 32 * 3 + 3, kBitmapUnflipped);
				pd->graphics->drawBitmap(loadedBitmap, baseX, baseY + 32 * 4 + 3, kBitmapUnflipped);
				pd->graphics->drawBitmap(loadedBitmap, baseX, baseY + 32 * 5 + 2, kBitmapUnflipped);
				pd->graphics->drawBitmap(loadedBitmap, baseX, baseY + 32 * 6 + 1, kBitmapUnflipped);
				pd->graphics->drawBitmap(loadedBitmap, baseX, baseY + 32 * 7, kBitmapUnflipped);
			pd->graphics->popContext();
			TEXTURES[TEXTURE_SPELL_NONE + (i - BITMAP_SPELL_EMPTY_ICON)] = bitmapToTextureArray(spellBitmap);
		}
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