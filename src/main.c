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
#define CELL_SIZE 16
#define MAP_WIDTH 10
#define MAP_HEIGHT 12

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

void drawCircleAtPoint(float x, float y, int radius)
{
	int drawX = (int)x - radius;
	int drawY = (int)y - radius;
	int width = radius * 2;
	int height = radius * 2;
	pd->graphics->drawEllipse(drawX, drawY, width, height, 1, 0, 0, kColorWhite);
}

LCDSolidColor brickTexture[256];

// LCDPattern shadePattern = {
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
// LCDPattern whiteShadePattern = {
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

int map[] =
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

float playerX, playerY;
int playerRadius;
float playerAngle;
float playerTurnSpeed;
float playerMaxSpeed;
float playerXVelocity, playerYVelocity;

void loadTextures(void)
{
	LCDBitmap* brickTextureBitmap = pd->graphics->loadBitmap("images/brickTexture", NULL);
	for (int x = 0; x < 16; x++) {
		for (int y = 0; y < 16; y++) {
			LCDColor drawColor = pd->graphics->getBitmapPixel(brickTextureBitmap, x, y);
			brickTexture[y * 16 + x] = drawColor;
		}
	}
}

void gameInit(void)
{
	loadTextures();
	playerX = 100;
	playerY = 120;
	playerRadius = 6;
	playerAngle = 0;
	playerTurnSpeed = 0.1f * TARGET_FPS;
	playerMaxSpeed = 2.0f * TARGET_FPS;
	playerXVelocity = cosf(playerAngle) * playerMaxSpeed;
	playerYVelocity = sinf(playerAngle) * playerMaxSpeed;
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

void drawRays(void)
{
	int cellSize = CELL_SIZE;
	int cellSizeBit = 4;
	int mapWidth = MAP_WIDTH;
	int mapHeight = MAP_HEIGHT;

	float lineHeights[SCREEN_WIDTH];
	int lineTextureX[SCREEN_WIDTH];

	int maxFieldDepth = 10;
	// int screenWidth = 400;
	int screenHeight = 240;
	// int renderStartX = 400 - screenWidth;
	int rayCount = 100;
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
			rayY = (float)(((int)playerY >> cellSizeBit) << cellSizeBit) - 0.01f;
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
		float verticalDistance = 1000000;
		float rayNegSlope = -tanf(rayAngle);

		if (rayAngle == P2 || rayAngle == P3) {
			// Looking straight up or down
			depthOfField = maxFieldDepth;
			rayX = playerX;
			rayY = playerY;
		} else if (rayAngle > P2 && rayAngle < P3) {
			// Looking left
			rayX = (float)(((int)playerX >> cellSizeBit) << cellSizeBit) - 0.01f;
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
		lineTextureX[ray] = textureX;

		rayAngle += rayInterval;
	}

	uint32_t *frameBuffer = (uint32_t*)pd->graphics->getFrame();
	for (int drawY = 0; drawY < SCREEN_HEIGHT; drawY++)
	{
		for (int drawInt32 = 0; drawInt32 < 13; drawInt32++)
		{
			uint32_t curDrawInt = 0;
			int textureSize = 16;
			float lineHeight = 0;
			float textureYStep = 0;
			float textureYOffset = 0;
			float lineOffset = 0;
			int textureX = 0;
			int isShaded = 0;
			int textureY = 0;
			LCDColor drawColor;
			for (int drawBit = 0; drawBit < 32; drawBit++)
			{
				int drawX = drawInt32 * 32 + drawBit;
				if (drawX >= 400) {
					continue;
				}

				if (drawBit % 4 == 0)
				{
					int index = drawX >> 2;
					lineHeight = lineHeights[index];
					textureYStep = textureSize / lineHeight;
					textureYOffset = 0;
					if (lineHeight > screenHeight) {
						textureYOffset = (lineHeight - screenHeight) / 2.0f;
						lineHeight = screenHeight;
					}
					lineOffset = (screenHeight >> 1) - lineHeight / 2;
					if (drawY < lineOffset || drawY > lineHeight + lineOffset) {
						continue;
					}
					textureX = lineTextureX[index];
					isShaded = 0;
					if (textureX >= 16) {
						textureX -= 16;
						isShaded = 1;
					}
	
					textureY = (int)(textureYOffset * textureYStep + textureYStep * (drawY - lineOffset));
	
					drawColor = brickTexture[textureX + textureY * textureSize];
				}
				else {
					if (drawY < lineOffset || drawY > lineHeight + lineOffset) {
						continue;
					}
				}

				LCDColor curDrawColor = drawColor;
				if (isShaded == 1) {
					if (drawColor == kColorWhite) {
						if ((drawY % 2 == 0 && drawX % 2 == 0) || (drawY % 2 == 1 && drawX % 2 == 1)) {
							curDrawColor = kColorBlack;
						}
					}
				}

				if (curDrawColor == kColorWhite)
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

void handleInput(float deltaTime)
{
	int cellSize = CELL_SIZE;
	int mapWidth = MAP_WIDTH;

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

int gameUpdate(void* userdata)
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
