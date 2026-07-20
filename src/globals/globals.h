#pragma once
#include "pd_api.h"
#include <stdbool.h>

#define PI 3.141592f
#define P2 PI / 2
#define P3 3 * PI / 2

#define DEFAULT_TARGET_FPS 40
#define FOV 66
#define FOV_RAD FOV * (PI / 180.0f)
#define SCREEN_WIDTH 200
#define HALF_SCREEN_WIDTH SCREEN_WIDTH / 2
#define SCREEN_HEIGHT 120
#define HALF_SCREEN_HEIGHT SCREEN_HEIGHT / 2
#define CELL_SIZE 16
#define MAP_WIDTH 76
#define MAP_HEIGHT 32

// Runtime debug flags, adjustable in-game via the debug settings tab (unlocked
// with the title-screen code). Stored in the save file. `enabled` is the master
// toggle that reveals the debug tab and draws the DEBUG marker in every scene.
typedef struct DebugFlags {
    bool enabled;
    int skipToRoom;    // 0 = off; 1..4 = enemy rooms, 5 = boss room
    int areaOverride;  // 0 = off; 1 = Twilight Court, 2 = Lost Archives, 3 = Echoing Caverns
    bool instantRoomClear;
    bool instantKillBoss;
    bool startInTestArea;
    bool skipAreaMap;
    int testFirstRoomEnemy;  // 0 = off; 1..ENEMY_TYPE_COUNT = spawn (EnemyType)(value - 1) in the test area's first room
} DebugFlags;

extern DebugFlags DEBUG;

#define DEBUG_SKIP_TO_ROOM_MAX 5
#define DEBUG_AREA_OVERRIDE_MAX 3

#define FOG_LEVELS 16
#define FOG_STEP 6
#define FOG_MIN_DIST 60
#define FOG_MAX_DIST (FOG_MIN_DIST + FOG_STEP * (FOG_LEVELS - 1))

typedef struct Point {
	int x;
	int y;
} Point;

typedef struct FPoint {
	float x;
	float y;
} FPoint;

typedef enum RoomType {
    ROOM_TYPE_ENEMY,
    ROOM_TYPE_ELITE,
    ROOM_TYPE_CAMPFIRE,
    ROOM_TYPE_BOSS,
    ROOM_TYPE_COUNT
} RoomType;

extern PlaydateAPI *pd;