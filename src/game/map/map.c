#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "helpers.h"
#include "game/map/map.h"
#include "game/entities/interactables.h"
#include "game/systems/progressionManager.h"
#include "game/entities/pickups.h"
#include "game/UI/UI.h"

#define PLAYDATE_SDK 1

typedef struct RoomLayout {
    int mapWidth;
    int mapHeight;
    Point playerSpawnMapPosition;
    Point exitMapPosition;
} RoomLayout;

#define CAMPFIRE_ROOM_WIDTH MAP_WIDTH / 2
#define CAMPFIRE_ROOM_HEIGHT MAP_HEIGHT / 2
#define MAP_PADDING 4

RoomLayout roomLayouts[ROOM_TYPE_COUNT] = {
    [ROOM_TYPE_ENEMY] = (RoomLayout){
        .mapWidth = MAP_WIDTH,
        .mapHeight = MAP_HEIGHT,
        .playerSpawnMapPosition = (Point){.x = MAP_WIDTH / 2, .y = MAP_HEIGHT / 2},
        .exitMapPosition = (Point){.x = MAP_WIDTH / 2, .y = MAP_PADDING - 3}
    },
    [ROOM_TYPE_ELITE] = (RoomLayout){
        .mapWidth = MAP_WIDTH,
        .mapHeight = MAP_HEIGHT,
        .playerSpawnMapPosition = (Point){.x = MAP_WIDTH / 2, .y = MAP_HEIGHT / 2},
        .exitMapPosition = (Point){.x = MAP_WIDTH / 2, .y = MAP_PADDING - 3}
    },
    [ROOM_TYPE_CAMPFIRE] = (RoomLayout){
        .mapWidth = 14,
        .mapHeight = 5,
        .playerSpawnMapPosition = (Point){.x = 1, .y = 2},
        .exitMapPosition = (Point){.x = 10, .y = 2}
    },
    [ROOM_TYPE_BOSS] = (RoomLayout){
        .mapWidth = MAP_WIDTH,
        .mapHeight = MAP_HEIGHT,
        .playerSpawnMapPosition = (Point){.x = MAP_WIDTH / 2, .y = MAP_HEIGHT / 2},
        .exitMapPosition = (Point){.x = MAP_WIDTH / 2, .y = MAP_PADDING - 3}
    }
};

static RoomType curRoomType;

int map[MAP_WIDTH * MAP_HEIGHT];

typedef struct Rect {
    int x, y, width, height;
} Rect;

typedef struct Node {
    Rect region;
    Rect room;
    struct Node* left;
    struct Node* right;
} Node;

void printMap(void) {
    for (int y = 0; y < MAP_HEIGHT; y++) {
        char rowString[MAP_WIDTH * 2];
        for (int x = 0; x < MAP_WIDTH; x++) {
            int mapValue = map[y * MAP_WIDTH + x];
            rowString[x*2] = mapValue == 1 ? '#' : '.';
            rowString[x*2 + 1] = ' ';
        }
        #if PLAYDATE_SDK
            pd->system->logToConsole("%s\n", rowString);
        #else
            printf("%s\n", rowString);
        #endif
    }
}

void fillMap(int tile) {
    for (int i = 0; i < MAP_WIDTH * MAP_HEIGHT; i++) {
        map[i] = tile;
    }
}

void setCell(int x, int y, int value) {
    if (x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT) {
        map[y * MAP_WIDTH + x] = value;
    }
}

void encloseMapInWalls(int width, int height) {
    for (int x = MAP_PADDING; x < width - MAP_PADDING; x++) {
        if (x == width / 2) {
            setCell(x, MAP_PADDING, MAP_GATE); // Top row
            setCell(x, height - MAP_PADDING - 1, MAP_GATE); // Bottom row
        } else {
            setCell(x, MAP_PADDING, MAP_WALL); // Top row
            setCell(x, height - MAP_PADDING - 1, MAP_WALL); // Bottom row
        }
    }
    for (int y = MAP_PADDING; y < height - MAP_PADDING; y++) {
        if (y == height / 2) {
            setCell(MAP_PADDING, y, MAP_GATE); // Left column
            setCell(width - MAP_PADDING - 1, y, MAP_GATE); // Right column
        } else {
            setCell(MAP_PADDING, y, MAP_WALL); // Left column
            setCell(width - MAP_PADDING - 1, y, MAP_WALL); // Right column
        }
    }
}

Point getPlayerSpawnMapPosition(void) {
    return roomLayouts[curRoomType].playerSpawnMapPosition;
}

Point getExitMapPosition(void) {
    return roomLayouts[curRoomType].exitMapPosition;
}

Point getRandomEnemySpawnPosition(void) {
    int x, y;
    int randNum = randInt(0, 3);
    int width = roomLayouts[curRoomType].mapWidth;
    int height = roomLayouts[curRoomType].mapHeight;
    if (randNum == 0) {
        x = (MAP_PADDING + 1) * CELL_SIZE;
        y = (height / 2) * CELL_SIZE + CELL_SIZE / 2;
    } else if (randNum == 1) {
        x = (width / 2) * CELL_SIZE + CELL_SIZE / 2;
        y = (MAP_PADDING + 1) * CELL_SIZE;
    } else if (randNum == 2) {
        x = (width - MAP_PADDING - 1) * CELL_SIZE;
        y = (height / 2) * CELL_SIZE + CELL_SIZE / 2;
    } else {
        x = (width / 2) * CELL_SIZE + CELL_SIZE / 2;
        y = (height - MAP_PADDING - 1) * CELL_SIZE;
    }
    return (Point){.x = x, .y = y};
}

Point getBossSpawnPosition(void) {
    int width = roomLayouts[curRoomType].mapWidth;
    int height = roomLayouts[curRoomType].mapHeight;
    int x = (width - MAP_PADDING - 1) * CELL_SIZE;
    int y = (height / 2) * CELL_SIZE + CELL_SIZE / 2;
    return (Point){.x = x, .y = y};
}

Point getShopMapPosition(void) {
    return (Point){.x = MAP_WIDTH / 2, .y = MAP_PADDING - 1};
}

void carveOutShopArea(void) {
    setCell(MAP_WIDTH / 2, MAP_PADDING, MAP_FLOOR);
    setCell(MAP_WIDTH / 2 - 1, MAP_PADDING, MAP_FLOOR);
    setCell(MAP_WIDTH / 2 + 1, MAP_PADDING, MAP_FLOOR);
    setCell(MAP_WIDTH / 2 - 2, MAP_PADDING - 1, MAP_WALL);
    setCell(MAP_WIDTH / 2 - 2, MAP_PADDING - 2, MAP_WALL);
    setCell(MAP_WIDTH / 2 - 2, MAP_PADDING - 3, MAP_WALL);
    setCell(MAP_WIDTH / 2 - 1, MAP_PADDING - 3, MAP_WALL);
    setCell(MAP_WIDTH / 2 - 1, MAP_PADDING - 4, MAP_WALL);
    setCell(MAP_WIDTH / 2 + 2, MAP_PADDING - 1, MAP_WALL);
    setCell(MAP_WIDTH / 2 + 2, MAP_PADDING - 2, MAP_WALL);
    setCell(MAP_WIDTH / 2 + 2, MAP_PADDING - 3, MAP_WALL);
    setCell(MAP_WIDTH / 2 + 1, MAP_PADDING - 3, MAP_WALL);
    setCell(MAP_WIDTH / 2 + 1, MAP_PADDING - 4, MAP_WALL);
    setCell(MAP_WIDTH / 2, MAP_PADDING - 4, MAP_WALL);
}

void spawnExit(void) {
    Point exitMapPosition = getExitMapPosition();
    addInteractable(exitMapPosition.x, exitMapPosition.y, INTERACTABLE_TYPE_EXIT);
}

void generateSquareDungeon(void) {
    fillMap(MAP_FLOOR);
    int width = roomLayouts[curRoomType].mapWidth;
    int height = roomLayouts[curRoomType].mapHeight;
    encloseMapInWalls(width, height);

    if (curRoomType == ROOM_TYPE_CAMPFIRE) {
        addInteractable(width / 3, height / 2, INTERACTABLE_TYPE_CAMPFIRE);
        spawnExit();
    }
}

void getRandomEmptyMapPosition(int *mapX, int *mapY) {
    do {
        *mapX = randInt(2, MAP_WIDTH - 3);
        *mapY = randInt(2, MAP_HEIGHT - 3);
    } while (map[*mapY * MAP_WIDTH + *mapX] != MAP_FLOOR);
}

void initMap(RoomType roomType) {
    curRoomType = roomType;
    generateSquareDungeon();
    // printMap();
}

int isBlocked(int x, int y) {
    return map[y * MAP_WIDTH + x] != 0;
}

// From James Howard's Catacombs of the Damned: https://github.com/jhhoward/Arduboy3D
/*
MIT License

Copyright (c) 2019 James Howard

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
int isClearLine(int x1, int y1, int x2, int y2) {
    int cellX1 = x1 / CELL_SIZE;
	int cellX2 = x2 / CELL_SIZE;
	int cellY1 = y1 / CELL_SIZE;
	int cellY2 = y2 / CELL_SIZE;

    int xDist = ABS(cellX2 - cellX1);

	int partial, delta;
	int deltaFrac;
	int xFrac, yFrac;
	int xStep, yStep;
	int32_t oppositeAxisStep;
	int x, y;

    if (xDist > 0)
    {
        if (cellX2 > cellX1)
        {
            partial = (CELL_SIZE * (cellX1 + 1) - x1);
            xStep = 1;
        }
        else
        {
            partial = (x1 - CELL_SIZE * (cellX1));
            xStep = -1;
        }

        deltaFrac = ABS(x2 - x1);
        delta = y2 - y1;
        oppositeAxisStep = ((int32_t)delta * CELL_SIZE) / deltaFrac;
        if (oppositeAxisStep > 0x7fffl) {
            yStep = 0x7fff;
        } else if (oppositeAxisStep < -0x7fffl) {
            yStep = -0x7fff;
        } else {
            yStep = oppositeAxisStep;
        }
        yFrac = y1 + (((int32_t)yStep*partial) / CELL_SIZE);

        x = cellX1 + xStep;
        cellX2 += xStep;
        do
        {
            y = (yFrac) / CELL_SIZE;
            yFrac += yStep;

			if (isBlocked(x, y)) {
				return 0;
            }

            x += xStep;

        } while (x != cellX2);
    }

    int yDist = ABS(cellY2 - cellY1);

    if (yDist > 0)
    {
        if (cellY2 > cellY1)
        {
            partial = (CELL_SIZE * (cellY1 + 1) - y1);
            yStep = 1;
        }
        else
        {
            partial = (y1 - CELL_SIZE * (cellY1));
            yStep = -1;
        }

        deltaFrac = ABS(y2 - y1);
        delta = x2 - x1;
        oppositeAxisStep = ((int32_t)delta * CELL_SIZE) / deltaFrac;
        if (oppositeAxisStep > 0x7fffl) {
            xStep = 0x7fff;
        }
        else if (oppositeAxisStep < -0x7fffl) {
            xStep = -0x7fff;
        }
        else {
            xStep = oppositeAxisStep;
        }
        xFrac = x1 + (((int32_t)xStep * partial) / CELL_SIZE);

        y = cellY1 + yStep;
        cellY2 += yStep;
        do
        {
            x = (xFrac) / CELL_SIZE;
            xFrac += xStep;

			if (isBlocked(x, y)) {
				return 0;
            }
            y += yStep;

        } while (y != cellY2);
    }

    return 1;
}

void castRayToWall(float x1, float y1, float x2, float y2, float *targetX, float *targetY) {
    int maxSteps = 50;
    float rayX = x1;
    float rayY = y1;
    float dx = x2 - x1;
    float dy = y2 - y1;

    float length = sqrtf(dx * dx + dy * dy);
    if (length == 0) {
        *targetX = x2;
        *targetY = y2;
        return;
    }

    float stepSize = CELL_SIZE; // step in pixels
    float stepX = (dx / length) * stepSize;
    float stepY = (dy / length) * stepSize;

    for (int i = 0; i < maxSteps; i++) {
        int mapX = (int)rayX / CELL_SIZE;
        int mapY = (int)rayY / CELL_SIZE;
        if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT) {
            *targetX = rayX - stepX;
            *targetY = rayY - stepY;
            return;
        }

        int index = mapY * MAP_WIDTH + mapX;
        if (map[index] == 1) {
            // Wall hit, return previous step
            *targetX = rayX - stepX;
            *targetY = rayY - stepY;
            return;
        }

        rayX += stepX;
        rayY += stepY;
    }

    *targetX = rayX - stepX;
    *targetY = rayY - stepY;
    return;
}

#define MAX_FIELD_DEPTH 12
Point checkRayCollision(float startX, float startY, float angle, float distance) {
    float rayX = startX;
    float rayY = startY;
    float xOffset = 0;
    float yOffset = 0;
    int depthOfField = 0;
    int maxDistance = 10000000;

    float sinAngle, cosAngle;
    sincosf(angle, &sinAngle, &cosAngle);

    // === Check for horizontal lines ===
    float horizontalRayX, horizontalRayY;
    float horizontalDistance = maxDistance;
    float rayPerpendicularSlope = -cosAngle / sinAngle;

    if (angle == 0 || angle == PI) {
        // Looking straight left or right
        depthOfField = MAX_FIELD_DEPTH;
        rayX = startX;
        rayY = startY;
    } else if (angle > PI) {
        // Looking up
        rayY = (float)(((int)startY / CELL_SIZE) * CELL_SIZE - 0.0001f);
        rayX = (startY - rayY) * rayPerpendicularSlope + startX;
        yOffset = -CELL_SIZE;
        xOffset = -yOffset * rayPerpendicularSlope;
    } else if (angle < PI && angle > 0) {
        // Looking down
        rayY = (((int)startY / CELL_SIZE) * CELL_SIZE) + CELL_SIZE;
        rayX = (startY - rayY) * rayPerpendicularSlope + startX;
        yOffset = CELL_SIZE;
        xOffset = -yOffset * rayPerpendicularSlope;
    }

    while (depthOfField < MAX_FIELD_DEPTH) {
        int mapX = (int)rayX / CELL_SIZE;
        int mapY = (int)rayY / CELL_SIZE;
        int mapIndex = (mapY * MAP_WIDTH) + mapX;
        if ((mapIndex < 0 || mapIndex >= MAP_WIDTH * MAP_HEIGHT || mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT) || map[mapIndex] > 0) {
            // Hit wall
            depthOfField = MAX_FIELD_DEPTH;
            horizontalDistance = distanceSquared(startX, startY, rayX, rayY);
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
    float rayNegSlope = -sinAngle / cosAngle;

    if (angle == P2 || angle == P3) {
        // Looking straight up or down
        depthOfField = MAX_FIELD_DEPTH;
        rayX = startX;
        rayY = startY;
    } else if (angle > P2 && angle < P3) {
        // Looking left
        rayX = (float)(((int)startX / CELL_SIZE) * CELL_SIZE) - 0.0001f;
        rayY = (startX - rayX) * rayNegSlope + startY;
        xOffset = -CELL_SIZE;
        yOffset = -xOffset * rayNegSlope;
    } else if (angle < P2 || angle > P3) {
        // Looking right
        rayX = (((int)startX / CELL_SIZE) * CELL_SIZE) + CELL_SIZE;
        rayY = (startX - rayX) * rayNegSlope + startY;
        xOffset = CELL_SIZE;
        yOffset = -xOffset * rayNegSlope;
    }

    while (depthOfField < MAX_FIELD_DEPTH) {
        int mapX = (int)rayX / CELL_SIZE;
        int mapY = (int)rayY / CELL_SIZE;
        int mapIndex = (mapY * MAP_WIDTH) + mapX;
        if ((mapIndex < 0 || mapIndex >= MAP_WIDTH * MAP_HEIGHT || mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT) || map[mapIndex] > 0) {
            depthOfField = MAX_FIELD_DEPTH; // Hit wall
            verticalDistance = distanceSquared(startX, startY, rayX, rayY);
            verticalRayX = rayX;
            verticalRayY = rayY;
        } else {
            rayX += xOffset;
            rayY += yOffset;
            depthOfField += 1;
        }
    }

    if (distance * distance < verticalDistance && distance < horizontalDistance) {
        rayX = startX + cosAngle * distance;
        rayY = startY + sinAngle * distance;
    } else if (verticalDistance < horizontalDistance) {
        rayX = verticalRayX - cosAngle * CELL_SIZE;
        rayY = verticalRayY - sinAngle * CELL_SIZE;
    } else if (horizontalDistance < verticalDistance) {
        rayX = horizontalRayX - cosAngle * CELL_SIZE;
        rayY = horizontalRayY - sinAngle * CELL_SIZE;
    } else {
        rayX = verticalRayX;
        rayY = verticalRayY;
    }

    Point hitPoint = { rayX, rayY };
    return hitPoint;
}


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