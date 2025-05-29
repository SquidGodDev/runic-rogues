#define TARGET_EXTENSION 1
#include "pd_api.h"
#include "helpers.h"

void drawCenteredBitmap(LCDBitmap *bitmap, int x, int y) {
	int width, height;
	pd->graphics->getBitmapData(bitmap, &width, &height, NULL, NULL, NULL);
	pd->graphics->drawBitmap(bitmap, x - width / 2, y - height / 2, kBitmapUnflipped);
}