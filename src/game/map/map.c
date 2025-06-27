#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "helpers.h"
#include "game/entities/interactables.h"
#include "game/systems/progressionManager.h"

#define PLAYDATE_SDK 1

#define WALL 1
#define FLOOR 0

typedef struct RoomLayout {
    int mapWidth;
    int mapHeight;
    Point playerSpawnMapPosition;
    Point exitMapPosition;
} RoomLayout;

#define CAMPFIRE_ROOM_WIDTH MAP_WIDTH / 2
#define CAMPFIRE_ROOM_HEIGHT MAP_HEIGHT / 2

RoomLayout roomLayouts[ROOM_TYPE_COUNT] = {
    [ROOM_TYPE_ENEMY] = (RoomLayout){
        .mapWidth = MAP_WIDTH,
        .mapHeight = MAP_HEIGHT,
        .playerSpawnMapPosition = (Point){.x = 2, .y = MAP_HEIGHT / 2},
        .exitMapPosition = (Point){.x = MAP_WIDTH * 3 / 4, .y = MAP_HEIGHT / 2}
    },
    [ROOM_TYPE_ELITE] = (RoomLayout){
        .mapWidth = MAP_WIDTH,
        .mapHeight = MAP_HEIGHT,
        .playerSpawnMapPosition = (Point){.x = 2, .y = MAP_HEIGHT / 2},
        .exitMapPosition = (Point){.x = MAP_WIDTH * 3 / 4, .y = MAP_HEIGHT / 2}
    },
    [ROOM_TYPE_SHOP] = (RoomLayout){
        .mapWidth = 10,
        .mapHeight = 10,
        .playerSpawnMapPosition = (Point){.x = 2, .y = 5},
        .exitMapPosition = (Point){.x = 7, .y = 5}
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
        .playerSpawnMapPosition = (Point){.x = 2, .y = MAP_HEIGHT / 2},
        .exitMapPosition = (Point){.x = MAP_WIDTH * 3 / 4, .y = MAP_HEIGHT / 2}
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
    for (int x = 0; x < width; x++) {
        setCell(x, 0, WALL); // Top row
        setCell(x, height - 1, WALL); // Bottom row
    }
    for (int y = 0; y < height; y++) {
        setCell(0, y, WALL); // Left column
        setCell(width - 1, y, WALL); // Right column
    }
}

Point getPlayerSpawnMapPosition(void) {
    return roomLayouts[curRoomType].playerSpawnMapPosition;
}

Point getExitMapPosition(void) {
    return roomLayouts[curRoomType].exitMapPosition;
}

void spawnExit(void) {
    Point exitMapPosition = getExitMapPosition();
    addInteractable(exitMapPosition.x, exitMapPosition.y, INTERACTABLE_TYPE_EXIT);
    displayExitArrow(exitMapPosition.x * CELL_SIZE + CELL_SIZE / 2, exitMapPosition.y * CELL_SIZE + CELL_SIZE / 2);
}

void generateSquareDungeon(void) {
    fillMap(FLOOR);
    int width = roomLayouts[curRoomType].mapWidth;
    int height = roomLayouts[curRoomType].mapHeight;
    encloseMapInWalls(width, height);

    if (curRoomType == ROOM_TYPE_CAMPFIRE) {
        addInteractable(width / 3, height / 2, INTERACTABLE_TYPE_CAMPFIRE);
        spawnExit();
    } else if (curRoomType == ROOM_TYPE_SHOP) {
        spawnExit();
    }
}

void initMap(RoomType roomType) {
    curRoomType = roomType;
    generateSquareDungeon();
    printMap();
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