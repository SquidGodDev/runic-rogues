#pragma once
#include "pd_api.h"

#define PI 3.141592f
#define P2 PI / 2
#define P3 3 * PI / 2

#define TARGET_FPS 40
#define FOV 66
#define FOV_RAD FOV * (PI / 180.0f)
#define SCREEN_WIDTH 200
#define HALF_SCREEN_WIDTH SCREEN_WIDTH / 2
#define SCREEN_HEIGHT 120
#define HALF_SCREEN_HEIGHT SCREEN_HEIGHT / 2
#define CELL_SIZE 16
#define MAP_WIDTH 76
#define MAP_HEIGHT 32

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