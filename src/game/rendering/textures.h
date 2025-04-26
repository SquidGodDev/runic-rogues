#pragma once

#include "pd_api.h"
#include "globals.h"

extern uint8_t *wallTexture;
extern uint8_t *floorTexture;
extern uint8_t *ceilingTexture;

extern uint8_t *projectileTexture;
extern uint8_t *projectileBreakTexture;

extern uint8_t *enemyProjectileTexture;
extern uint8_t *batTexture;
extern uint8_t *slimeTexture;
extern uint8_t *babySlimeTexture;
extern uint8_t *skeletonTexture;
extern uint8_t *impTexture;

extern LCDBitmap *handBitmap;
extern LCDBitmap *handOpenBitmap;

extern LCDBitmap *fullHeartBitmap;
extern LCDBitmap *halfHeartBitmap;
extern LCDBitmap *emptyHeartBitmap;

extern LCDFont *gameFont;

extern int fogPatterns[FOG_LEVELS][8];

void initTextures(void);