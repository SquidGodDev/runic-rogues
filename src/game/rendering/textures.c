#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "game/rendering/textures.h"
#include "game/UI/popups.h"

uint8_t* TEXTURES[TEXTURE_COUNT];
TextureBounds* TEXTURE_BOUNDS[TEXTURE_COUNT];
const char *texturePathStrings[] = {
	"images/UI/minimap/minimapCampfire",
	"images/UI/minimap/minimapEnemy",
	"images/UI/minimap/minimapExit",
	"images/UI/minimap/minimapProjectile",
	"images/UI/minimap/minimapWarning",
	"images/UI/minimap/minimapCard",
	"images/UI/minimap/minimapShopItem",
	"images/environment/echoingCaverns/echoingCavernsWall",
	"images/environment/echoingCaverns/echoingCavernsCeiling",
	"images/environment/echoingCaverns/echoingCavernsFloor",
	"images/environment/echoingCaverns/echoingCavernsGate",
	"images/environment/lostArchives/lostArchivesWall",
	"images/environment/lostArchives/lostArchivesCeiling",
	"images/environment/lostArchives/lostArchivesFloor",
	"images/environment/lostArchives/lostArchivesGate",
	"images/environment/test/testWall",
	"images/environment/test/testCeiling",
	"images/environment/test/testFloor",
	"images/player/fireballTexture",
	"images/player/knife",
	"images/player/lightningBall",
	"images/particles/projectileBreak",
	"images/particles/projectileShrink",
	"images/particles/warning",
	"images/particles/rootAttack",
	"images/particles/lightningAttack",
	"images/particles/serpentTail",
	"images/particles/bomb",
	"images/particles/blackHole",
	"images/particles/spawn",
	"images/enemies/enemyProjectile",
	"images/enemies/bat",
	"images/enemies/slime",
	"images/enemies/smallSlime",
	"images/enemies/skeleton",
	"images/enemies/serpent",
	"images/enemies/turret",
	"images/enemies/elderBoss",
	"images/enemies/rat",
	"images/enemies/imp",
	"images/enemies/flyingTome",
	"images/enemies/livingArmor",
	"images/enemies/mummy",
	"images/enemies/wraithBoss",
	"images/enemies/wraith",
	"images/interactables/portal",
	"images/interactables/campfire",
	"images/interactables/extinguishedCampfire",
	"images/pickups/xpOrb"
};

LCDBitmap* BITMAPS[BITMAP_PATHS_COUNT];
const char *bitmapPathStrings[] = {
	"images/title/titleScreen",
	"images/title/gameEndScreen",
	"images/UI/health/heart",
	"images/UI/health/healthBar",
	"images/UI/experience/experienceBar",
	"images/UI/spells/dashIndicatorEmpty",
	"images/UI/spells/dashIndicatorFull",
	"images/UI/minimap/minimapWall",
	"images/UI/minimap/minimapBorder",
	"images/UI/minimap/minimapMask",
	"images/UI/wave/waveProgressBar",
	"images/UI/prompts/AButton",
	"images/UI/prompts/exitPrompt",
	"images/UI/prompts/exitPrompt_JP",
	"images/UI/prompts/restPrompt",
	"images/UI/prompts/restPrompt_JP",
	"images/shop/shopPopupBackground",
	"images/shop/cursor",
	"images/shop/skipCursor",
	"images/shop/skipIcon",
	"images/UI/prompts/bossHealthBar",
	"images/UI/prompts/elderBossTitle",
	"images/UI/prompts/elderBossTitle_JP",
	"images/UI/prompts/wraithBossTitle",
	"images/UI/prompts/wraithBossTitle_JP",
	"images/UI/spells/AButton",
	"images/UI/spells/BButton",
	"images/UI/spells/empty",
	"images/UI/spells/fireball",
	"images/runes/empty",
	"images/runes/vampirism",
	"images/runes/wingedBoots",
	"images/runes/healthy",
	"images/runes/swift",
	"images/runes/dagger",
	"images/runes/broadsword",
	"images/runes/pocketWatch",
	"images/runes/hourglass",
	"images/runes/slingshot",
	"images/runes/longbow",
	"images/runes/bearPaw",
	"images/runes/ironGauntlet",
	"images/runes/frostShard",
	"images/runes/iceLance",
	"images/runes/needleAndThread",
	"images/runes/hammerAndNails",
	"images/runes/magnifyingGlass",
	"images/runes/quartzCluster",
	"images/runes/enchantedAmulet",
	"images/runes/arcaneStaff"
};

LCDBitmapTable* BITMAP_TABLES[BITMAP_TABLE_PATHS_COUNT];
const char *bitmapTablePathStrings[] = {
	"images/UI/prompts/exitArrow",
	"images/player/hand",
	"images/player/sword",
	"images/player/knifeHand",
	"images/shop/ditherBackground"
};

LCDFont *fontTitleEN;
LCDFont *fontTitleJP;
LCDFont *fontSubEN;
LCDFont *fontSubJP;

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

void bitmapToTextureArray(LCDBitmap *bitmap, int textureIndex) {
	int width, height;
	pd->graphics->getBitmapData(bitmap, &width, &height, NULL, NULL, NULL);
	uint8_t *textureArray = pd->system->realloc(NULL, width * height * sizeof(uint8_t));
	int minX = width;
	int minY = height;
	int maxX = -1;
	int maxY = -1;

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			LCDColor drawColor = pd->graphics->getBitmapPixel(bitmap, x, y);
			textureArray[y * width + x] = (uint8_t)drawColor;
			int frameX = x;
			int frameY = y % 32;
			if (drawColor != kColorClear) {
                if (frameX < minX) minX = frameX;
                if (frameX > maxX) maxX = frameX;
                if (frameY < minY) minY = frameY;
                if (frameY > maxY) maxY = frameY;
            }
		}
	}

	TEXTURES[textureIndex] = textureArray;
	TextureBounds *bounds = (TextureBounds *)pd->system->realloc(NULL, sizeof(TextureBounds));
	bounds->minX = minX;
	bounds->minY = minY;
	bounds->maxX = maxX;
	bounds->maxY = maxY;
	TEXTURE_BOUNDS[textureIndex] = bounds;
}

void loadTexture(int textureIndex) {
	const char *texturePath = texturePathStrings[textureIndex];
	const char *outErr = NULL;
	LCDBitmap *textureBitmap = pd->graphics->loadBitmap(texturePath, &outErr);
	if (!textureBitmap) {
		pd->system->error(outErr);
	}
	bitmapToTextureArray(textureBitmap, textureIndex);
}

static const char *fontPathStrings[FONT_COUNT] = {
	"fonts/m6x11-12",
	"fonts/KH-Dot-Kabutochou-16-Outline",
	"fonts/m5x7-12",
	"fonts/KH-Dot-Kodenmachou-12-Outline"
};
static LCDFont **fontSlots[FONT_COUNT] = {
	&fontTitleEN,
	&fontTitleJP,
	&fontSubEN,
	&fontSubJP
};

void loadDefaultFont(void) {
	fontSubEN = pd->graphics->loadFont("fonts/m5x7-12", NULL);
}

int getTextureAssetCount(void) {
	return FONT_COUNT + TEXTURE_COUNT + BITMAP_PATHS_COUNT + BITMAP_TABLE_PATHS_COUNT;
}

void loadTextureAsset(int step) {
	if (step == 0) {
		if (sizeof(bitmapPathStrings) / sizeof(bitmapPathStrings[0]) != BITMAP_PATHS_COUNT) {
			pd->system->error("Bitmap path count does not match BITMAP_PATHS_COUNT");
		}
	}

	if (step < FONT_COUNT) {
		if (*fontSlots[step]) {
			return;
		}

		*fontSlots[step] = pd->graphics->loadFont(fontPathStrings[step], NULL);
		return;
	}
	step -= FONT_COUNT;

	if (step < TEXTURE_COUNT) {
		loadTexture(step);
		return;
	}
	step -= TEXTURE_COUNT;

	if (step < BITMAP_PATHS_COUNT) {
		const char *outErr = NULL;
		BITMAPS[step] = pd->graphics->loadBitmap(bitmapPathStrings[step], &outErr);
		if (!BITMAPS[step]) {
			pd->system->error(outErr);
		}
		return;
	}
	step -= BITMAP_PATHS_COUNT;

	if (step < BITMAP_TABLE_PATHS_COUNT) {
		const char *outErr = NULL;
		BITMAP_TABLES[step] = pd->graphics->loadBitmapTable(bitmapTablePathStrings[step], &outErr);
		if (!BITMAP_TABLES[step]) {
			pd->system->error(outErr);
		}
		if (step == BITMAP_TABLE_PATHS_COUNT - 1) {
			initPopups();
		}
		return;
	}
}