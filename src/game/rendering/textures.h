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

extern LCDBitmap *handBitmap;
extern LCDBitmap *handOpenBitmap;

extern LCDBitmap *fullHeartBitmap;
extern LCDBitmap *halfHeartBitmap;
extern LCDBitmap *emptyHeartBitmap;

extern LCDFont *gameFont;

extern int fogPatterns[FOG_LEVELS][8];

void initTextures(void);