#define TARGET_EXTENSION 1

#include <stdio.h>
#include <math.h>
#include <float.h>

#define DRAW_MAP 0

#define PI 3.141592f
#define P2 PI / 2
#define P3 3 * PI / 2
#define TARGET_FPS 30
#define SCREEN_WIDTH 400
#define SCREEN_HEIGHT 240

#include "pd_api.h"
static PlaydateAPI *pd = NULL;

inline float fastRsqrt(float x)
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
inline float fastRcp(float x) { x = fastRsqrt(x); return x * x; }
inline float fastDiv(float x, float y) { return x * fastRcp(y); }
inline float fastSqrt(float x) { return x * fastRsqrt(x); }

static void drawCircleAtPoint(float x, float y, int radius)
{
	int drawX = (int)x - radius;
	int drawY = (int)y - radius;
	int width = radius * 2;
	int height = radius * 2;
	pd->graphics->drawEllipse(drawX, drawY, width, height, 1, 0, 0, kColorWhite);
}

static float distanceSquared(float x1, float y1, float x2, float y2)
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

LCDBitmap *brickTexture;
// static LCDPattern shadePattern = {
// 	// Bitmap
// 	0b10101010,
// 	0b01010101,
// 	0b10101010,
// 	0b01010101,
// 	0b10101010,
// 	0b01010101,
// 	0b10101010,
// 	0b01010101,

// 	// Mask
// 	0b11111111,
// 	0b11111111,
// 	0b11111111,
// 	0b11111111,
// 	0b11111111,
// 	0b11111111,
// 	0b11111111,
// 	0b11111111,
// };
// static LCDPattern whiteShadePattern = {
// 	// Bitmap
// 	0b01010101,
// 	0b10101010,
// 	0b01010101,
// 	0b10101010,
// 	0b01010101,
// 	0b10101010,
// 	0b01010101,
// 	0b10101010,	

// 	// Mask
// 	0b11111111,
// 	0b11111111,
// 	0b11111111,
// 	0b11111111,
// 	0b11111111,
// 	0b11111111,
// 	0b11111111,
// 	0b11111111,
// };

static int cellSize = 20;
static int mapWidth = 10;
static int mapHeight = 12;
static int map[] =
{
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 0, 0, 1, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 1, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 1, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 1, 1, 1, 1, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 1, 1, 1, 0, 0, 1, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 1, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 1, 0, 0, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1
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

static float playerX, playerY;
static int playerRadius;
static float playerAngle;
static float playerTurnSpeed;
static float playerMaxSpeed;
static float playerXVelocity, playerYVelocity;

static void gameInit(void)
{
	brickTexture = pd->graphics->loadBitmap("images/brickTexture", NULL);
	playerX = 100;
	playerY = 120;
	playerRadius = 6;
	playerAngle = 0;
	playerTurnSpeed = 0.1f * TARGET_FPS;
	playerMaxSpeed = 2.0f * TARGET_FPS;
	playerXVelocity = cosf(playerAngle) * playerMaxSpeed;
	playerYVelocity = sinf(playerAngle) * playerMaxSpeed;
}

static void drawMap(void)
{
	if (DRAW_MAP == 0) {
		return;
	}

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

static void drawPlayer(void)
{
	if (DRAW_MAP == 0) {
		return;
	}

	drawCircleAtPoint(playerX, playerY, playerRadius);

	int angleIndicatorX = (int)(playerX + playerXVelocity / 4);
	int angleIndicatorY = (int)(playerY + playerYVelocity / 4);
	pd->graphics->drawLine((int)playerX, (int)playerY, angleIndicatorX, angleIndicatorY, 2, kColorWhite);
}

static float lineHeights[SCREEN_WIDTH];
static float lineOffsets[SCREEN_WIDTH];
static float lineTextureX[SCREEN_WIDTH];
static float lineIsShaded[SCREEN_WIDTH];

static void drawRays(void)
{
	int maxFieldDepth = 10;
	int screenWidth = 400;
	int screenHeight = 240;
	// int renderStartX = 400 - screenWidth;
	int rayCount = screenWidth;
	// int rayWidth = screenWidth / rayCount;
	float fov = 70 * (PI / 180.0f);
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

		// === Check for horizontal lines ===
		float horizontalRayX, horizontalRayY;
		float horizontalDistance = 1000000;
		float rayPerpendicularSlope = -1 / tanf(rayAngle);

		if (rayAngle == 0 || rayAngle == PI) {
			// Looking straight left or right
			depthOfField = maxFieldDepth;
			rayX = playerX;
			rayY = playerY;
		} else if (rayAngle > PI) {
			// Looking up
			rayY = floorf(playerY / cellSize) * cellSize - 0.01f;
			rayX = (playerY - rayY) * rayPerpendicularSlope + playerX;
			yOffset = -cellSize;
			xOffset = -yOffset * rayPerpendicularSlope;
		} else if (rayAngle < PI && rayAngle > 0) {
			// Looking down
			rayY = floorf(playerY / cellSize) * cellSize + cellSize;
			rayX = (playerY - rayY) * rayPerpendicularSlope + playerX;
			yOffset = cellSize;
			xOffset = -yOffset * rayPerpendicularSlope;
		}

		while (depthOfField < maxFieldDepth) {
			int mapX = (int)(rayX / cellSize);
			int mapY = (int)(rayY / cellSize);
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
		float verticalDistance = 1000000;
		float rayNegSlope = -tanf(rayAngle);

		if (rayAngle == P2 || rayAngle == P3) {
			// Looking straight up or down
			depthOfField = maxFieldDepth;
			rayX = playerX;
			rayY = playerY;
		} else if (rayAngle > P2 && rayAngle < P3) {
			// Looking left
			rayX = floorf(playerX / cellSize) * cellSize - 0.01f;
			rayY = (playerX - rayX) * rayNegSlope + playerY;
			xOffset = -cellSize;
			yOffset = -xOffset * rayNegSlope;
		} else if (rayAngle < P2 || rayAngle > P3) {
			// Looking right
			rayX = floorf(playerX / cellSize) * cellSize + cellSize;
			rayY = (playerX - rayX) * rayNegSlope + playerY;
			xOffset = cellSize;
			yOffset = -xOffset * rayNegSlope;
		}

		while (depthOfField < maxFieldDepth) {
			int mapX = (int)(rayX / cellSize);
			int mapY = (int)(rayY / cellSize);
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

		float rayDistance = 0;
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
		float lineHeight = (cellSize * screenHeight) / rayDistance;
		lineHeights[ray] = lineHeight;

		if (lineHeight > screenHeight) {
			lineHeight = screenHeight;
		}

		float lineOffset = screenHeight / 2 - lineHeight / 2;
		lineOffsets[ray] = lineOffset;

		float textureX = (int)(rayX / 2.0f) % textureSize;
		if (isShaded == 1) {
			textureX = (int)(rayX / 2.0f) % textureSize;
			if (rayAngle < PI) {
				textureX = textureSize - textureX - 1;
			}
		} else {
			textureX = (int)(rayY / 2.0f) % textureSize;
			if (rayAngle > P2 && rayAngle < P3) {
				textureX = textureSize - textureX - 1;
			}
		}
		lineTextureX[ray] = textureX;
		lineIsShaded[ray] = isShaded;

		rayAngle += rayInterval;
	}

	uint32_t *frameBuffer = (uint32_t*)pd->graphics->getFrame();
	for (int drawY = 0; drawY < SCREEN_HEIGHT; drawY++)
	{
		for (int drawInt32 = 0; drawInt32 < 13; drawInt32++)
		{
			uint32_t curDrawInt = 0;
			for (int drawBit = 0; drawBit < 32; drawBit++)
			{
				int textureSize = 16;
				int drawX = drawInt32 * 32 + drawBit;
				float lineHeight = lineHeights[drawX];
				float lineOffset = lineOffsets[drawX];
				float textureYStep = textureSize / lineHeight;
				float textureYOffset = 0;
				if (lineHeight > screenHeight) {
					textureYOffset = (lineHeight - screenHeight) / 2.0f;
					lineHeight = screenHeight;
				}
				if (drawY < lineOffset || drawY > lineHeight + lineOffset) {
					continue;
				}
				float textureX = lineTextureX[drawX];
				float isShaded = lineIsShaded[drawX];

				float textureY = textureYOffset * textureYStep + textureYStep * (drawY - lineOffset);

				LCDColor drawColor = pd->graphics->getBitmapPixel(brickTexture, textureX, textureY);
				if (isShaded == 1) {
					if (drawColor == kColorWhite) {
						if (drawX % 2 == 0 || drawY % 2 == 1) {
							drawColor = kColorBlack;
						}
					}
				}

				if (drawColor == kColorWhite)
				{
					curDrawInt |= 1 << (31 - (drawBit & 31));
				}
				// pd->graphics->setPixel(drawX, drawY, drawColor);
			}
			curDrawInt = swap(curDrawInt);
			frameBuffer[drawY * 13 + drawInt32] = curDrawInt;
		}
	}

	pd->graphics->markUpdatedRows(0, LCD_ROWS - 1);
}

static void handleInput(float deltaTime)
{
	// char str_num[50];
	// sprintf(str_num, "%f", deltaTime);
	// pd->system->logToConsole(str_num);

	PDButtons current;
	pd->system->getButtonState(&current, NULL, NULL);

	if (current & kButtonLeft) {
		playerAngle -= playerTurnSpeed * deltaTime;
		if (playerAngle < 0) {
			playerAngle += 2 * PI;
		}
		playerXVelocity = cosf(playerAngle) * playerMaxSpeed;
		playerYVelocity = sinf(playerAngle) * playerMaxSpeed;
	} else if (current & kButtonRight) {
		playerAngle += playerTurnSpeed * deltaTime;
		if (playerAngle > 2 * PI) {
			playerAngle -= 2 * PI;
		}
		playerXVelocity = cosf(playerAngle) * playerMaxSpeed;
		playerYVelocity = sinf(playerAngle) * playerMaxSpeed;
	}

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
	int lookBehindXIndex = (playerX - xOffset) / cellSize;
	int yIndex = playerY / cellSize;
	int lookAheadYIndex = (playerY + yOffset) / cellSize;
	int lookBehindYIndex = (playerY - yOffset) / cellSize;

	if (current & kButtonUp) {
		if (map[yIndex * mapWidth + lookAheadXIndex] == 0) {
			playerX += playerXVelocity * deltaTime;
		}

		if (map[lookAheadYIndex * mapWidth + xIndex] == 0) {
			playerY += playerYVelocity * deltaTime;
		}
	} else if (current & kButtonDown) {
		if (map[yIndex * mapWidth + lookBehindXIndex] == 0) {
			playerX -= playerXVelocity * deltaTime;
		}

		if (map[lookBehindYIndex * mapWidth + xIndex] == 0) {
			playerY -= playerYVelocity * deltaTime;
		}
	}
}

static int gameUpdate(void* userdata)
{
	float deltaTime = pd->system->getElapsedTime();
	pd->system->resetElapsedTime();

	// pd->sprite->updateAndDrawSprites();
	// pd->graphics->clear(kColorBlack);

	handleInput(deltaTime);
	drawMap();
	drawPlayer();
	drawRays();

	pd->system->drawFPS(0, 0);

	pd->lua->pushNil();
	return 1;
}

int eventHandler(PlaydateAPI *playdate, PDSystemEvent event, uint32_t arg)
{
	if (event == kEventInit) {
		pd = playdate;
		pd->system->resetElapsedTime();
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
