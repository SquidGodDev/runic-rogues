#define TARGET_EXTENSION 1
#include "pd_api.h"
#include <math.h>

#include "globals/globals.h"
#include "globals/helpers.h"
#include "globals/fixedPoint.h"
#include "game/entities/entities.h"
#include "game/rendering/raycasting.h"
#include "game/rendering/textures.h"
#include "game/map/map.h"
#include "game/player/player.h"
#include "game/systems/progressionManager.h"

#define MAX_FIELD_DEPTH 12
#define ENVIRONMENT_TEXTURE_SIZE 16
#define CEILING_RENDER_CONSTANT 65 * SCREEN_WIDTH / 200 // Lower scales larger, higher scales smaller
#define FRAME_BUFFER_INTS_PER_ROW 13 // ceil(LCD_COLUMNS / 32)

#define TRIG_TABLE_SIZE  1024
#define TRIG_TABLE_MASK  (TRIG_TABLE_SIZE - 1)
#define TRIG_QUARTER     (TRIG_TABLE_SIZE / 4)

static uint8_t *wallTexture;
static uint8_t *ceilingTexture;
static uint8_t *floorTexture;
static uint8_t *gateTexture;

static fixed_t sinTable[TRIG_TABLE_SIZE];
static fixed_t invDyTable[HALF_SCREEN_HEIGHT];

typedef struct RowFogData {
	uint8_t *fogPattern;
	int skipCeilingFloor;
	int inFog;
} RowFogData;
static RowFogData rowFogTable[HALF_SCREEN_HEIGHT];

// Cacheline is 32 bytes (256 bits)
typedef struct LineData {
	uint16_t lineHeight;
	uint16_t textureX;
	uint32_t lineOffset;
	uint8_t *wallTexture;
	fixed_t textureYStep;
	fixed_t textureYModifier;
	fixed_t ceilingXModifier;
	fixed_t ceilingYModifier;
} LineData;

static inline int angleToIndex(float radians) {
	float normalized = radians * (TRIG_TABLE_SIZE / (2.0f * PI));
	int index = (int)normalized;
	return index & TRIG_TABLE_MASK;
}

static inline fixed_t fpSin(int index) {
	return sinTable[index & TRIG_TABLE_MASK];
}

static inline fixed_t fpCos(int index) {
	return sinTable[(index + TRIG_QUARTER) & TRIG_TABLE_MASK];
}

// Integer sqrt of a Q16.16 distance-squared value. Uses 64-bit to avoid overflow
// for large map distances. Result is Q16.16.
static inline fixed_t fixedSqrt(int64_t distSq) {
	// distSq = d^2 * 65536 (Q16.16). Shift right to get d^2 as int32, take fastSqrt to get d.
	return FLOAT_TO_FP(fastSqrt((float)(int32_t)(distSq >> FP_SHIFT)));
}

void initRaycasting(void) {
	AreaData curAreaData = getCurrentAreaData();
	wallTexture = curAreaData.wallTexture;
	ceilingTexture = curAreaData.ceilingTexture;
	floorTexture = curAreaData.floorTexture;
	gateTexture = curAreaData.gateTexture;

	for (int i = 0; i < TRIG_TABLE_SIZE; i++) {
		float angle = (float)i * 2.0f * PI / TRIG_TABLE_SIZE;
		sinTable[i] = FLOAT_TO_FP(sinf(angle));
	}

	for (int y = 0; y < HALF_SCREEN_HEIGHT; y++) {
		float rowDist = (float)HALF_SCREEN_HEIGHT / (HALF_SCREEN_HEIGHT - y);
		float invDy = 1.0f / (y - HALF_SCREEN_HEIGHT);
		invDyTable[y] = FLOAT_TO_FP(invDy);

		float ceilingDist = rowDist * CELL_SIZE;
		RowFogData *fogData = &rowFogTable[y];
		fogData->skipCeilingFloor = ceilingDist > FOG_MAX_DIST;
		if (ceilingDist < FOG_MIN_DIST) {
			fogData->inFog = 0;
			fogData->fogPattern = NULL;
		} else if (ceilingDist <= FOG_MAX_DIST) {
			fogData->inFog = 1;
			int fogPatternIndex = (int)((ceilingDist - FOG_MIN_DIST) / FOG_STEP);
			fogData->fogPattern = fogPatterns[fogPatternIndex];
		} else {
			fogData->inFog = 0;
			fogData->fogPattern = NULL;
		}
	}
}

void renderScene(void)
{
	int cellSizeBit = 4;
	int mapSize = MAP_WIDTH * MAP_HEIGHT;

	LineData lineData[SCREEN_WIDTH];
	int zBuffer[SCREEN_WIDTH];

	int rayCount = SCREEN_WIDTH;
	float rayInterval = FOV_RAD / rayCount;
	float rayAngle = playerAngle - rayInterval * rayCount / 2;

	fixed_t playerX_fp = FLOAT_TO_FP(playerX);
	fixed_t playerY_fp = FLOAT_TO_FP(playerY);

	int playerAngleIndex = angleToIndex(playerAngle);

	for (int ray = 0; ray < rayCount; ray++) {
		if (rayAngle < 0) {
			rayAngle += 2 * PI;
		} else if (rayAngle > 2 * PI) {
			rayAngle -= 2 * PI;
		}

		int rayAngleIndex = angleToIndex(rayAngle);

		fixed_t rayX = playerX_fp;
		fixed_t rayY = playerY_fp;
		fixed_t xOffset = 0;
		fixed_t yOffset = 0;
		int depthOfField = 0;
		// Sentinel: larger than any possible Q16.16 squared distance on the map.
		// Q16.16 distSq for distance d = d^2 * 65536. Max map dist ~475 units → ~14.8B.
		int64_t maxDistSq = 9999999999LL;

		fixed_t sinAngle = fpSin(rayAngleIndex);
		fixed_t cosAngle = fpCos(rayAngleIndex);

		// === Check for horizontal lines ===
		fixed_t horizontalRayX = 0, horizontalRayY = 0;
		int64_t horizontalDistance = maxDistSq;

		if (sinAngle == 0) {
			// Looking straight left or right
			depthOfField = MAX_FIELD_DEPTH;
		} else {
			fixed_t rayPerpendicularSlope = FP_DIV(-cosAngle, sinAngle);

			if (sinAngle < 0) {
				// Looking up
				rayY = INT_TO_FP((FP_TO_INT(playerY_fp) >> cellSizeBit) << cellSizeBit) - 1;
				rayX = FP_MUL(playerY_fp - rayY, rayPerpendicularSlope) + playerX_fp;
				yOffset = -INT_TO_FP(CELL_SIZE);
				xOffset = FP_MUL(-yOffset, rayPerpendicularSlope);
			} else {
				// Looking down
				rayY = INT_TO_FP(((FP_TO_INT(playerY_fp) >> cellSizeBit) << cellSizeBit) + CELL_SIZE);
				rayX = FP_MUL(playerY_fp - rayY, rayPerpendicularSlope) + playerX_fp;
				yOffset = INT_TO_FP(CELL_SIZE);
				xOffset = FP_MUL(-yOffset, rayPerpendicularSlope);
			}
		}

		while (depthOfField < MAX_FIELD_DEPTH) {
			int mapX = FP_TO_INT(rayX) >> cellSizeBit;
			int mapY = FP_TO_INT(rayY) >> cellSizeBit;
			int mapIndex = (mapY * MAP_WIDTH) + mapX;
			if ((mapIndex < 0 || mapIndex >= mapSize) || map[mapIndex] > 0) {
				depthOfField = MAX_FIELD_DEPTH;
				int64_t dx = (int64_t)(playerX_fp - rayX);
				int64_t dy = (int64_t)(playerY_fp - rayY);
				horizontalDistance = (dx * dx + dy * dy) >> FP_SHIFT;
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
		rayX = playerX_fp;
		rayY = playerY_fp;
		xOffset = 0;
		yOffset = 0;
		fixed_t verticalRayX = 0, verticalRayY = 0;
		int64_t verticalDistance = maxDistSq;

		if (cosAngle == 0) {
			// Looking straight up or down
			depthOfField = MAX_FIELD_DEPTH;
		} else {
			fixed_t rayNegSlope = FP_DIV(-sinAngle, cosAngle);

			if (cosAngle < 0) {
				// Looking left
				rayX = INT_TO_FP((FP_TO_INT(playerX_fp) >> cellSizeBit) << cellSizeBit) - 1;
				rayY = FP_MUL(playerX_fp - rayX, rayNegSlope) + playerY_fp;
				xOffset = -INT_TO_FP(CELL_SIZE);
				yOffset = FP_MUL(-xOffset, rayNegSlope);
			} else {
				// Looking right
				rayX = INT_TO_FP(((FP_TO_INT(playerX_fp) >> cellSizeBit) << cellSizeBit) + CELL_SIZE);
				rayY = FP_MUL(playerX_fp - rayX, rayNegSlope) + playerY_fp;
				xOffset = INT_TO_FP(CELL_SIZE);
				yOffset = FP_MUL(-xOffset, rayNegSlope);
			}
		}

		while (depthOfField < MAX_FIELD_DEPTH) {
			int mapX = FP_TO_INT(rayX) >> cellSizeBit;
			int mapY = FP_TO_INT(rayY) >> cellSizeBit;
			int mapIndex = (mapY * MAP_WIDTH) + mapX;
			if ((mapIndex < 0 || mapIndex >= mapSize) || map[mapIndex] > 0) {
				depthOfField = MAX_FIELD_DEPTH;
				int64_t dx = (int64_t)(playerX_fp - rayX);
				int64_t dy = (int64_t)(playerY_fp - rayY);
				verticalDistance = (dx * dx + dy * dy) >> FP_SHIFT;
				verticalRayX = rayX;
				verticalRayY = rayY;
			} else {
				rayX += xOffset;
				rayY += yOffset;
				depthOfField += 1;
			}
		}

		LineData curLineData;
		curLineData.wallTexture = wallTexture;
		fixed_t rayDistance = fixedSqrt(maxDistSq);
		int isShaded = 0;
		if (verticalDistance < horizontalDistance) {
			rayX = verticalRayX;
			rayY = verticalRayY;
			rayDistance = fixedSqrt(verticalDistance);
		} else if (horizontalDistance < verticalDistance) {
			rayX = horizontalRayX;
			rayY = horizontalRayY;
			rayDistance = fixedSqrt(horizontalDistance);
			isShaded = 1;
		}

		int wallType = MAP_WALL;
		int mapX = FP_TO_INT(rayX) >> cellSizeBit;
		int mapY = FP_TO_INT(rayY) >> cellSizeBit;
		int mapIndex = (mapY * MAP_WIDTH) + mapX;
		if ((mapIndex < 0 || mapIndex >= mapSize) || map[mapIndex] > 0) {
			wallType = map[mapIndex];
		}

		if (wallType == MAP_GATE) {
			curLineData.wallTexture = gateTexture;
		} else if (wallType == MAP_WALL) {
			curLineData.wallTexture = wallTexture;
		}

		// Fix fisheye
		int offsetAngleIndex = playerAngleIndex - rayAngleIndex;
		fixed_t fixedOffsetAngle = fpCos(offsetAngleIndex);
		rayDistance = FP_MUL(rayDistance, fixedOffsetAngle);

		curLineData.ceilingXModifier = FP_DIV(FP_MUL(cosAngle, INT_TO_FP(CEILING_RENDER_CONSTANT * CELL_SIZE)), fixedOffsetAngle);
		curLineData.ceilingYModifier = FP_DIV(FP_MUL(sinAngle, INT_TO_FP(CEILING_RENDER_CONSTANT * CELL_SIZE)), fixedOffsetAngle);

		if (verticalDistance >= maxDistSq && horizontalDistance >= maxDistSq) {
			curLineData.lineHeight = 0;
			curLineData.lineOffset = HALF_SCREEN_HEIGHT;
			lineData[ray] = curLineData;
			zBuffer[ray] = FP_TO_INT(rayDistance);

			rayAngle += rayInterval;
			continue;
		}

		// === Draw 3D Walls ===

		// Get wall slice draw height
		fixed_t lineHeight = FP_DIV(INT_TO_FP(CELL_SIZE * SCREEN_HEIGHT), rayDistance);
		curLineData.lineHeight = FP_TO_INT(lineHeight);

		fixed_t textureYStep = FP_DIV(INT_TO_FP(ENVIRONMENT_TEXTURE_SIZE), lineHeight);
		fixed_t textureYOffset = 0;
		if (lineHeight > INT_TO_FP(SCREEN_HEIGHT)) {
			textureYOffset = (lineHeight - INT_TO_FP(SCREEN_HEIGHT)) >> 1;
			lineHeight = INT_TO_FP(SCREEN_HEIGHT);
		}
		fixed_t lineOffset = INT_TO_FP(SCREEN_HEIGHT >> 1) - (lineHeight >> 1);
		curLineData.textureYStep = textureYStep;
		curLineData.textureYModifier = FP_MUL(textureYOffset, textureYStep) - FP_MUL(textureYStep, lineOffset);
		curLineData.lineOffset = FP_TO_INT(lineOffset);

		int textureX;
		if (isShaded == 1) {
			textureX = FP_TO_INT(rayX) % ENVIRONMENT_TEXTURE_SIZE;
			if (rayAngle < PI) {
				textureX = ENVIRONMENT_TEXTURE_SIZE - textureX - 1;
			}
			textureX += 16;
		} else {
			textureX = FP_TO_INT(rayY) % ENVIRONMENT_TEXTURE_SIZE;
			if (rayAngle > P2 && rayAngle < P3) {
				textureX = ENVIRONMENT_TEXTURE_SIZE - textureX - 1;
			}
		}
		curLineData.textureX = textureX;
		lineData[ray] = curLineData;
		zBuffer[ray] = FP_TO_INT(rayDistance);

		rayAngle += rayInterval;
	}

	int fogOffset = ((int)(radToDeg(playerAngle)) & ~1) % 8;

	int maxDrawInt = ceilf((float)SCREEN_WIDTH / 32);
	int bitRemainder = SCREEN_WIDTH % 32;
	fixed_t texClampMax = INT_TO_FP(ENVIRONMENT_TEXTURE_SIZE - 1);
	uint32_t *frameBuffer = (uint32_t *)pd->graphics->getFrame();
	for (int drawY = 0; drawY < SCREEN_HEIGHT; drawY++) {
		fixed_t invDy = 0;
		int skipCeilingFloor = 0;
		int inFog = 0;
		int fogRow = drawY % 8;
		int fogShade = 0;

		if (drawY < HALF_SCREEN_HEIGHT) {
			invDy = invDyTable[drawY];
			RowFogData *fogData = &rowFogTable[drawY];
			skipCeilingFloor = fogData->skipCeilingFloor;
			inFog = fogData->inFog;
			if (inFog) {
				fogShade = fogData->fogPattern[fogRow];
			}
		}

		for (int drawInt32 = 0; drawInt32 < maxDrawInt; drawInt32++) {
			int frameBufferIndex = drawY * FRAME_BUFFER_INTS_PER_ROW + drawInt32;
			uint32_t curDrawInt = frameBuffer[frameBufferIndex];
			int floorFrameBufferIndex = (SCREEN_HEIGHT - drawY - 1) * FRAME_BUFFER_INTS_PER_ROW + drawInt32;
			uint32_t curFloorDrawInt = 0;
			int drawBitMax = drawInt32 == (maxDrawInt - 1) ? bitRemainder : 32;
			int drawIntOffset = drawInt32 << 5;
			for (int drawBit = 0; drawBit < drawBitMax; drawBit++) {
				uint8_t drawColor;
				int drawX = drawIntOffset + drawBit;
				int fogShadeBit = (drawX + fogOffset) & 7;

				LineData *curLineData = &lineData[drawX];
				int lineHeight = curLineData->lineHeight;
				int lineOffset = curLineData->lineOffset;
				uint8_t *curWallTexture = curLineData->wallTexture;

				fixed_t textureYModifier = curLineData->textureYModifier;
				if (textureYModifier > texClampMax) {
					textureYModifier = texClampMax;
				} else if (textureYModifier < 0) {
					textureYModifier = 0;
				}
				lineData[drawX].textureYModifier += curLineData->textureYStep;

				if (drawY < lineOffset) {
					int fogReject = skipCeilingFloor | ((fogShade >> fogShadeBit) & inFog);
					fixed_t textureCoordX = playerX_fp - FP_MUL(curLineData->ceilingXModifier, invDy);
					fixed_t textureCoordY = playerY_fp - FP_MUL(curLineData->ceilingYModifier, invDy);

					uint32_t textureX = FP_TO_INT(textureCoordX) & (CELL_SIZE - 1);
					uint32_t textureY = FP_TO_INT(textureCoordY) & (CELL_SIZE - 1);

					// Draw ceiling
					uint32_t textureIndex = textureX + (textureY << 4);
					drawColor = ceilingTexture[textureIndex];
					drawColor &= ~(-fogReject);

					// Draw floor
					curFloorDrawInt |= (floorTexture[textureIndex] << (31 - (drawBit & 31))) & ~(-fogReject);
				} else if (drawY > lineHeight + lineOffset + 1) {
					// Floor - we already drew this when we drew the ceiling
					continue;
				} else {
					int wallDistance = zBuffer[drawX];
					if (wallDistance > FOG_MAX_DIST) {
						curDrawInt &= ~(1 << (31 - (drawBit & 31)));
						continue;
					}
					uint32_t textureX = curLineData->textureX;
					if (textureX >= 16) {
						// Shaded wall
						textureX -= 16;
						if (((drawX + drawY) & 1) == 0) {
							curDrawInt &= ~(1 << (31 - (drawBit & 31)));
							continue;
						}
					}

					uint32_t textureY = (uint32_t)FP_TO_INT_ROUND(textureYModifier);
					drawColor = curWallTexture[textureX + (textureY << 4)];

					uint32_t wallFogShade = wallDistance < FOG_MIN_DIST ? 0 : fogPatterns[(wallDistance - FOG_MIN_DIST) / FOG_STEP][fogRow];
					if ((wallFogShade >> fogShadeBit) & 1) {
						drawColor = 0;
					}
				}

				uint32_t bitMask = 1 << (31 - (drawBit & 31));
				curDrawInt = (curDrawInt & ~bitMask) | (-((int)drawColor & 1) & bitMask);
			}
			curDrawInt = swap(curDrawInt);
			frameBuffer[frameBufferIndex] = curDrawInt;
			if (drawY < HALF_SCREEN_HEIGHT) {
				frameBuffer[floorFrameBufferIndex] = curFloorDrawInt;
			}
		}
	}

	renderEntities(zBuffer);

	pd->graphics->markUpdatedRows(0, SCREEN_HEIGHT - 1);
}