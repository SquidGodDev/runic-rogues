#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "game/rendering/textures.h"

uint8_t* TEXTURES[TEXTURE_COUNT];
const char *texturePathStrings[] = {
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
};

LCDBitmap* BITMAPS[BITMAP_PATHS_COUNT];
const char *bitmapPathStrings[] = {
	"images/player/hand",
	"images/player/handOpen",
	"images/UI/fullHeart",
	"images/UI/halfHeart",
	"images/UI/emptyHeart",
	"images/UI/minimapWall",
	"images/UI/minimapPlayer",
	"images/UI/minimapBorder",
	"images/UI/minimapEnemy",
	"images/UI/minimapProjectile",
	"images/UI/minimapMask",
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
								  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

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

	gameFont = pd->graphics->loadFont("fonts/m5x7-12-outlined", NULL);
	pd->graphics->setFont(gameFont);
}