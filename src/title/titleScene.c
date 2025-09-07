#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/sceneManager.h"

#include "title/titleScene.h"
#include "game/rendering/textures.h"

void initTitleScene(void) {
	// Nothing
}

void updateTitleScene(float deltaTime) {
	pd->graphics->drawBitmap(BITMAPS[BITMAP_TITLE_SCREEN_BACKGROUND], 0, 0, kBitmapUnflipped);

    PDButtons pushed;
    pd->system->getButtonState(NULL, &pushed, NULL);
    if (pushed & kButtonA) {
        switchScene(SCENE_GAME);
    }
}
