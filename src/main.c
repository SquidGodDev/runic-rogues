#define TARGET_EXTENSION 1

#include <stdio.h>
#include <math.h>
#include <float.h>

#define DRAW_MAP 0

#define PI 3.141592f
#define P2 PI / 2
#define P3 3 * PI / 2

#define TARGET_FPS 30
#define SCREEN_WIDTH 200
#define HALF_SCREEN_WIDTH SCREEN_WIDTH / 2
#define SCREEN_HEIGHT 120
#define HALF_SCREEN_HEIGHT SCREEN_HEIGHT / 2
#define CELL_SIZE 16
#define MAP_WIDTH 20
#define MAP_HEIGHT 20

#define MAX_DRAWABLE_COUNT 20
#define DRAWABLE_TEXTURE_SIZE 32

#define MAX_ENEMY_COUNT 10

#define MAX_PROJECTILE_COUNT 10

#define FOG_LEVELS 15
#define FOG_STEP 6
#define FOG_MIN_DIST 60
#define FOG_MAX_DIST (FOG_MIN_DIST + FOG_STEP * FOG_LEVELS)

#include "pd_api.h"
static PlaydateAPI *pd = NULL;

static inline float fastRsqrt(float x)
{
	union {
		float f;
		uint32_t u;
	} f32u32;
	f32u32.f = x;
	f32u32.u = 0x5f3759df - (f32u32.u >> 1);
	f32u32.f *= 1.5f - (x * 0.5f * f32u32.f * f32u32.f);
	return f32u32.f;
}
// static inline float fastRcp(float x) { x = fastRsqrt(x); return x * x; }
// static inline float fastDiv(float x, float y) { return x * fastRcp(y); }
static inline float fastSqrt(float x) { return x * fastRsqrt(x); }
static inline float distanceSquared(float x1, float y1, float x2, float y2)
{
	return (y2 - y1) * (y2 - y1) + (x2 - x1) * (x2 - x1);
}

static inline uint32_t swap(uint32_t n)
{
#if TARGET_PLAYDATE
	//return __REV(n);
	uint32_t result;

	__asm volatile ("rev %0, %1" : "=l" (result) : "l" (n));
	return(result);
#else
	return ((n & 0xff000000) >> 24) | ((n & 0xff0000) >> 8) | ((n & 0xff00) << 8) | (n << 24);
#endif
}

static inline float lerp(float start, float end, float t) {
	return (1 - t) * start + t * end;
}

typedef struct DrawablePair {
	int order;
	float dist;
} DrawablePair;

int drawablePairComparison(const void *a, const void *b) {
	return ((DrawablePair *)b)->dist - ((DrawablePair *)a)->dist;
}

void sortDrawables(int* order, float* dist, int count) {
	DrawablePair drawablePairs[count];
	for (int i = 0; i < count; i++) {
		drawablePairs[i].order = order[i];
		drawablePairs[i].dist = dist[i];
	}

	qsort(drawablePairs, count, sizeof(DrawablePair), drawablePairComparison);

	for (int i = 0; i < count; i++) {
		order[i] = drawablePairs[i].order;
		dist[i] = drawablePairs[i].dist;
	}
}

void drawCircleAtPoint(float x, float y, int radius)
{
	int drawX = (int)x - radius;
	int drawY = (int)y - radius;
	int width = radius * 2;
	int height = radius * 2;
	pd->graphics->drawEllipse(drawX, drawY, width, height, 1, 0, 0, kColorWhite);
}

int map[] =
{
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

/* Lua program to create basic map
================================
local width = 20
local height = 12

for h = 1, height do
  local rowString = ""
  for w = 1, width do
	  local cell = "0, "
	  if w == width then
		if h == height then
		  cell = "1"
		else
		  cell = "1,"
		end
	  elseif h == 1 or h == height or w == 1 then
		cell = "1, "
	  end
	  rowString = rowString .. cell
  end
  print(rowString)
end
*/

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

int defaultStrafeSetting = 0;
int toggleMovementSetting = 0;
int movementToggled = 0;

float playerX, playerY;
int playerRadius;
float playerAngle;
float playerTurnSpeed;
float playerTurnAccel;
float playerTurnDrag;
float playerTurnMaxSpeed;
float playerTurnInitialSpeed;

float playerXVelocity, playerYVelocity;
float playerSpeed;
float playerInitialSpeed;
float playerAccel;
float playerDrag;
float playerMaxSpeed;
float playerStrafeSpeed;

float planeX, planeY;
float dirX, dirY;

float weaponCooldown = 0.3f; // Seconds
float weaponCurCooldown = 0;
float weaponBobCounter = 0;
float weaponBobSpeed = 0.15f;
int weaponBasePositionX = 126;
int weaponBasePositionY = 75;
int weaponXOffset = 5;
int weaponYOffset = 3;
int weaponActiveXOffset = -4;
int weaponActiveYOffset = -7;
float weaponTargetDrawX = 0;
float weaponTargetDrawY = 0;
float weaponDrawX = 0;
float weaponDrawY = 0;

int queuedDrawables = 0;

enum DrawableType {
	ENEMY,
	PROJECTILE
};

typedef struct Drawable {
	float x;
	float y;
	uint32_t type;
} Drawable;

Drawable drawables[MAX_DRAWABLE_COUNT] = {0};

typedef struct Enemy {
	float x;
	float y;
	float direction;
	float speed;
	uint32_t active;
} Enemy;

Enemy enemies[MAX_ENEMY_COUNT] = {0};

typedef struct Projectile {
	float x;
	float y;
	float direction;
	float speed;
	uint32_t active;
} Projectile;

Projectile projectiles[MAX_PROJECTILE_COUNT] = {0};

#define SKYBOX_SIZE (SCREEN_HEIGHT / 2) * 13
uint8_t wallTexture[256];
uint8_t floorTexture[256];
uint8_t ceilingTexture[256];
uint8_t enemyTexture[1024];
uint8_t projectileTexture[1024];
uint32_t skybox[SKYBOX_SIZE];

LCDBitmap *handBitmap;
LCDBitmap *handOpenBitmap;

void loadTexture(const char *texturePath, uint8_t *textureArray) {
	LCDBitmap *textureBitmap = pd->graphics->loadBitmap(texturePath, NULL);
	int width, height;
	pd->graphics->getBitmapData(textureBitmap, &width, &height, NULL, NULL, NULL);
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			LCDColor drawColor = pd->graphics->getBitmapPixel(textureBitmap, x, y);
			textureArray[y * width + x] = (uint8_t)drawColor;
		}
	}
}

void loadTextures(void)
{
	loadTexture("images/brickTexture", wallTexture);
	loadTexture("images/floorTexture", floorTexture);
	loadTexture("images/floorTexture", ceilingTexture);
	loadTexture("images/enemyTexture", enemyTexture);
	loadTexture("images/fireballTexture", projectileTexture);

	handBitmap = pd->graphics->loadBitmap("images/hand", NULL);
	handOpenBitmap = pd->graphics->loadBitmap("images/handOpen", NULL);

	LCDBitmap *skyboxBitmap = pd->graphics->loadBitmap("images/skybox", NULL);
	for (int y = 0; y < SCREEN_HEIGHT / 2; y++) {
		for (int drawInt32 = 0; drawInt32 < 13; drawInt32++) {
			uint32_t curDrawInt = 0;
			int drawBitMax = drawInt32 == 12 ? 16 : 32;
			int drawIntOffset = drawInt32 * 32;
			for (int drawBit = 0; drawBit < drawBitMax; drawBit++) {
				int x = drawIntOffset + drawBit;
				LCDColor drawColor = pd->graphics->getBitmapPixel(skyboxBitmap, x, y);
				if (drawColor == kColorWhite) {
					curDrawInt |= 1 << (31 - (drawBit & 31));
				}
			}
			skybox[y * 13 + drawInt32] = curDrawInt;
		}
	}
}

void gameInit(void)
{	
	// Textures
	loadTextures();

	// Drawables
	queuedDrawables = 0;
	enemies[0] = (struct Enemy){2 * CELL_SIZE, 2 * CELL_SIZE, 0, 0, 1};
	enemies[1] = (struct Enemy){8 * CELL_SIZE, 6 * CELL_SIZE, 0, 0, 1};
	enemies[2] = (struct Enemy){8 * CELL_SIZE, 7 * CELL_SIZE, 0, 0, 1};
	enemies[3] = (struct Enemy){8 * CELL_SIZE, 8 * CELL_SIZE, 0, 0, 1};
	enemies[4] = (struct Enemy){8 * CELL_SIZE, 9 * CELL_SIZE, 0, 0, 1};

	// Player
	playerX = 100;
	playerY = 120;
	playerRadius = 6;
	playerAngle = 0;
	playerTurnSpeed = 0;
	playerTurnMaxSpeed = 0.06f * TARGET_FPS;
	playerTurnInitialSpeed = 0.01f * TARGET_FPS;
	playerTurnAccel = 0.2f * TARGET_FPS;
	playerTurnDrag = 0.4f * TARGET_FPS;
	playerMaxSpeed = 1.4f * TARGET_FPS;
	playerInitialSpeed = 0.7f * TARGET_FPS;
	playerSpeed = 0;
	playerStrafeSpeed = 0;
	playerAccel = 6.0f * TARGET_FPS;
	playerDrag = 6.0f * TARGET_FPS;
	playerXVelocity = 0;
	playerYVelocity = 0;
	planeX = 0;
	planeY = -0.59f; // Related to fov somehow - not sure so I just tweaked it until it looked right
	dirX = 1;
	dirY = 0;

	// Animation
	weaponTargetDrawX = weaponBasePositionX;
	weaponTargetDrawY = weaponBasePositionY;
	weaponDrawX = weaponTargetDrawX;
	weaponDrawY = weaponTargetDrawY;
}

void drawMap(void)
{
	if (DRAW_MAP == 0) {
		return;
	}

	int cellSize = CELL_SIZE;
	int mapWidth = MAP_WIDTH;
	int mapHeight = MAP_HEIGHT;
	pd->graphics->fillRect(0, 0, mapWidth * cellSize, mapHeight * cellSize, kColorWhite);
	for (int y = 0; y < mapHeight; y++) {
		for (int x = 0; x < mapWidth; x++) {
			int cellValue = map[y * mapWidth + x];
			int drawX = x * cellSize;
			int drawY = y * cellSize;
			if (cellValue == 1) {
				pd->graphics->drawRect(drawX, drawY, cellSize, cellSize, kColorBlack);
			} else {
				pd->graphics->fillRect(drawX, drawY, cellSize, cellSize, kColorBlack);
			}
		}
	}
}

void drawPlayer(void)
{
	if (DRAW_MAP == 0) {
		return;
	}

	drawCircleAtPoint(playerX, playerY, playerRadius);

	int angleIndicatorX = (int)(playerX + playerXVelocity / 4);
	int angleIndicatorY = (int)(playerY + playerYVelocity / 4);
	pd->graphics->drawLine((int)playerX, (int)playerY, angleIndicatorX, angleIndicatorY, 2, kColorWhite);
}

void renderDrawables(float *zBuffer) {
	if (queuedDrawables <= 0) {
		return;
	}

	uint8_t *frameBufferU8 = (uint8_t *)pd->graphics->getFrame();
	int drawableOrder[queuedDrawables];
	float drawableDistances[queuedDrawables];
	for (int i = 0; i < queuedDrawables; i++) {
		drawableOrder[i] = i;
		Drawable curDrawable = drawables[i];
		float drawableDistance = 0;
		if (curDrawable.type != PROJECTILE) {
			drawableDistance = distanceSquared(playerX, playerY, curDrawable.x, curDrawable.y);
		}
		drawableDistances[i] = drawableDistance;
	}
	sortDrawables(drawableOrder, drawableDistances, queuedDrawables);

	//transform drawable with the inverse camera matrix
	// [ planeX   dirX ] -1                                       [ dirY      -dirX ]
	// [               ]       =  1/(planeX*dirY-dirX*planeY) *   [                 ]
	// [ planeY   dirY ]                                          [ -planeY  planeX ]
	float inverseDeterminant = 1.0f / (planeX * dirY - dirX * planeY);
	int fixedPointScreenHeight = SCREEN_HEIGHT << 7;
	for (int i = 0; i < queuedDrawables; i++) {
		Drawable curDrawable = drawables[drawableOrder[i]];
		uint8_t *drawableTexture;
		int anchorTopLeft = 0;
		int isProjectile = 0;
		if (curDrawable.type == PROJECTILE) {
			drawableTexture = projectileTexture;
			anchorTopLeft = 1;
			isProjectile = 1;
		} else if (curDrawable.type == ENEMY) {
			drawableTexture = enemyTexture;
		}
		
		float playerMapX = playerX / CELL_SIZE;
		float playerMapY = playerY / CELL_SIZE;
		float drawableMapX = curDrawable.x / CELL_SIZE;
		float drawableMapY = curDrawable.y / CELL_SIZE;
		float drawableX = drawableMapX - playerMapX;
		float drawableY = drawableMapY - playerMapY;

		float transformY = inverseDeterminant * (-planeY * drawableX + planeX * drawableY);
		if (transformY > 0) {
			float invTransformY = 1 / transformY;
			float transformX = inverseDeterminant * (dirX * drawableY - dirY * drawableX);
			float drawableDistance = transformY * CELL_SIZE;
			if (drawableDistance > FOG_MAX_DIST) {
				continue;
			}
	
			int drawableHeight = abs((int)((float)SCREEN_HEIGHT * invTransformY));
			int drawableWidth = drawableHeight;

			int halfDrawableWidth = (drawableWidth >> 1);
			int halfDrawableHeight = (drawableHeight >> 1);
			int screenXOffset = anchorTopLeft ? halfDrawableWidth / 2 : 0;
			int screenYOffset = anchorTopLeft ? halfDrawableHeight / 2 : 0;
			int drawableScreenX = (int)((float)(HALF_SCREEN_WIDTH) * (1 + transformX * invTransformY));

			int drawStartY = -halfDrawableHeight + HALF_SCREEN_HEIGHT;
			int drawEndY = halfDrawableHeight + HALF_SCREEN_HEIGHT;
			if (anchorTopLeft) {
				drawStartY += screenYOffset;
				drawEndY += screenYOffset;
			}
			if (drawStartY < 0) { drawStartY = 0; }
			if (drawEndY >= SCREEN_HEIGHT) { drawEndY = SCREEN_HEIGHT; }
			int fixedPointDrawableHeight = drawableHeight << 7;
	
			int drawStartX = -halfDrawableWidth + drawableScreenX;
			int drawEndX = halfDrawableWidth + drawableScreenX;
			if (anchorTopLeft) {
				drawStartX += screenXOffset;
				drawEndX += screenXOffset;
			}
			if (drawStartX < 0) { drawStartX = 0; }
			if (drawEndX >= SCREEN_WIDTH) { drawEndX = SCREEN_WIDTH; }
			
			int textureYCache[SCREEN_HEIGHT] = {[0 ... SCREEN_HEIGHT - 1] = -1};

			for (int drawX = drawStartX; drawX < drawEndX; drawX++) {
				if (isProjectile || drawableDistance < zBuffer[drawX]) {
					int fogShadeBit = drawX % 8;
					int textureX = (int)(((drawX - screenXOffset - (-(drawableWidth >> 1) + drawableScreenX)) << 8) * DRAWABLE_TEXTURE_SIZE / drawableWidth) >> 8;
					for (int drawY = drawStartY; drawY < drawEndY; drawY++) {

						int textureYIndex = textureYCache[drawY];
						if (textureYIndex == -1) {
							int d = ((drawY - screenYOffset) << 8) - fixedPointScreenHeight + fixedPointDrawableHeight; // 256 and 128 factors to avoid floats
							int textureY = ((d * DRAWABLE_TEXTURE_SIZE) / drawableHeight) >> 8;
							textureYIndex = DRAWABLE_TEXTURE_SIZE * textureY;
							textureYCache[drawY] = textureYIndex;
						}
						int inFog = 0;
						int fogRow = drawY % 8;
						int fogShade = 0;
						if (drawableDistance < FOG_MIN_DIST) {
							inFog = 0;
						} else {
							inFog = 1;
							int fogPatternIndex = (drawableDistance - FOG_MIN_DIST) / FOG_STEP;
							int *fogPattern = fogPatterns[fogPatternIndex];
							fogShade = fogPattern[fogRow];
						}
						uint8_t color = drawableTexture[textureYIndex + textureX];
						if (color == 1 && !(inFog && ((fogShade >> fogShadeBit) & 1))) {
							frameBufferU8[drawY * LCD_ROWSIZE + (drawX >> 3)] |= (1 << (7 - (drawX & 7)));
						} else if (color == 0) {
							frameBufferU8[drawY * LCD_ROWSIZE + (drawX >> 3)] &= ~(1 << (7 - (drawX & 7)));
						}
					}
				}
			}
		}
	}

	queuedDrawables = 0;
}

void addDrawableToRenderQueue(float x, float y, int type) {
	if (queuedDrawables >= MAX_DRAWABLE_COUNT) {
		return;
	}
	Drawable curDrawable = (struct Drawable){x, y, type};
	drawables[queuedDrawables] = curDrawable;
	queuedDrawables += 1;
}

// Cacheline is 32 bytes (256 bits) - struct is 16 bytes, so can fit 2 into one cacheline
typedef struct LineData {
	uint16_t lineHeight;
	uint16_t textureX;
	uint32_t lineOffset;
	float textureYStep; // 32 bits
	float textureYModifier; // 32 bits
} LineData;

void drawRays(void)
{
	int cellSize = CELL_SIZE;
	int cellSizeBit = 4;
	int mapWidth = MAP_WIDTH;
	int mapHeight = MAP_HEIGHT;

	LineData lineData[SCREEN_WIDTH];
	float zBuffer[SCREEN_WIDTH];

	int maxFieldDepth = 8;
	int rayCount = SCREEN_WIDTH;
	float fov = 66 * (PI / 180.0f);
	float rayInterval = fov / rayCount;
	float rayAngle = playerAngle - rayInterval * rayCount / 2;

	for (int ray = 0; ray < rayCount; ray++) {
		if (rayAngle < 0) {
			rayAngle += 2 * PI;
		} else if (rayAngle > 2 * PI) {
			rayAngle -= 2 * PI;
		}

		float rayX = playerX;
		float rayY = playerY;
		float xOffset = 0;
		float yOffset = 0;
		int depthOfField = 0;
		int maxDistance = 10000000;

		// === Check for horizontal lines ===
		float horizontalRayX, horizontalRayY;
		float horizontalDistance = maxDistance;
		float rayPerpendicularSlope = -1 / tanf(rayAngle);

		if (rayAngle == 0 || rayAngle == PI) {
			// Looking straight left or right
			depthOfField = maxFieldDepth;
			rayX = playerX;
			rayY = playerY;
		} else if (rayAngle > PI) {
			// Looking up
			rayY = (float)(((int)playerY >> cellSizeBit) << cellSizeBit) - 0.0001f;
			rayX = (playerY - rayY) * rayPerpendicularSlope + playerX;
			yOffset = -cellSize;
			xOffset = -yOffset * rayPerpendicularSlope;
		} else if (rayAngle < PI && rayAngle > 0) {
			// Looking down
			rayY = (((int)playerY >> cellSizeBit) << cellSizeBit) + cellSize;
			rayX = (playerY - rayY) * rayPerpendicularSlope + playerX;
			yOffset = cellSize;
			xOffset = -yOffset * rayPerpendicularSlope;
		}

		while (depthOfField < maxFieldDepth) {
			int mapX = (int)rayX >> cellSizeBit;
			int mapY = (int)rayY >> cellSizeBit;
			int mapIndex = mapY * mapWidth + mapX;
			if (mapIndex >= 0 && mapIndex < mapWidth * mapHeight && map[mapIndex] == 1) {
				depthOfField = maxFieldDepth; // Hit wall
				horizontalDistance = distanceSquared(playerX, playerY, rayX, rayY);
				horizontalRayX = rayX;
				horizontalRayY = rayY;
			} else {
				rayX += xOffset;
				rayY += yOffset;
				depthOfField += 1;
			}
		}

		// === Check for vertical lines ===
		depthOfField = 0;
		float verticalRayX, verticalRayY;
		float verticalDistance = maxDistance;
		float rayNegSlope = -tanf(rayAngle);

		if (rayAngle == P2 || rayAngle == P3) {
			// Looking straight up or down
			depthOfField = maxFieldDepth;
			rayX = playerX;
			rayY = playerY;
		} else if (rayAngle > P2 && rayAngle < P3) {
			// Looking left
			rayX = (float)(((int)playerX >> cellSizeBit) << cellSizeBit) - 0.0001f;
			rayY = (playerX - rayX) * rayNegSlope + playerY;
			xOffset = -cellSize;
			yOffset = -xOffset * rayNegSlope;
		} else if (rayAngle < P2 || rayAngle > P3) {
			// Looking right
			rayX = (((int)playerX >> cellSizeBit) << cellSizeBit) + cellSize;
			rayY = (playerX - rayX) * rayNegSlope + playerY;
			xOffset = cellSize;
			yOffset = -xOffset * rayNegSlope;
		}

		while (depthOfField < maxFieldDepth) {
			int mapX = (int)rayX >> cellSizeBit;
			int mapY = (int)rayY >> cellSizeBit;
			int mapIndex = mapY * mapWidth + mapX;
			if (mapIndex >= 0 && mapIndex < mapWidth * mapHeight && map[mapIndex] == 1) {
				depthOfField = maxFieldDepth; // Hit wall
				verticalDistance = distanceSquared(playerX, playerY, rayX, rayY);
				verticalRayX = rayX;
				verticalRayY = rayY;
			} else {
				rayX += xOffset;
				rayY += yOffset;
				depthOfField += 1;
			}
		}

		LineData curLineData;
		float rayDistance = maxDistance;
		int isShaded = 0;
		if (verticalDistance < horizontalDistance) {
			rayX = verticalRayX;
			rayY = verticalRayY;
			rayDistance = fastSqrt(verticalDistance);
		} else if (horizontalDistance < verticalDistance) {
			rayX = horizontalRayX;
			rayY = horizontalRayY;
			rayDistance = fastSqrt(horizontalDistance);
			isShaded = 1;
		}

		if (verticalDistance > (maxDistance - 1) && horizontalDistance > (maxDistance - 1)) {
			curLineData.lineHeight = 0;
			curLineData.lineOffset = HALF_SCREEN_HEIGHT;
			lineData[ray] = curLineData;
			zBuffer[ray] = rayDistance;

			rayAngle += rayInterval;
			continue;
		}

		if (DRAW_MAP != 0) {
			pd->graphics->drawLine((int)playerX, (int)playerY, (int)rayX, (int)rayY, 1, kColorWhite);
		}

		// === Draw 3D Walls ===

		// Fix fisheye
		float offsetAngle = playerAngle - rayAngle;
		if (offsetAngle < 0) {
			offsetAngle += 2 * PI;
		} else if (offsetAngle > 2 * PI) {
			offsetAngle -= 2 * PI;
		}
		rayDistance = rayDistance * cosf(offsetAngle);

		// Get wall slice draw height
		int textureSize = 16;
		float lineHeight = (cellSize * SCREEN_HEIGHT) / rayDistance;
		curLineData.lineHeight = (int)lineHeight;

		float textureYStep = textureSize / lineHeight;
		float textureYOffset = 0;
		if (lineHeight > SCREEN_HEIGHT) {
			textureYOffset = (lineHeight - SCREEN_HEIGHT) / 2.0f;
			lineHeight = SCREEN_HEIGHT;
		}
		float lineOffset = (SCREEN_HEIGHT >> 1) - lineHeight / 2;
		curLineData.textureYStep = textureYStep;
		curLineData.textureYModifier = textureYOffset * textureYStep - textureYStep * lineOffset;
		curLineData.lineOffset = (int)lineOffset;

		int textureX;
		if (isShaded == 1) {
			textureX = ((int)rayX >> 1) % textureSize;
			if (rayAngle < PI) {
				textureX = textureSize - textureX - 1;
			}
			textureX += 16;
		} else {
			textureX = ((int)rayY >> 1) % textureSize;
			if (rayAngle > P2 && rayAngle < P3) {
				textureX = textureSize - textureX - 1;
			}
		}
		curLineData.textureX = textureX;
		lineData[ray] = curLineData;
		zBuffer[ray] = rayDistance;

		rayAngle += rayInterval;
	}

	// Draw to screen
	float rayDirX0 = dirX + planeX;
	float rayDirY0 = dirY + planeY;
	float rayDirX1 = dirX - planeX;
	float rayDirY1 = dirY - planeY;
	float ceilingStepXModifier = (rayDirX1 - rayDirX0) / SCREEN_WIDTH;
	float ceilingStepYModifier = (rayDirY1 - rayDirY0) / SCREEN_WIDTH;
	float playerMapX = playerX / CELL_SIZE;
	float playerMapY = playerY / CELL_SIZE;

	int fogOffset = ((int)(playerAngle * (180 / PI)) & ~1) % 8;

	int frameBuffer32BitIntsPerRow = ceilf((float)LCD_COLUMNS / 32);
	int maxDrawInt = ceilf((float)SCREEN_WIDTH / 32);
	int bitRemainder = SCREEN_WIDTH % 32;
	uint32_t *frameBuffer = (uint32_t *)pd->graphics->getFrame();
	for (int drawY = 0; drawY < SCREEN_HEIGHT; drawY++) {
		// Current y position compared to the center of the screen (the horizon)
		int p = HALF_SCREEN_HEIGHT - drawY;
		// Horizontal distance from the camera to the floor for the current row.
		// 0.5 is the z position exactly in the middle between floor and ceiling.
		float rowDistance = (float)HALF_SCREEN_HEIGHT / p;

		int inFog = 0;
		float ceilingDistance = rowDistance * CELL_SIZE;
		int skipCeilingFloor = ceilingDistance > FOG_MAX_DIST;
		int fogRow = drawY % 8;
		int fogShade = 0;
		if (ceilingDistance < FOG_MIN_DIST) {
			inFog = 0;
		} else if (ceilingDistance <= FOG_MAX_DIST) {
			inFog = 1;
			int fogPatternIndex = (ceilingDistance - FOG_MIN_DIST) / FOG_STEP;
			int *fogPattern = fogPatterns[fogPatternIndex];
			fogShade = fogPattern[fogRow];
		}

		// calculate the real world step vector we have to add for each x (parallel to camera plane)
		// adding step by step avoids multiplications with a weight in the inner loop
		float ceilingStepX = rowDistance * ceilingStepXModifier;
		float ceilingStepY = rowDistance * ceilingStepYModifier;
		
		// real world coordinates of the leftmost column. This will be updated as we step to the right.
		float ceilingX = playerMapX + rowDistance * rayDirX0 - ceilingStepX;
		float ceilingY = playerMapY + rowDistance * rayDirY0 - ceilingStepY;

		for (int drawInt32 = 0; drawInt32 < maxDrawInt; drawInt32++) {
			int frameBufferIndex = drawY * frameBuffer32BitIntsPerRow + drawInt32;
			uint32_t curDrawInt = frameBuffer[frameBufferIndex];
			// if (drawY < SCREEN_HEIGHT / 2) {
			// 	curDrawInt = skybox[frameBufferIndex];
			// }
			int drawBitMax = drawInt32 == (maxDrawInt - 1) ? bitRemainder : 32;
			int drawIntOffset = drawInt32 * 32;
			for (int drawBit = 0; drawBit < drawBitMax; drawBit++) {
				uint8_t drawColor;
				int drawX = drawIntOffset + drawBit;
				int fogShadeBit = (drawX + fogOffset) % 8;

				LineData curLineData = lineData[drawX];
				int lineHeight = curLineData.lineHeight;
				int lineOffset = curLineData.lineOffset;

				float textureYModifier = curLineData.textureYModifier;
				float textureYStep = curLineData.textureYStep;
				curLineData.textureYModifier = textureYModifier + textureYStep;
				lineData[drawX] = curLineData;

				ceilingX += ceilingStepX;
				ceilingY += ceilingStepY;
				if (drawY < lineOffset) {
					int floorDrawColor = 0;
					if (skipCeilingFloor || (inFog & ((fogShade >> fogShadeBit) & 1))) {
						drawColor = 0;
						floorDrawColor = 0;
					} else {
						// the cell coord is simply got from the integer parts of floorX and floorY
						int cellX = (int)(ceilingX);
						int cellY = (int)(ceilingY);
				
						// get the texture coordinate from the fractional part
						int textureX = (int)(CELL_SIZE * (ceilingX - cellX)) & (CELL_SIZE - 1);
						int textureY = (int)(CELL_SIZE * (ceilingY - cellY)) & (CELL_SIZE - 1);
	
						// Draw ceiling
						int textureIndex = textureX + (textureY << 4);
						drawColor = ceilingTexture[textureIndex];
	
						// Draw floor
						floorDrawColor = floorTexture[textureIndex];
					}

					int floorFrameBufferIndex = (SCREEN_HEIGHT - drawY - 1) * frameBuffer32BitIntsPerRow + drawInt32;
					uint32_t floorFrameBufferInt = frameBuffer[floorFrameBufferIndex];
					if (floorDrawColor == 1) {
						floorFrameBufferInt |= 1 << (31 - (drawBit & 31));
					} else {
						floorFrameBufferInt &= ~(1 << (31 - (drawBit & 31)));
					}
					frameBuffer[floorFrameBufferIndex] = floorFrameBufferInt;
				} else if (drawY > lineHeight + lineOffset + 1) {
					continue;
				} else {
					float wallDistance = zBuffer[drawX];
					if (wallDistance > FOG_MAX_DIST) {
						curDrawInt &= ~(1 << (31 - (drawBit & 31)));
						continue;
					}
					int textureX = curLineData.textureX;
					if (textureX >= 16) {
						textureX -= 16;
						if ((drawX + drawY) % 2 == 0) {
							curDrawInt &= ~(1 << (31 - (drawBit & 31)));
							continue;
						}
					}
	
					int textureY = (int)(textureYModifier);
					drawColor = wallTexture[textureX + (textureY << 4)];

					int fogShade = 0;
					if (wallDistance < FOG_MIN_DIST) {
						fogShade = 0;
					} else if (wallDistance >= FOG_MIN_DIST) {
						int fogPatternIndex = (int)((float)(wallDistance - FOG_MIN_DIST) / (float)FOG_STEP);
						int *fogPattern = fogPatterns[fogPatternIndex];
						fogShade = fogPattern[fogRow];
					}

					if ((fogShade >> fogShadeBit) & 1) {
						drawColor = 0;
					}
				}

				if (drawColor == 1) {
					curDrawInt |= 1 << (31 - (drawBit & 31));
				} else {
					curDrawInt &= ~(1 << (31 - (drawBit & 31)));
				}
			}
			curDrawInt = swap(curDrawInt);
			frameBuffer[frameBufferIndex] = curDrawInt;
		}
	}

	renderDrawables(zBuffer);

	pd->graphics->markUpdatedRows(0, SCREEN_HEIGHT - 1);
}

void drawWeapon(float deltaTime) {
	float lerpSpeed = 0.2f;
	weaponDrawX = lerp(weaponDrawX, weaponTargetDrawX, lerpSpeed);
	weaponDrawY = lerp(weaponDrawY, weaponTargetDrawY, lerpSpeed);
	if (weaponCurCooldown > 0) {
		weaponCurCooldown -= deltaTime;
		if (weaponCurCooldown <= 0) {
			weaponCurCooldown = 0;
		}
		pd->graphics->drawBitmap(handOpenBitmap, weaponDrawX, weaponDrawY, kBitmapUnflipped);
	} else {
		pd->graphics->drawBitmap(handBitmap, weaponDrawX, weaponDrawY, kBitmapUnflipped);
	}
}

void fireProjectile(void) {
	float projectileSpeed = 5.0f * TARGET_FPS;
	for (int i = 0; i < MAX_PROJECTILE_COUNT; i++) {
		if (!projectiles[i].active) {
			projectiles[i].x = playerX + cosf(playerAngle) * CELL_SIZE;
			projectiles[i].y = playerY + sinf(playerAngle) * CELL_SIZE;
			projectiles[i].direction = playerAngle;
			projectiles[i].speed = projectileSpeed;
			projectiles[i].active = 1;
			break;
		}
	}
}

void updateProjectiles(float deltaTime) {
	for (int i = 0; i < MAX_PROJECTILE_COUNT; i++) {
		Projectile curProjectile = projectiles[i];
		if (!curProjectile.active) {
			continue;
		}

		float xDirection = cosf(curProjectile.direction);
		float yDirection = sinf(curProjectile.direction);
		int drawableMapX = (int)((curProjectile.x - xDirection * CELL_SIZE) / CELL_SIZE);
		int drawableMapY = (int)((curProjectile.y - yDirection * CELL_SIZE) / CELL_SIZE);
		int drawableMapIndex = drawableMapY * MAP_WIDTH + drawableMapX;
		if ((drawableMapIndex >= MAP_WIDTH * MAP_HEIGHT) || (map[drawableMapIndex] != 0)) {
			curProjectile.active = 0;
		}

		float xVelocity = xDirection * curProjectile.speed;
		float yVelocity = yDirection * curProjectile.speed;
		curProjectile.x += xVelocity * deltaTime;
		curProjectile.y += yVelocity * deltaTime;

		projectiles[i] = curProjectile;

		addDrawableToRenderQueue(curProjectile.x, curProjectile.y, PROJECTILE);
	}
}

void updateEnemies(float deltaTime) {
	for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
		Enemy curEnemy = enemies[i];
		if (!curEnemy.active) {
			continue;
		}

		addDrawableToRenderQueue(curEnemy.x, curEnemy.y, ENEMY);
	}
}

void handleInput(float deltaTime)
{
	int cellSize = CELL_SIZE;
	int mapWidth = MAP_WIDTH;

	PDButtons current;
	PDButtons pushed;
	pd->system->getButtonState(&current, &pushed, NULL);

	if (pushed & kButtonA) {
		if (weaponCurCooldown <= 0) {
			weaponCurCooldown = weaponCooldown;
			fireProjectile();
		}
	}

	if (pushed & kButtonB) {
		movementToggled ^= 1;
	}

	int bButtonPressed = (current & kButtonB) != 0;
	if (toggleMovementSetting) {
		bButtonPressed = movementToggled;
	}
	int rotationEnabled = (defaultStrafeSetting == bButtonPressed);
	int strafeEnabled = (defaultStrafeSetting != bButtonPressed);

	if ((current & kButtonLeft) && rotationEnabled) {
		if (playerTurnSpeed > -playerTurnInitialSpeed) {
			playerTurnSpeed = -playerTurnInitialSpeed;
		}
		playerTurnSpeed -= playerTurnAccel * deltaTime; 
		if (playerTurnSpeed < -playerTurnMaxSpeed) {
			playerTurnSpeed = -playerTurnMaxSpeed;
		}
	} else if ((current & kButtonRight) && rotationEnabled) {
		if (playerTurnSpeed < playerTurnInitialSpeed) {
			playerTurnSpeed = playerTurnInitialSpeed;
		}
		playerTurnSpeed += playerTurnAccel * deltaTime; 
		if (playerTurnSpeed > playerTurnMaxSpeed) {
			playerTurnSpeed = playerTurnMaxSpeed;
		}
	} else {
		if (playerTurnSpeed < 0.5f && playerTurnSpeed > -0.5f) {
			playerTurnSpeed = 0;
		} else if (playerTurnSpeed > 0) {
			playerTurnSpeed -= playerTurnDrag * deltaTime;
		} else if (playerTurnSpeed < 0) {
			playerTurnSpeed += playerTurnDrag * deltaTime;
		}
	}

	playerAngle += playerTurnSpeed * deltaTime;
	if (playerAngle < 0) {
		playerAngle += 2 * PI;
	} else if (playerAngle > 2 * PI) {
		playerAngle -= 2 * PI;
	}

	float adjustedTurnSpeed = playerTurnSpeed * deltaTime;
	float oldDirX = dirX;
	dirX = dirX * cosf(adjustedTurnSpeed) - dirY * sinf(adjustedTurnSpeed);
	dirY = oldDirX * sinf(adjustedTurnSpeed) + dirY * cosf(adjustedTurnSpeed);
	float oldPlaneX = planeX;
	planeX = planeX * cosf(adjustedTurnSpeed) - planeY * sinf(adjustedTurnSpeed);
	planeY = oldPlaneX * sinf(adjustedTurnSpeed) + planeY * cosf(adjustedTurnSpeed);

	if ((current & kButtonRight) && strafeEnabled) {
		if (playerStrafeSpeed < playerInitialSpeed) {
			playerStrafeSpeed = playerInitialSpeed;
		}
		playerStrafeSpeed += playerAccel * deltaTime;
		if (playerStrafeSpeed > playerMaxSpeed) {
			playerStrafeSpeed = playerMaxSpeed;
		}
	} else if ((current & kButtonLeft) && strafeEnabled) {
		if (playerStrafeSpeed > -playerInitialSpeed) {
			playerStrafeSpeed = -playerInitialSpeed;
		}
		playerStrafeSpeed -= playerAccel * deltaTime;
		if (playerStrafeSpeed < -playerMaxSpeed) {
			playerStrafeSpeed = -playerMaxSpeed;
		}
	} else {
		float dragCutoff = 0.1f * playerMaxSpeed;
		if (playerStrafeSpeed < dragCutoff && playerStrafeSpeed > -dragCutoff) {
			playerStrafeSpeed = 0;
		} else if (playerStrafeSpeed > 0) {
			playerStrafeSpeed -= playerDrag * deltaTime;
		} else if (playerStrafeSpeed < 0) {
			playerStrafeSpeed += playerDrag * deltaTime;
		}
	}

	if (current & kButtonUp) {
		if (playerSpeed < playerInitialSpeed) {
			playerSpeed = playerInitialSpeed;
		}
		playerSpeed += playerAccel * deltaTime;
		if (playerSpeed > playerMaxSpeed) {
			playerSpeed = playerMaxSpeed;
		}
	} else if (current & kButtonDown) {
		if (playerSpeed > -playerInitialSpeed) {
			playerSpeed = -playerInitialSpeed;
		}
		playerSpeed -= playerAccel * deltaTime;
		if (playerSpeed < -playerMaxSpeed) {
			playerSpeed = -playerMaxSpeed;
		}
	} else {
		float dragCutoff = 0.1f * playerMaxSpeed;
		if (playerSpeed < dragCutoff && playerSpeed > -dragCutoff) {
			playerSpeed = 0;
		} else if (playerSpeed > 0) {
			playerSpeed -= playerDrag * deltaTime;
		} else if (playerSpeed < 0) {
			playerSpeed += playerDrag * deltaTime;
		}
	}

	float combinedMagnitude = sqrtf((playerSpeed * playerSpeed) + (playerStrafeSpeed * playerStrafeSpeed));
	float normalizedSpeed = 0;
	float normalizedStrafeSpeed = 0;

	if (combinedMagnitude != 0) {
		normalizedSpeed = playerSpeed * (fabsf(playerSpeed) / combinedMagnitude);
		normalizedStrafeSpeed = playerStrafeSpeed * (fabsf(playerStrafeSpeed) / combinedMagnitude);
	}

	playerXVelocity = cosf(playerAngle) * normalizedSpeed + cosf(playerAngle + PI / 2) * normalizedStrafeSpeed;
	playerYVelocity = sinf(playerAngle) * normalizedSpeed + sinf(playerAngle + PI / 2) * normalizedStrafeSpeed;

	int wallCollisionDist = 10;
	int xOffset = 0;
	if (playerXVelocity < 0) {
		xOffset = -wallCollisionDist;
	} else {
		xOffset = wallCollisionDist;
	}

	int yOffset = 0;
	if (playerYVelocity < 0) {
		yOffset = -wallCollisionDist;
	} else {
		yOffset = wallCollisionDist;
	}

	// Player coordinates -> Map coordinates
	int xIndex = playerX / cellSize;
	int lookAheadXIndex = (playerX + xOffset) / cellSize;
	int yIndex = playerY / cellSize;
	int lookAheadYIndex = (playerY + yOffset) / cellSize;

	if (map[yIndex * mapWidth + lookAheadXIndex] == 0) {
		playerX += playerXVelocity * deltaTime;
	}

	if (map[lookAheadYIndex * mapWidth + xIndex] == 0) {
		playerY += playerYVelocity * deltaTime;
	}

	// Weapon drawing
	if (current & kButtonLeft) {
		weaponTargetDrawX = weaponBasePositionX - weaponXOffset;
	} else if (current & kButtonRight) {
		weaponTargetDrawX = weaponBasePositionX + weaponXOffset;
	} else {
		weaponTargetDrawX = weaponBasePositionX;
	}

	if ((current & kButtonUp) || (current & kButtonDown)) {
		weaponBobCounter += weaponBobSpeed;
		if (weaponBobCounter > 2 * PI) {
			weaponBobCounter -= 2 * PI;
		}

		weaponTargetDrawY = weaponBasePositionY - weaponYOffset * sinf(weaponBobCounter);
	} else {
		weaponBobCounter = 0;
		weaponTargetDrawY = weaponBasePositionY;
	}

	if (weaponCurCooldown > 0) {
		weaponTargetDrawX = weaponBasePositionX;
		weaponTargetDrawY = weaponBasePositionY + weaponActiveYOffset;
	}
}

int gameUpdate(void *userdata)
{
	float deltaTime = pd->system->getElapsedTime();
	pd->system->resetElapsedTime();

	pd->display->setRefreshRate(0);

	// pd->drawable->updateAndDrawDrawables();
	// pd->graphics->clear(kColorBlack);

	handleInput(deltaTime);
	updateEnemies(deltaTime);
	updateProjectiles(deltaTime);
	drawMap();
	drawPlayer();
	drawRays();
	drawWeapon(deltaTime);

	pd->system->drawFPS(0, 0);

	pd->lua->pushNil();
	return 1;
}

PDMenuItem* toggleDefaultStrafeMenuItem;
PDMenuItem* toggleMovementMenuItem;

void toggleDefaultStrafeCallback(void* userdata) {
	int value = pd->system->getMenuItemValue(toggleDefaultStrafeMenuItem);
	defaultStrafeSetting = value;
}

void toggleMovementCallback(void* userdata) {
	int value = pd->system->getMenuItemValue(toggleMovementMenuItem);
	toggleMovementSetting = value;
	movementToggled = 0;
}

int eventHandler(PlaydateAPI *playdate, PDSystemEvent event, uint32_t arg)
{
	if (event == kEventInit) {
		pd = playdate;
		pd->system->resetElapsedTime();
		pd->display->setScale(400 / SCREEN_WIDTH);

		const char *movementOptions[] = {"rotate", "strafe"};
		toggleDefaultStrafeMenuItem = pd->system->addOptionsMenuItem("Default", movementOptions, 2, toggleDefaultStrafeCallback, NULL);

		toggleMovementMenuItem = pd->system->addCheckmarkMenuItem("Toggle", 0, toggleMovementCallback, NULL);

		gameInit();
		pd->system->setUpdateCallback(gameUpdate, NULL);
	}
	// if (event == kEventInitLua) {
	// 	pd = playdate;
	// 	pd->system->resetElapsedTime();

	// 	gameInit();

	// 	pd->lua->addFunction(gameUpdate, "game.update", NULL);
	// }

	return 0;
}
