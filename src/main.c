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
#define HALF_SCREEN_HEIGHT SCREEN_HEIGHT / 2
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

// Source: https://github.com/TheMediocritist/Wolf_3D/
// int fillPatterns[17][8] = {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
//                       	   {0xFF, 0xFF, 0xFF, 0xEE, 0xFF, 0xFF, 0xFF, 0xEE},
//                       	   {0xFF, 0xBB, 0xFF, 0xEE, 0xFF, 0xBB, 0xFF, 0xEE},
//                       	   {0xFF, 0xBB, 0xFF, 0xAA, 0xFF, 0xBB, 0xFF, 0xAA},
//                       	   {0xFF, 0xAA, 0xFF, 0xAA, 0xFF, 0xAA, 0xFF, 0xAA},
//                       	   {0xFF, 0xAA, 0xDD, 0xAA, 0xFF, 0xAA, 0xDD, 0xAA},
//                       	   {0x77, 0xAA, 0xDD, 0xAA, 0x77, 0xAA, 0xDD, 0xAA},
//                       	   {0x55, 0xAA, 0xDD, 0xAA, 0x55, 0xAA, 0xDD, 0xAA},
//                       	   {0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA},
//                       	   {0x55, 0xAA, 0x55, 0x88, 0x55, 0xAA, 0x55, 0x88},
//                       	   {0x55, 0x22, 0x55, 0x88, 0x55, 0x22, 0x55, 0x88},
//                       	   {0x55, 0x00, 0x55, 0x88, 0x55, 0x00, 0x55, 0x88},
//                       	   {0x55, 0x00, 0x55, 0x00, 0x55, 0x00, 0x55, 0x00},
//                       	   {0x55, 0x00, 0x44, 0x00, 0x55, 0x00, 0x44, 0x00},
//                       	   {0x11, 0x00, 0x44, 0x00, 0x11, 0x00, 0x44, 0x00},
//                       	   {0x11, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00},
//                       	   {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

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
float planeX, planeY;
float dirX, dirY;

#define SKYBOX_SIZE (SCREEN_HEIGHT / 2) * 13
uint8_t brickTexture[256];
uint8_t floorTexture[256];
uint8_t ceilingTexture[256];
uint32_t skybox[SKYBOX_SIZE];

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
	loadTexture("images/brickTexture", brickTexture);
	loadTexture("images/floorTexture", floorTexture);
	loadTexture("images/ceilingTexture", ceilingTexture);

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
	loadTextures();
	playerX = 100;
	playerY = 120;
	playerRadius = 6;
	playerAngle = 0;
	playerTurnSpeed = 0.1f * TARGET_FPS;
	playerMaxSpeed = 2.0f * TARGET_FPS;
	playerXVelocity = cosf(playerAngle) * playerMaxSpeed;
	playerYVelocity = sinf(playerAngle) * playerMaxSpeed;
	planeX = 0;
	planeY = -0.59f; // Related to fov somehow - not sure so I just tweaked it until it looked right
	dirX = 1;
	dirY = 0;
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

	int maxFieldDepth = 10;
	int screenHeight = 240;
	int rayCount = 400;
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
		float verticalDistance = 1000000;
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
		curLineData.lineHeight = (int)lineHeight;

		float textureYStep = textureSize / lineHeight;
		float textureYOffset = 0;
		if (lineHeight > screenHeight) {
			textureYOffset = (lineHeight - screenHeight) / 2.0f;
			lineHeight = screenHeight;
		}
		float lineOffset = (screenHeight >> 1) - lineHeight / 2;
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

		rayAngle += rayInterval;
	}

	// Draw to screen
	float rayDirX0 = dirX + planeX;
	float rayDirY0 = dirY + planeY;
	float rayDirX1 = dirX - planeX;
	float rayDirY1 = dirY - planeY;
	float floorStepXModifier = (rayDirX1 - rayDirX0) / SCREEN_WIDTH;
	float floorStepYModifier = (rayDirY1 - rayDirY0) / SCREEN_WIDTH;
	float playerMapX = playerX / CELL_SIZE;
	float playerMapY = playerY / CELL_SIZE;

	uint32_t *frameBuffer = (uint32_t *)pd->graphics->getFrame();
	for (int drawY = 0; drawY < SCREEN_HEIGHT; drawY++) {

		// Current y position compared to the center of the screen (the horizon)
		int p = drawY >= HALF_SCREEN_HEIGHT ? drawY - HALF_SCREEN_HEIGHT : HALF_SCREEN_HEIGHT - drawY;
		// Horizontal distance from the camera to the floor for the current row.
		// 0.5 is the z position exactly in the middle between floor and ceiling.
		float rowDistance = (float)HALF_SCREEN_HEIGHT / p;

		// calculate the real world step vector we have to add for each x (parallel to camera plane)
		// adding step by step avoids multiplications with a weight in the inner loop
		float floorStepX = rowDistance * floorStepXModifier;
		float floorStepY = rowDistance * floorStepYModifier;
		
		// real world coordinates of the leftmost column. This will be updated as we step to the right.
		float floorX = playerMapX + rowDistance * rayDirX0 - floorStepX;
		float floorY = playerMapY + rowDistance * rayDirY0 - floorStepY;

		for (int drawInt32 = 0; drawInt32 < 13; drawInt32++) {
			int frameBufferIndex = drawY * 13 + drawInt32;
			uint32_t curDrawInt = frameBuffer[frameBufferIndex];
			// if (drawY < SCREEN_HEIGHT / 2) {
			// 	curDrawInt = skybox[frameBufferIndex];
			// }
			int drawBitMax = drawInt32 == 12 ? 16 : 32;
			int drawIntOffset = drawInt32 * 32;
			for (int drawBit = 0; drawBit < drawBitMax; drawBit++) {
				int drawX = drawIntOffset + drawBit;
				LineData curLineData = lineData[drawX];
				int lineHeight = curLineData.lineHeight;
				int lineOffset = curLineData.lineOffset;
				float textureYModifier = curLineData.textureYModifier;
				float textureYStep = curLineData.textureYStep;
				curLineData.textureYModifier = textureYModifier + textureYStep;
				lineData[drawX] = curLineData;
				uint8_t drawColor;
				floorX += floorStepX;
				floorY += floorStepY;
				if (drawY < lineOffset) {
					// the cell coord is simply got from the integer parts of floorX and floorY
					int cellX = (int)(floorX);
					int cellY = (int)(floorY);
			
					// get the texture coordinate from the fractional part
					int textureX = (int)(CELL_SIZE * (floorX - cellX)) & (CELL_SIZE - 1);
					int textureY = (int)(CELL_SIZE * (floorY - cellY)) & (CELL_SIZE - 1);

					// Draw ceiling
					int textureIndex = textureX + (textureY << 4);
					drawColor = ceilingTexture[textureIndex];

					// Draw floor
					int floorDrawColor = floorTexture[textureIndex];
					int floorFrameBufferIndex = (SCREEN_HEIGHT - drawY - 1) * 13 + drawInt32;
					uint32_t floorFrameBufferInt = frameBuffer[floorFrameBufferIndex];
					if (floorDrawColor == 1) {
						floorFrameBufferInt |= 1 << (31 - (drawBit & 31));
					} else {
						floorFrameBufferInt &= ~(1 << (31 - (drawBit & 31)));
					}
					frameBuffer[floorFrameBufferIndex] = floorFrameBufferInt;
				} else if (drawY > lineHeight + lineOffset) {
					continue;
				} else {
					int textureX = curLineData.textureX;
					if (textureX >= 16) {
						textureX -= 16;
						if ((drawX + drawY) % 2 == 0) {
							curDrawInt &= ~(1 << (31 - (drawBit & 31)));
							continue;
						}
					}
	
					int textureY = (int)(textureYModifier);
					drawColor = brickTexture[textureX + (textureY << 4)];
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

		float adjustedTurnSpeed = playerTurnSpeed * deltaTime;
		float oldDirX = dirX;
		dirX = dirX * cosf(-adjustedTurnSpeed) - dirY * sinf(-adjustedTurnSpeed);
		dirY = oldDirX * sinf(-adjustedTurnSpeed) + dirY * cosf(-adjustedTurnSpeed);
		float oldPlaneX = planeX;
		planeX = planeX * cosf(-adjustedTurnSpeed) - planeY * sinf(-adjustedTurnSpeed);
		planeY = oldPlaneX * sinf(-adjustedTurnSpeed) + planeY * cosf(-adjustedTurnSpeed);
	} else if (current & kButtonRight) {
		playerAngle += playerTurnSpeed * deltaTime;
		if (playerAngle > 2 * PI) {
			playerAngle -= 2 * PI;
		}
		playerXVelocity = cosf(playerAngle) * playerMaxSpeed;
		playerYVelocity = sinf(playerAngle) * playerMaxSpeed;

		float adjustedTurnSpeed = playerTurnSpeed * deltaTime;
		float oldDirX = dirX;
      	dirX = dirX * cosf(adjustedTurnSpeed) - dirY * sinf(adjustedTurnSpeed);
		dirY = oldDirX * sinf(adjustedTurnSpeed) + dirY * cosf(adjustedTurnSpeed);
		float oldPlaneX = planeX;
		planeX = planeX * cosf(adjustedTurnSpeed) - planeY * sinf(adjustedTurnSpeed);
		planeY = oldPlaneX * sinf(adjustedTurnSpeed) + planeY * cosf(adjustedTurnSpeed);
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
			playerX += dirX * playerMaxSpeed * deltaTime;
		}

		if (map[lookAheadYIndex * mapWidth + xIndex] == 0) {
			playerY +=  dirY * playerMaxSpeed * deltaTime;
		}
	} else if (current & kButtonDown) {
		if (map[yIndex * mapWidth + lookBehindXIndex] == 0) {
			playerX -=  dirX * playerMaxSpeed * deltaTime;
		}

		if (map[lookBehindYIndex * mapWidth + xIndex] == 0) {
			playerY -=  dirY * playerMaxSpeed * deltaTime;
		}
	}
}

int gameUpdate(void *userdata)
{
	float deltaTime = pd->system->getElapsedTime();
	pd->system->resetElapsedTime();

	pd->display->setRefreshRate(0);

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
