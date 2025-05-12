#define MIN_ROOM_SIZE 3
#define MAX_DEPTH 5
#define MAX_ROOM_SHRINK_SIZE 5
#define MIN_SPLIT_PERCENT 0.3f
#define MAX_SPLIT_PERCENT 0.7f
#define MIN_LARGE_ROOM_DEPTH 3
#define WALL 1
#define FLOOR 0

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
        printf("%s\n", rowString);
    }
}

void printTree(Node* node, int depth) {
    if (!node) return;

    for (int i = 0; i < depth; i++) printf("  "); // Indentation
    printf("Region: (%d,%d) %dx%d", node->region.x, node->region.y, node->region.width, node->region.height);

    if (!node->left && !node->right) {
        printf(" | Room: (%d,%d) %dx%d", node->room.x, node->room.y, node->room.width, node->room.height);
    }

    printf("\n");

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
    
    if (1) {
        for (int x = (ax < bx ? ax : bx); x <= (ax > bx ? ax : bx); x++) {
            setCell(x, ay, FLOOR);
        }
        for (int y = (ay < by ? ay : by); y <= (ay > by ? ay : by); y++) {
            setCell(bx, y, FLOOR);
        }
    } else {
        for (int y = (ay < by ? ay : by); y <= (ay > by ? ay : by); y++) {
            setCell(ax, y, FLOOR);
        }
        for (int x = (ax < bx ? ax : bx); x <= (ax > bx ? ax : bx); x++) {
            setCell(x, by, FLOOR);
        }
    }
}

Node* bspSplit(Rect region, int depth) {
    // Node* node = pd->system->realloc(NULL, sizeof(Node));
    Node* node = malloc(sizeof(Node));

    node->region = region;
    node->left = NULL;
    node->right = NULL;
    
    int largeRoom = (depth > MIN_LARGE_ROOM_DEPTH) && !(rand() % (MAX_DEPTH - depth + 1));

    if (largeRoom || depth >= MAX_DEPTH || region.width <= MIN_ROOM_SIZE || region.height <= MIN_ROOM_SIZE) {
        node->room = defineRoom(region);
        carveRoom(node->room);
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
    // pd->system->realloc(node, 0);
    free(node);
}

void generateDungeon(void) {
    fillMap(WALL);
    Rect rootRegion = {1, 1, MAP_WIDTH - 2, MAP_HEIGHT - 2};
    Node *dungeonRoot = bspSplit(rootRegion, 0 /* depth */);
    printMap();
    printTree(dungeonRoot, 0);
    freeTree(dungeonRoot);
}