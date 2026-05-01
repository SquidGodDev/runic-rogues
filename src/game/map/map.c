#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/helpers.h"
#include "game/map/map.h"
#include "game/entities/interactables.h"
#include "game/systems/progressionManager.h"
#include "game/entities/pickups.h"
#include "game/UI/UI.h"
#include "game/player/player.h"

#define PLAYDATE_SDK 1

// Campfire room layout (unchanged)
typedef struct RoomLayout {
    int mapWidth;
    int mapHeight;
    Point playerSpawnMapPosition;
    Point exitMapPosition;
} RoomLayout;

static RoomLayout campfireLayout = {
    .mapWidth = 14,
    .mapHeight = 5,
    .playerSpawnMapPosition = {.x = 1, .y = 2},
    .exitMapPosition = {.x = 10, .y = 2}
};

#define MAP_PADDING 4

// Multi-room dungeon data
typedef struct DungeonRoom {
    int interiorMinX, interiorMinY;
    int interiorMaxX, interiorMaxY;
    int centerX, centerY;
} DungeonRoom;

#define MAX_GATE_CELLS 15

typedef struct RoomGate {
    int cellCount;
    Point cells[MAX_GATE_CELLS];
} RoomGate;

static DungeonRoom dungeonRooms[DUNGEON_ROOM_COUNT];
static RoomGate roomGates[DUNGEON_ROOM_COUNT - 1];
static RoomGate entranceGate;

static RoomType curRoomType;

int map[MAP_WIDTH * MAP_HEIGHT];

void printMap(void) {
    for (int y = 0; y < MAP_HEIGHT; y++) {
        char rowString[MAP_WIDTH * 2 + 1];
        for (int x = 0; x < MAP_WIDTH; x++) {
            int mapValue = map[y * MAP_WIDTH + x];
            if (mapValue == MAP_GATE) {
                rowString[x*2] = 'G';
            } else if (mapValue == MAP_WALL) {
                rowString[x*2] = '#';
            } else {
                rowString[x*2] = '.';
            }
            rowString[x*2 + 1] = ' ';
        }
        rowString[MAP_WIDTH * 2] = '\0';
        #if PLAYDATE_SDK
            pd->system->logToConsole("%s", rowString);
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

static void carveRoom(int minX, int minY, int maxX, int maxY) {
    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            setCell(x, y, MAP_FLOOR);
        }
    }
}

void encloseMapInWalls(int width, int height) {
    for (int x = MAP_PADDING; x < width - MAP_PADDING; x++) {
        if (x == width / 2) {
            setCell(x, MAP_PADDING, MAP_GATE);
            setCell(x, height - MAP_PADDING - 1, MAP_GATE);
        } else {
            setCell(x, MAP_PADDING, MAP_WALL);
            setCell(x, height - MAP_PADDING - 1, MAP_WALL);
        }
    }
    for (int y = MAP_PADDING; y < height - MAP_PADDING; y++) {
        if (y == height / 2) {
            setCell(MAP_PADDING, y, MAP_GATE);
            setCell(width - MAP_PADDING - 1, y, MAP_GATE);
        } else {
            setCell(MAP_PADDING, y, MAP_WALL);
            setCell(width - MAP_PADDING - 1, y, MAP_WALL);
        }
    }
}

static void generateMultiRoomDungeon(void) {
    fillMap(MAP_WALL);

    // Start hallway: x=[1,5] floor, y=5. Entrance gate tracks x=5 to close behind player.
    for (int x = 1; x <= 5; x++) {
        setCell(x, 5, MAP_FLOOR);
    }
    entranceGate = (RoomGate){ .cellCount = 1 };
    entranceGate.cells[0] = (Point){.x = 5, .y = 5};

    // Non-boss rooms: 5 rooms of 9x9 interior, stride 14 (9 room + 5 hallway)
    int roomInteriorSize = 9;
    int hallwayLength = 5;
    for (int i = 0; i < NON_BOSS_ROOM_COUNT; i++) {
        int minX = 6 + i * (roomInteriorSize + hallwayLength);
        int minY = 1;
        int maxX = minX + roomInteriorSize - 1;
        int maxY = minY + roomInteriorSize - 1;
        dungeonRooms[i] = (DungeonRoom){
            .interiorMinX = minX, .interiorMinY = minY,
            .interiorMaxX = maxX, .interiorMaxY = maxY,
            .centerX = (minX + maxX) / 2, .centerY = (minY + maxY) / 2
        };
        carveRoom(minX, minY, maxX, maxY);
    }

    // Boss room: 15x15 interior, below Room 4 (minX=62, maxX=70, centerX=66, maxY=9)
    // Entrance hallway at y=[10,14], boss room starts at y=15
    dungeonRooms[DUNGEON_ROOM_COUNT - 1] = (DungeonRoom){
        .interiorMinX = 59, .interiorMinY = 15,
        .interiorMaxX = 73, .interiorMaxY = 29,
        .centerX = 66, .centerY = 22
    };
    carveRoom(59, 15, 73, 29);

    // Inter-room gates: 5-cell hallways (1 gate column + 4 wall columns), 1 cell wide at centerY
    for (int i = 0; i < NON_BOSS_ROOM_COUNT - 1; i++) {
        int gateX = dungeonRooms[i].interiorMaxX + 1;
        int gateY = dungeonRooms[i].centerY;
        int cellIdx = 0;
        RoomGate gate = { .cellCount = 0 };

        // Front column: MAP_GATE
        setCell(gateX, gateY, MAP_GATE);
        gate.cells[cellIdx++] = (Point){.x = gateX, .y = gateY};
        // Hallway columns: remain MAP_WALL, opened with gate
        for (int dx = 1; dx <= 4; dx++) {
            gate.cells[cellIdx++] = (Point){.x = gateX + dx, .y = gateY};
        }
        gate.cellCount = cellIdx;
        roomGates[i] = gate;
    }

    // Boss gate: 5-cell vertical hallway (1 gate row + 4 wall rows), 3 cells wide
    // Centered at x=66 (room 4 centerX), connecting room 4 bottom to boss room top
    {
        int hallMinX = 65;
        int hallMaxX = 67;
        int gateY = dungeonRooms[NON_BOSS_ROOM_COUNT - 1].interiorMaxY + 1;
        int cellIdx = 0;
        RoomGate gate = { .cellCount = 0 };

        // Front row: MAP_GATE
        for (int x = hallMinX; x <= hallMaxX; x++) {
            if (x == (hallMinX + hallMaxX) / 2) {
                setCell(x, gateY, MAP_GATE);
            } else {
                setCell(x, gateY, MAP_WALL);
            }
            gate.cells[cellIdx++] = (Point){.x = x, .y = gateY};
        }
        // Hallway rows: remain MAP_WALL, opened with gate
        for (int dy = 1; dy <= 4; dy++) {
            for (int x = hallMinX; x <= hallMaxX; x++) {
                gate.cells[cellIdx++] = (Point){.x = x, .y = gateY + dy};
            }
        }
        gate.cellCount = cellIdx;
        roomGates[NON_BOSS_ROOM_COUNT - 1] = gate;
    }
}

static void generateCampfireDungeon(void) {
    fillMap(MAP_FLOOR);
    int width = campfireLayout.mapWidth;
    int height = campfireLayout.mapHeight;
    encloseMapInWalls(width, height);
    addInteractable(width / 3, height / 2, INTERACTABLE_TYPE_CAMPFIRE);
    spawnExit();
}

Point getPlayerSpawnMapPosition(void) {
    if (curRoomType == ROOM_TYPE_CAMPFIRE) {
        return campfireLayout.playerSpawnMapPosition;
    }
    #if DEBUG_SKIP_TO_ROOM > 0
        DungeonRoom *room = &dungeonRooms[DEBUG_SKIP_TO_ROOM - 1];
        return (Point){.x = room->interiorMinX + 1, .y = room->centerY};
    #endif
    return (Point){.x = 1, .y = 5};
}

Point getExitMapPosition(void) {
    if (curRoomType == ROOM_TYPE_CAMPFIRE) {
        return campfireLayout.exitMapPosition;
    }
    // Exit in the boss room, offset south of center
    DungeonRoom *bossRoom = &dungeonRooms[DUNGEON_ROOM_COUNT - 1];
    return (Point){.x = bossRoom->centerX, .y = bossRoom->interiorMaxY - 1};
}

Point getShopMapPosition(void) {
    // Place shop at the north wall of the first room
    return (Point){.x = dungeonRooms[0].centerX, .y = dungeonRooms[0].interiorMinY - 1};
}

void spawnExit(void) {
    Point exitMapPosition = getRoomCenter(DUNGEON_ROOM_COUNT - 1);
    addInteractable(exitMapPosition.x, exitMapPosition.y, INTERACTABLE_TYPE_EXIT);
    setArrowPosition(exitMapPosition.x * CELL_SIZE + CELL_SIZE / 2, exitMapPosition.y * CELL_SIZE + CELL_SIZE / 2);
}

void getRandomEmptyMapPosition(int *mapX, int *mapY) {
    do {
        *mapX = randInt(2, MAP_WIDTH - 3);
        *mapY = randInt(2, MAP_HEIGHT - 3);
    } while (map[*mapY * MAP_WIDTH + *mapX] != MAP_FLOOR);
}

void initMap(void) {
    curRoomType = getCurrentRoomType();
    if (curRoomType == ROOM_TYPE_CAMPFIRE) {
        generateCampfireDungeon();
    } else {
        generateMultiRoomDungeon();
    }
    // printMap();
}

int isBlocked(int x, int y) {
    return map[y * MAP_WIDTH + x] != 0;
}

// --- Dungeon room query functions ---

int getDungeonRoomCount(void) {
    return DUNGEON_ROOM_COUNT;
}

Point getRoomCenter(int roomIndex) {
    return (Point){.x = dungeonRooms[roomIndex].centerX, .y = dungeonRooms[roomIndex].centerY};
}

Point getRoomWorldCenter(int roomIndex) {
    return (Point){
        .x = dungeonRooms[roomIndex].centerX * CELL_SIZE + CELL_SIZE / 2,
        .y = dungeonRooms[roomIndex].centerY * CELL_SIZE + CELL_SIZE / 2
    };
}

Point getRoomSpawnPosition(int roomIndex, int spawnSlot) {
    DungeonRoom *room = &dungeonRooms[roomIndex];
    // Anchor spawns at the far side of the room (away from the left-side entrance hallway)
    int fx = room->interiorMaxX - 2;
    int cy = room->centerY;
    int mapX, mapY;

    switch (spawnSlot) {
        case 0: mapX = fx;     mapY = cy;     break;
        case 1: mapX = fx - 1; mapY = cy - 1; break;
        case 2: mapX = fx + 1; mapY = cy - 1; break;
        case 3: mapX = fx - 1; mapY = cy + 1; break;
        case 4: mapX = fx + 1; mapY = cy + 1; break;
        case 5: mapX = fx - 2; mapY = cy;     break;
        case 6: mapX = fx + 2; mapY = cy;     break;
        case 7: mapX = fx;     mapY = cy - 2; break;
        case 8: mapX = fx;     mapY = cy + 2; break;
        default: {
            mapX = randInt((room->interiorMinX + room->interiorMaxX) / 2, room->interiorMaxX - 1);
            mapY = randInt(room->interiorMinY + 1, room->interiorMaxY - 1);
            break;
        }
    }

    // Clamp to interior
    if (mapX < room->interiorMinX) { mapX = room->interiorMinX; }
    if (mapX > room->interiorMaxX) { mapX = room->interiorMaxX; }
    if (mapY < room->interiorMinY) { mapY = room->interiorMinY; }
    if (mapY > room->interiorMaxY) { mapY = room->interiorMaxY; }

    return (Point){
        .x = mapX * CELL_SIZE + CELL_SIZE / 2,
        .y = mapY * CELL_SIZE + CELL_SIZE / 2
    };
}

void closeEntranceGate(void) {
    for (int i = 0; i < entranceGate.cellCount; i++) {
        setCell(entranceGate.cells[i].x, entranceGate.cells[i].y, MAP_WALL);
    }
}

void openRoomGate(int gateIndex) {
    RoomGate *gate = &roomGates[gateIndex];
    for (int i = 0; i < gate->cellCount; i++) {
        setCell(gate->cells[i].x, gate->cells[i].y, MAP_FLOOR);
    }
}

void closeRoomGate(int gateIndex) {
    RoomGate *gate = &roomGates[gateIndex];
    for (int i = 0; i < gate->cellCount; i++) {
        setCell(gate->cells[i].x, gate->cells[i].y, MAP_WALL);
    }
}

int getPlayerCurrentRoom(void) {
    int cellX = (int)playerX / CELL_SIZE;
    int cellY = (int)playerY / CELL_SIZE;
    for (int i = 0; i < DUNGEON_ROOM_COUNT; i++) {
        DungeonRoom *room = &dungeonRooms[i];
        if (cellX >= room->interiorMinX && cellX <= room->interiorMaxX &&
            cellY >= room->interiorMinY && cellY <= room->interiorMaxY) {
            return i;
        }
    }
    return -1;
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

#define MAX_FIELD_DEPTH 24
Point checkRayCollision(float startX, float startY, float angle, float distance) {
    float rayX = startX;
    float rayY = startY;
    float xOffset = 0;
    float yOffset = 0;
    int depthOfField = 0;
    int maxDistance = 10000000;

    float sinAngle = sinf(angle);
    float cosAngle = cosf(angle);

    // === Check for horizontal lines ===
    float horizontalRayX = 0.0f, horizontalRayY = 0.0f;
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
    float verticalRayX = 0.0f, verticalRayY = 0.0f;
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
