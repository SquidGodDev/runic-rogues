#pragma once

#include "pd_api.h"
#include "globals.h"

extern uint8_t wallTexture[256];
extern uint8_t floorTexture[256];
extern uint8_t ceilingTexture[256];
extern uint8_t enemyTexture[6144];
extern uint8_t projectileTexture[1024];

extern LCDBitmap *handBitmap;
extern LCDBitmap *handOpenBitmap;

extern LCDBitmap *fullHeartBitmap;
extern LCDBitmap *halfHeartBitmap;
extern LCDBitmap *emptyHeartBitmap;

extern int fogPatterns[FOG_LEVELS][8];

void initTextures(void);