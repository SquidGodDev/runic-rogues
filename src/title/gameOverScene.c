#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/sceneManager.h"

#include "title/gameOverScene.h"
#include "game/rendering/textures.h"
#include "game/UI/runeInventory.h"

void initGameOverScene(void) {
	removeRuneInventoryMenuItem();
	freeRuneInventoryCache();
}

void updateGameOverScene(float deltaTime) {
	pd->graphics->drawBitmap(BITMAPS[BITMAP_GAME_OVER_SCREEN], 0, 0, kBitmapUnflipped);

    PDButtons pushed;
    pd->system->getButtonState(NULL, &pushed, NULL);
    if (pushed & kButtonA) {
        switchScene(SCENE_TITLE);
    }
}
