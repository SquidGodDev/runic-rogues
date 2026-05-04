#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/sceneManager.h"

#include "title/titleScene.h"
#include "game/gameScene.h"
#include "game/player/classes/classData.h"
#include "game/rendering/textures.h"

void initTitleScene(void) {
	// Nothing
}

void updateTitleScene(float deltaTime) {
	pd->graphics->drawBitmap(BITMAPS[BITMAP_TITLE_SCREEN_BACKGROUND], 0, 0, kBitmapUnflipped);

	pd->graphics->setFont(fontSubEN);
	pd->graphics->setDrawMode(kDrawModeFillWhite);
	const char *hint = "B: Tutorial";
	pd->graphics->drawText(hint, strlen(hint), kUTF8Encoding, 4, SCREEN_HEIGHT - 14);
	pd->graphics->setDrawMode(kDrawModeCopy);

    PDButtons pushed;
    pd->system->getButtonState(NULL, &pushed, NULL);
    if (isSceneTransitioning()) {
        return;
    }
    if (pushed & kButtonA) {
        initRunStart();
        switchScene(SCENE_GAME);
    } else if (pushed & kButtonB) {
        setCurrentClass(CLASS_MAGE);
        initRunStart();
        switchScene(SCENE_TUTORIAL);
    }
}
