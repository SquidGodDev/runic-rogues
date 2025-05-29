#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "helpers.h"
#include "game/entities/interactables.h"

#define PLAYDATE_SDK 1

#define MIN_ROOM_SIZE 5
#define MAX_DEPTH 5
#define MAX_ROOM_SHRINK_SIZE 5
#define MIN_SPLIT_PERCENT 0.3f
#define MAX_SPLIT_PERCENT 0.7f
#define MIN_LARGE_ROOM_DEPTH 3
#define WALL 1
#define FLOOR 0

int map[MAP_WIDTH * MAP_HEIGHT];
static int playerSpawnMapX, playerSpawnMapY;

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

void printTree(Node* node, int depth) {
    if (!node) return;

    for (int i = 0; i < depth; i++) pd->system->logToConsole("  "); // Indentation
    #if PLAYDATE_SDK 
        pd->system->logToConsole("Region: (%d,%d) %dx%d", node->region.x, node->region.y, node->region.width, node->region.height);
    #else
        printf("Region: (%d,%d) %dx%d", node->region.x, node->region.y, node->region.width, node->region.height);
    #endif

    if (!node->left && !node->right) {
        #if PLAYDATE_SDK
            pd->system->logToConsole(" | Room: (%d,%d) %dx%d", node->room.x, node->room.y, node->room.width, node->room.height);
        #else
            printf(" | Room: (%d,%d) %dx%d", node->room.x, node->room.y, node->room.width, node->room.height);
        #endif
    }

    #if PLAYDATE_SDK
        pd->system->logToConsole("\n");
    #else
        printf("\n");
    #endif

    printTree(node->left, depth + 1);
    printTree(node->right, depth + 1);
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

Rect defineRoom(Rect region) {
    int width = region.width; //randInt(region.width - MAX_ROOM_SHRINK_SIZE, region.width);
    int height = region.height; //randInt(region.height - MAX_ROOM_SHRINK_SIZE, region.height);
    int x = region.x; //+ rand() % (region.width - width + 1);
    int y = region.y; //+ rand() % (region.height - height + 1);
    return (Rect){x, y, width, height};
}

void carveRoom(Rect room) {
    for (int y = room.y; y < room.y + room.height; y++) {
        for (int x = room.x; x < room.x + room.width; x++) {
            setCell(x, y, FLOOR);
        }
    }
}

void carveHall(Rect roomA, Rect roomB) {
    // Room A connection point
    int ax = roomA.x + roomA.width / 2; //randInt(1, roomA.width - 1);
    int ay = roomA.y + roomA.height / 2; //randInt(1, roomA.height - 1);
    
    // Room A connection point
    int bx = roomB.x + roomB.width / 2; //randInt(1, roomB.width - 1);
    int by = roomB.y + roomB.height / 2; //randInt(1, roomB.height - 1);
    
    if (rand() % 2) {
        // Carve hallway horizontal then vertical
        for (int x = (ax < bx ? ax : bx); x <= (ax > bx ? ax : bx); x++) {
            setCell(x, ay, FLOOR);
        }
        for (int y = (ay < by ? ay : by); y <= (ay > by ? ay : by); y++) {
            setCell(bx, y, FLOOR);
        }
    } else {
        // Carve hallway vertical then horizontal
        for (int y = (ay < by ? ay : by); y <= (ay > by ? ay : by); y++) {
            setCell(ax, y, FLOOR);
        }
        for (int x = (ax < bx ? ax : bx); x <= (ax > bx ? ax : bx); x++) {
            setCell(x, by, FLOOR);
        }
    }
}

Node* bspSplit(Rect region, int depth) {
    #if PLAYDATE_SDK
        Node* node = pd->system->realloc(NULL, sizeof(Node));
    #else
        Node* node = malloc(sizeof(Node));
    #endif

    node->region = region;
    node->left = NULL;
    node->right = NULL;
    
    int largeRoom = (depth > MIN_LARGE_ROOM_DEPTH) && !(rand() % (MAX_DEPTH - depth + 1));

    if (largeRoom || depth >= MAX_DEPTH || region.width <= MIN_ROOM_SIZE || region.height <= MIN_ROOM_SIZE) {
        node->room = defineRoom(region);
        carveRoom(node->room);
        playerSpawnMapX = node->room.x + node->room.width / 2; 
        playerSpawnMapY = node->room.y + node->room.height / 2; 
        return node;
    }
    
    int splitHorizontally = region.height > region.width;
    float splitPercentage = randFloat(MIN_SPLIT_PERCENT, MAX_SPLIT_PERCENT);
    
    if (splitHorizontally) {
        int splitHeight = (int)(splitPercentage * region.height);
        if (splitHeight < MIN_ROOM_SIZE) {
            splitHeight = MIN_ROOM_SIZE;
        }
        node->left = bspSplit((Rect){region.x, region.y, region.width, splitHeight - 1}, depth + 1);
        node->right = bspSplit((Rect){region.x, region.y + splitHeight, region.width, region.height - splitHeight}, depth + 1);
    } else {
        int splitWidth = (int)(splitPercentage * region.width);
        if (splitWidth < MIN_ROOM_SIZE) {
            splitWidth = MIN_ROOM_SIZE;
        }
        node->left = bspSplit((Rect){region.x, region.y, splitWidth - 1, region.height}, depth + 1);
        node->right = bspSplit((Rect){region.x + splitWidth, region.y, region.width - splitWidth, region.height}, depth + 1);
    }
    
    carveHall(node->left->room, node->right->room);
    node->room = rand() % 2 ? node->left->room : node->right->room;
    
    return node;
}

void freeTree(Node* node) {
    if (!node) {
        return;
    }
    freeTree(node->left);
    freeTree(node->right);
    #if PLAYDATE_SDK
        pd->system->realloc(node, 0);
    #else
        free(node);
    #endif
}

void generateMazeDungeon(void) {
    fillMap(WALL);
    Rect rootRegion = {1, 1, MAP_WIDTH - 2, MAP_HEIGHT - 2};
    Node *dungeonRoot = bspSplit(rootRegion, 0 /* depth */);
    // printTree(dungeonRoot, 0);
    freeTree(dungeonRoot);
}

void encloseMapInWalls(void) {
    for (int x = 0; x < MAP_WIDTH; x++) {
        setCell(x, 0, WALL); // Top row
        setCell(x, MAP_HEIGHT - 1, WALL); // Bottom row
    }
    for (int y = 0; y < MAP_HEIGHT; y++) {
        setCell(0, y, WALL); // Left column
        setCell(MAP_WIDTH - 1, y, WALL); // Right column
    }
}

void generateSquareDungeon(void) {
    fillMap(FLOOR);
    encloseMapInWalls();
    playerSpawnMapX = MAP_WIDTH / 2;
    playerSpawnMapY = MAP_HEIGHT / 2;
    addInteractable(2, 2, INTERACTABLE_TYPE_EXIT);
}

void generateDungeon(void) {
    // generateMazeDungeon();
    generateSquareDungeon();

    printMap();
}

void initMap(int *playerSpawnX, int *playerSpawnY) {
    generateDungeon();
    *playerSpawnX = playerSpawnMapX * CELL_SIZE;
    *playerSpawnY = playerSpawnMapY * CELL_SIZE;
}

int isBlocked(int x, int y) {
    return map[y * MAP_WIDTH + x] != 0;
}

// From James Howard's Catacombs of the Damned: https://github.com/jhhoward/Arduboy3D
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