#pragma once

#define PI 3.141592f
#define P2 PI / 2
#define P3 3 * PI / 2

#define TARGET_FPS 50
#define FOV 66
#define FOV_RAD FOV * (PI / 180.0f)
#define SCREEN_WIDTH 200
#define HALF_SCREEN_WIDTH SCREEN_WIDTH / 2
#define SCREEN_HEIGHT 120
#define HALF_SCREEN_HEIGHT SCREEN_HEIGHT / 2
#define CELL_SIZE 16
#define MAP_WIDTH 14
#define MAP_HEIGHT 14

#define FOG_LEVELS 16
#define FOG_STEP 6
#define FOG_MIN_DIST 60
#define FOG_MAX_DIST (FOG_MIN_DIST + FOG_STEP * (FOG_LEVELS - 1))

typedef struct Point {
	float x;
	float y;
} Point;

extern PlaydateAPI *pd;