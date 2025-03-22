#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "helpers.h"
#include "game/rendering/drawables.h"
#include "game/rendering/raycasting.h"
#include "game/rendering/textures.h"
#include "game/map/map.h"
#include "game/player/player.h"

// Cacheline is 32 bytes (256 bits) - struct is 16 bytes, so can fit 2 into one cacheline
typedef struct LineData {
	uint16_t lineHeight;
	uint16_t textureX;
	uint32_t lineOffset;
	float textureYStep; // 32 bits
	float textureYModifier; // 32 bits
} LineData;

void renderScene(void)
{
	int cellSize = CELL_SIZE;
	int cellSizeBit = 4;
	int mapWidth = MAP_WIDTH;
	int mapHeight = MAP_HEIGHT;

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

	renderDrawables(zBuffer);

	pd->graphics->markUpdatedRows(0, SCREEN_HEIGHT - 1);
}