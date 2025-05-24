#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "helpers.h"
#include "game/entities/entities.h"
#include "game/rendering/raycasting.h"
#include "game/rendering/textures.h"
#include "game/map/map.h"
#include "game/player/player.h"
#include "game/systems/progressionManager.h"

#define ENVIRONMENT_TEXTURE_SIZE 16
#define CEILING_RENDER_CONSTANT 67 // Lower scales larger, higher scales smaller
#define FRAME_BUFFER_INTS_PER_ROW 13 // ceil(LCD_COLUMNS / 32)

uint8_t *wallTexture;
uint8_t *ceilingTexture;
uint8_t *floorTexture;

// Cacheline is 32 bytes (256 bits)
typedef struct LineData {
	uint16_t lineHeight;
	uint16_t textureX;
	uint32_t lineOffset;
	float textureYStep; // 32 bits
	float textureYModifier; // 32 bits
	float ceilingXModifier;
	float ceilingYModifier;
} LineData;

void initRaycasting(void) {
	FloorData curFloorData = getCurrentFloorData();
	wallTexture = curFloorData.wallTexture;
	ceilingTexture = curFloorData.ceilingTexture;
	floorTexture = curFloorData.floorTexture;
}

void renderScene(void)
{
	int cellSize = CELL_SIZE;
	int cellSizeBit = 4;
	int mapSize = MAP_WIDTH * MAP_HEIGHT;

	LineData lineData[SCREEN_WIDTH];
	float zBuffer[SCREEN_WIDTH];

	int maxFieldDepth = 8;
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
			int mapIndex = (mapY << MAP_WIDTH_BIT) + mapX;
			if (mapIndex >= 0 && mapIndex < mapSize && map[mapIndex] == 1) {
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
			int mapIndex = (mapY << MAP_WIDTH_BIT) + mapX;
			if (mapIndex >= 0 && mapIndex < mapSize && map[mapIndex] == 1) {
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

		// Fix fisheye
		float offsetAngle = playerAngle - rayAngle;
		if (offsetAngle < 0) {
			offsetAngle += 2 * PI;
		} else if (offsetAngle > 2 * PI) {
			offsetAngle -= 2 * PI;
		}
		float fixedOffsetAngle = cosf(offsetAngle);
		rayDistance = rayDistance * fixedOffsetAngle;

		curLineData.ceilingXModifier = cosf(rayAngle) * CEILING_RENDER_CONSTANT * CELL_SIZE / fixedOffsetAngle;
		curLineData.ceilingYModifier = sinf(rayAngle) * CEILING_RENDER_CONSTANT * CELL_SIZE / fixedOffsetAngle;

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
			int *fogPattern = fogPatterns[fogPatternIndex];
			fogShade = fogPattern[fogRow];
		}

		for (int drawInt32 = 0; drawInt32 < maxDrawInt; drawInt32++) {
			int frameBufferIndex = drawY * FRAME_BUFFER_INTS_PER_ROW + drawInt32;
			uint32_t curDrawInt = frameBuffer[frameBufferIndex];
			int drawBitMax = drawInt32 == (maxDrawInt - 1) ? bitRemainder : 32;
			int drawIntOffset = drawInt32 * 32;
			for (int drawBit = 0; drawBit < drawBitMax; drawBit++) {
				uint8_t drawColor;
				int drawX = drawIntOffset + drawBit;
				int fogShadeBit = (drawX + fogOffset) & 7; // mod 8

				LineData curLineData = lineData[drawX];
				int lineHeight = curLineData.lineHeight;
				int lineOffset = curLineData.lineOffset;

				float textureYModifier = curLineData.textureYModifier;
				if (textureYModifier > ENVIRONMENT_TEXTURE_SIZE - 1) {
					textureYModifier = ENVIRONMENT_TEXTURE_SIZE - 1;
				}
				lineData[drawX].textureYModifier += curLineData.textureYStep;

				if (drawY < lineOffset) {
					int floorDrawColor = 0;
					if (skipCeilingFloor || (inFog & ((fogShade >> fogShadeBit) & 1))) {
						drawColor = 0;
						floorDrawColor = 0;
					} else {
						float textureCoordX = playerX - curLineData.ceilingXModifier * invDy;
						float textureCoordY = playerY - curLineData.ceilingYModifier * invDy;

						int textureX = (int)(textureCoordX) & (CELL_SIZE - 1);
						int textureY = (int)(textureCoordY) & (CELL_SIZE - 1);
	
						// Draw ceiling
						int textureIndex = textureX + (textureY << 4);
						drawColor = ceilingTexture[textureIndex];
	
						// Draw floor
						floorDrawColor = floorTexture[textureIndex];
					}

					int floorFrameBufferIndex = (SCREEN_HEIGHT - drawY - 1) * FRAME_BUFFER_INTS_PER_ROW + drawInt32;
					uint32_t floorFrameBufferInt = frameBuffer[floorFrameBufferIndex];
					if (floorDrawColor == 1) {
						floorFrameBufferInt |= 1 << (31 - (drawBit & 31));
					} else {
						floorFrameBufferInt &= ~(1 << (31 - (drawBit & 31)));
					}
					frameBuffer[floorFrameBufferIndex] = floorFrameBufferInt;
				} else if (drawY > lineHeight + lineOffset + 1) {
					// Floor - we already drew this when we drew the ceiling
					continue;
				} else {
					float wallDistance = zBuffer[drawX];
					if (wallDistance > FOG_MAX_DIST) {
						curDrawInt &= ~(1 << (31 - (drawBit & 31)));
						continue;
					}
					int textureX = curLineData.textureX;
					if (textureX >= 16) {
						// Shaded wall
						textureX -= 16;
						if (((drawX + drawY) & 1) == 0) {
							curDrawInt &= ~(1 << (31 - (drawBit & 31)));
							continue;
						}
					}
	
					int textureY = (int)(textureYModifier + 0.5f);
					drawColor = wallTexture[textureX + (textureY << 4)];

					int fogShade = 0;
					if (wallDistance < FOG_MIN_DIST) {
						fogShade = 0;
					} else if (wallDistance >= FOG_MIN_DIST) {
						int fogPatternIndex = ((int)wallDistance - FOG_MIN_DIST) / FOG_STEP;
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

	renderEntities(zBuffer);

	pd->graphics->markUpdatedRows(0, SCREEN_HEIGHT - 1);
}