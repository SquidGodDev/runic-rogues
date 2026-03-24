#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/helpers.h"
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

static uint8_t *wallTexture;
static uint8_t *ceilingTexture;
static uint8_t *floorTexture;
static uint8_t *gateTexture;

// Cacheline is 32 bytes (256 bits)
typedef struct LineData {
	uint16_t lineHeight;
	uint16_t textureX;
	uint32_t lineOffset;
	uint8_t *wallTexture;
	float textureYStep; // 32 bits
	float textureYModifier; // 32 bits
	float ceilingXModifier;
	float ceilingYModifier;
} LineData;

void initRaycasting(void) {
	AreaData curAreaData = getCurrentAreaData();
	wallTexture = curAreaData.wallTexture;
	ceilingTexture = curAreaData.ceilingTexture;
	floorTexture = curAreaData.floorTexture;
	gateTexture = curAreaData.gateTexture;
}

void renderScene(void)
{
	int cellSize = CELL_SIZE;
	int cellSizeBit = 4;
	int mapSize = MAP_WIDTH * MAP_HEIGHT;

	LineData lineData[SCREEN_WIDTH];
	int zBuffer[SCREEN_WIDTH];

	int rayCount = SCREEN_WIDTH;
	float rayInterval = FOV_RAD / rayCount;
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

		float sinAngle = sinf(rayAngle);
		float cosAngle = cosf(rayAngle);

		// === Check for horizontal lines ===
		float horizontalRayX = 0.0f, horizontalRayY = 0.0f;
		float horizontalDistance = maxDistance;
		float rayPerpendicularSlope = -cosAngle / sinAngle;

		if (rayAngle == 0 || rayAngle == PI) {
			// Looking straight left or right
			depthOfField = MAX_FIELD_DEPTH;
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

		while (depthOfField < MAX_FIELD_DEPTH) {
			int mapX = (int)rayX >> cellSizeBit;
			int mapY = (int)rayY >> cellSizeBit;
			int mapIndex = (mapY * MAP_WIDTH) + mapX;
			if ((mapIndex < 0 || mapIndex >= mapSize) || map[mapIndex] > 0) {
				// Hit wall
				depthOfField = MAX_FIELD_DEPTH;
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
		float verticalRayX = 0.0f, verticalRayY = 0.0f;
		float verticalDistance = maxDistance;
		float rayNegSlope = -sinAngle / cosAngle;

		if (rayAngle == P2 || rayAngle == P3) {
			// Looking straight up or down
			depthOfField = MAX_FIELD_DEPTH;
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

		while (depthOfField < MAX_FIELD_DEPTH) {
			int mapX = (int)rayX >> cellSizeBit;
			int mapY = (int)rayY >> cellSizeBit;
			int mapIndex = (mapY * MAP_WIDTH) + mapX;
			if ((mapIndex < 0 || mapIndex >= mapSize) || map[mapIndex] > 0) {
				depthOfField = MAX_FIELD_DEPTH; // Hit wall
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

		int wallType = MAP_WALL;
		int mapX = (int)rayX >> cellSizeBit;
		int mapY = (int)rayY >> cellSizeBit;
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
		float offsetAngle = playerAngle - rayAngle;
		if (offsetAngle < 0) {
			offsetAngle += 2 * PI;
		} else if (offsetAngle > 2 * PI) {
			offsetAngle -= 2 * PI;
		}
		float fixedOffsetAngle = cosf(offsetAngle);
		rayDistance = rayDistance * fixedOffsetAngle;

		curLineData.ceilingXModifier = cosAngle * CEILING_RENDER_CONSTANT * CELL_SIZE / fixedOffsetAngle;
		curLineData.ceilingYModifier = sinAngle * CEILING_RENDER_CONSTANT * CELL_SIZE / fixedOffsetAngle;

		if (verticalDistance > (maxDistance - 1) && horizontalDistance > (maxDistance - 1)) {
			curLineData.lineHeight = 0;
			curLineData.lineOffset = HALF_SCREEN_HEIGHT;
			lineData[ray] = curLineData;
			zBuffer[ray] = rayDistance;

			rayAngle += rayInterval;
			continue;
		}

		// === Draw 3D Walls ===

		// Get wall slice draw height
		float lineHeight = (cellSize * SCREEN_HEIGHT) / rayDistance;
		curLineData.lineHeight = (int)lineHeight;

		float textureYStep = ENVIRONMENT_TEXTURE_SIZE / lineHeight;
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
			textureX = ((int)rayX) % ENVIRONMENT_TEXTURE_SIZE;
			if (rayAngle < PI) {
				textureX = ENVIRONMENT_TEXTURE_SIZE - textureX - 1;
			}
			textureX += 16;
		} else {
			textureX = ((int)rayY) % ENVIRONMENT_TEXTURE_SIZE;
			if (rayAngle > P2 && rayAngle < P3) {
				textureX = ENVIRONMENT_TEXTURE_SIZE - textureX - 1;
			}
		}
		curLineData.textureX = textureX;
		lineData[ray] = curLineData;
		zBuffer[ray] = rayDistance;

		rayAngle += rayInterval;
	}

	int fogOffset = ((int)(radToDeg(playerAngle)) & ~1) % 8;

	int maxDrawInt = ceilf((float)SCREEN_WIDTH / 32);
	int bitRemainder = SCREEN_WIDTH % 32;
	uint32_t *frameBuffer = (uint32_t *)pd->graphics->getFrame();
	for (int drawY = 0; drawY < SCREEN_HEIGHT; drawY++) {
		// Horizontal distance from the camera to the floor for the current row.
		// 0.5 is the z position exactly in the middle between floor and ceiling.
		float rowDistance = (float)HALF_SCREEN_HEIGHT / (HALF_SCREEN_HEIGHT - drawY);
		float invDy = 1.0f / (drawY - HALF_SCREEN_HEIGHT);

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
			uint8_t *fogPattern = fogPatterns[fogPatternIndex];
			fogShade = fogPattern[fogRow];
		}

		for (int drawInt32 = 0; drawInt32 < maxDrawInt; drawInt32++) {
			int frameBufferIndex = drawY * FRAME_BUFFER_INTS_PER_ROW + drawInt32;
			uint32_t curDrawInt = frameBuffer[frameBufferIndex];
			int floorFrameBufferIndex = (SCREEN_HEIGHT - drawY - 1) * FRAME_BUFFER_INTS_PER_ROW + drawInt32;
			uint32_t curFloorDrawInt = 0;
			int drawBitMax = drawInt32 == (maxDrawInt - 1) ? bitRemainder : 32;
			int drawIntOffset = drawInt32 << 5; // multiply by 32
			for (int drawBit = 0; drawBit < drawBitMax; drawBit++) {
				uint8_t drawColor;
				int drawX = drawIntOffset + drawBit;
				int fogShadeBit = (drawX + fogOffset) & 7; // mod 8

				LineData *curLineData = &lineData[drawX];
				int lineHeight = curLineData->lineHeight;
				int lineOffset = curLineData->lineOffset;
				uint8_t *curWallTexture = curLineData->wallTexture;

				float textureYModifier = curLineData->textureYModifier;
				textureYModifier = textureYModifier > ENVIRONMENT_TEXTURE_SIZE - 1 ? ENVIRONMENT_TEXTURE_SIZE - 1 : textureYModifier;
				lineData[drawX].textureYModifier += curLineData->textureYStep;

				if (drawY < lineOffset) {
					int fogReject = skipCeilingFloor | ((fogShade >> fogShadeBit) & inFog);
					float textureCoordX = playerX - curLineData->ceilingXModifier * invDy;
					float textureCoordY = playerY - curLineData->ceilingYModifier * invDy;

					uint32_t textureX = (uint32_t)(textureCoordX) & (CELL_SIZE - 1);
					uint32_t textureY = (uint32_t)(textureCoordY) & (CELL_SIZE - 1);

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
	
					uint32_t textureY = (int)(textureYModifier + 0.5f);
					drawColor = curWallTexture[textureX + (textureY << 4)];

					uint32_t fogShade = wallDistance < FOG_MIN_DIST ? 0 : fogPatterns[((int)wallDistance - FOG_MIN_DIST) / FOG_STEP][fogRow];
					if ((fogShade >> fogShadeBit) & 1) {
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