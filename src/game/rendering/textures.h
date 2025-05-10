#pragma once

#include "pd_api.h"
#include "globals.h"

extern uint8_t *echoingCavernsWall;
extern uint8_t *echoingCavernsCeiling;
extern uint8_t *echoingCavernsFloor;
extern uint8_t *lostArchivesWall;
extern uint8_t *lostArchivesCeiling;
extern uint8_t *lostArchivesFloor;
extern uint8_t *testWall;
extern uint8_t *testCeiling;
extern uint8_t *testFloor;

extern uint8_t *projectileTexture;
extern uint8_t *projectileBreakTexture;

extern uint8_t *enemyProjectileTexture;
extern uint8_t *batTexture;
extern uint8_t *slimeTexture;
extern uint8_t *babySlimeTexture;
extern uint8_t *skeletonTexture;
extern uint8_t *impTexture;
extern uint8_t *flyingTomeTexture;
extern uint8_t *livingArmorTexture;

typedef enum BitmapPaths {
	BITMAP_HAND,
	BITMAP_HAND_OPEN,
	BITMAP_FULL_HEART,
	BITMAP_HALF_HEART,
	BITMAP_EMPTY_HEART,
	BITMAP_MINIMAP_WALL,
	BITMAP_MINIMAP_PLAYER,
	BITMAP_MINIMAP_BORDER,
	BITMAP_MINIMAP_ENEMY,
	BITMAP_MINIMAP_PROJECTILE,
	BITMAP_PATHS_COUNT
} BitmapPaths;
extern LCDBitmap* BITMAPS[];

extern LCDFont *gameFont;

extern int fogPatterns[FOG_LEVELS][8];

void initTextures(void);