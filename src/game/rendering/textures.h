#pragma once

#include "pd_api.h"
#include "globals.h"

extern uint8_t wallTexture[256];
extern uint8_t floorTexture[256];
extern uint8_t ceilingTexture[256];
extern uint8_t projectileTexture[1024];
extern uint8_t projectileBreakTexture[22528];

extern uint8_t batTexture[6144];
extern uint8_t slimeTexture[1024];
extern uint8_t skeletonTexture[14336];

extern LCDBitmap *handBitmap;
extern LCDBitmap *handOpenBitmap;

extern LCDBitmap *fullHeartBitmap;
extern LCDBitmap *halfHeartBitmap;
extern LCDBitmap *emptyHeartBitmap;

extern LCDFont *gameFont;

extern int fogPatterns[FOG_LEVELS][8];

void initTextures(void);