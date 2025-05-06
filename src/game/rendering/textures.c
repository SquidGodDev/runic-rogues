#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "game/rendering/textures.h"

uint8_t *echoingCavernsWall;
uint8_t *echoingCavernsCeiling;
uint8_t *echoingCavernsFloor;

uint8_t *lostArchivesWall;
uint8_t *lostArchivesCeiling;
uint8_t *lostArchivesFloor;

uint8_t *testWall;
uint8_t *testCeiling;
uint8_t *testFloor;

uint8_t *projectileTexture;
uint8_t *projectileBreakTexture;

uint8_t *enemyProjectileTexture;
uint8_t *batTexture;
uint8_t *slimeTexture;
uint8_t *babySlimeTexture;
uint8_t *skeletonTexture;
uint8_t *impTexture;
uint8_t *flyingTomeTexture;
uint8_t *livingArmorTexture;

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
	echoingCavernsWall = loadTexture("images/environment/echoingCaverns/echoingCavernsWall");
	echoingCavernsCeiling = loadTexture("images/environment/echoingCaverns/echoingCavernsCeiling");
	echoingCavernsFloor = loadTexture("images/environment/echoingCaverns/echoingCavernsFloor");
	lostArchivesWall = loadTexture("images/environment/lostArchives/lostArchivesWall");
	lostArchivesCeiling = loadTexture("images/environment/lostArchives/lostArchivesCeiling");
	lostArchivesFloor = loadTexture("images/environment/lostArchives/lostArchivesFloor");
	testWall = loadTexture("images/environment/test/testWall");
	testCeiling = loadTexture("images/environment/test/testCeiling");
	testFloor = loadTexture("images/environment/test/testFloor");

	batTexture = loadTexture("images/enemies/bat");
	slimeTexture = loadTexture("images/enemies/slime");
	babySlimeTexture = loadTexture("images/enemies/smallSlime");
	skeletonTexture = loadTexture("images/enemies/skeleton");
	impTexture = loadTexture("images/enemies/imp");
	flyingTomeTexture = loadTexture("images/enemies/flyingTome");
	livingArmorTexture = loadTexture("images/enemies/livingArmor");

	enemyProjectileTexture = loadTexture("images/enemies/enemyProjectile");
	projectileTexture = loadTexture("images/player/fireballTexture");
	projectileBreakTexture = loadTexture("images/projectileBreak");

	handBitmap = pd->graphics->loadBitmap("images/player/hand", NULL);
	handOpenBitmap = pd->graphics->loadBitmap("images/player/handOpen", NULL);
	fullHeartBitmap = pd->graphics->loadBitmap("images/player/fullHeart", NULL);
	halfHeartBitmap = pd->graphics->loadBitmap("images/player/halfHeart", NULL);
	emptyHeartBitmap = pd->graphics->loadBitmap("images/player/emptyHeart", NULL);

	gameFont = pd->graphics->loadFont("fonts/m5x7-12-outlined", NULL);
	pd->graphics->setFont(gameFont);
}