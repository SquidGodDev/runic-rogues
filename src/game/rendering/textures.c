#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "game/rendering/textures.h"

uint8_t *wallTexture;
uint8_t *floorTexture;
uint8_t *ceilingTexture;

uint8_t *projectileTexture;
uint8_t *projectileBreakTexture;

uint8_t *batTexture;
uint8_t *slimeTexture;
uint8_t *babySlimeTexture;
uint8_t *skeletonTexture;

LCDBitmap *handBitmap;
LCDBitmap *handOpenBitmap;

LCDBitmap *fullHeartBitmap;
LCDBitmap *halfHeartBitmap;
LCDBitmap *emptyHeartBitmap;

LCDFont *gameFont;

int fogPatterns[FOG_LEVELS][8] = {{0x11, 0x00, 0x44, 0x00, 0x11, 0x00, 0x44, 0x00},
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
	LCDBitmap *textureBitmap = pd->graphics->loadBitmap(texturePath, NULL);
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
	wallTexture = loadTexture("images/brickTexture");
	floorTexture = loadTexture("images/floorTexture");
	ceilingTexture = loadTexture("images/floorTexture");
	batTexture = loadTexture("images/enemies/bat");
	slimeTexture = loadTexture("images/enemies/slime");
	babySlimeTexture = loadTexture("images/enemies/smallSlime");
	skeletonTexture = loadTexture("images/enemies/skeleton");
	projectileTexture = loadTexture("images/fireballTexture");
	projectileBreakTexture = loadTexture("images/projectileBreak");

	handBitmap = pd->graphics->loadBitmap("images/hand", NULL);
	handOpenBitmap = pd->graphics->loadBitmap("images/handOpen", NULL);
	fullHeartBitmap = pd->graphics->loadBitmap("images/fullHeart", NULL);
	halfHeartBitmap = pd->graphics->loadBitmap("images/halfHeart", NULL);
	emptyHeartBitmap = pd->graphics->loadBitmap("images/emptyHeart", NULL);

	gameFont = pd->graphics->loadFont("fonts/m5x7-12-outlined", NULL);
	pd->graphics->setFont(gameFont);
}