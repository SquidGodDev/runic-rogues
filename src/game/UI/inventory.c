#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "game/UI/inventory.h"
#include "game/gameScene.h"
#include "game/rendering/textures.h"
#include "game/settings/settings.h"

LCDSprite *backgroundSprite;

void openInventoryCallback(void* userdata) {
	if (scene != SCENE_INVENTORY) {
		pd->system->removeAllMenuItems();
		initInventory();
		setScene(SCENE_INVENTORY);
	}
}

void createInventoryMenuItem(void) {
    pd->system->removeAllMenuItems();
	pd->system->addMenuItem("Inventory", openInventoryCallback, NULL);
}

void initInventory(void) {
	backgroundSprite = pd->sprite->newSprite();
	pd->sprite->moveTo(backgroundSprite, 100, 60);
	pd->sprite->setImage(backgroundSprite, BITMAPS[BITMAP_INVENTORY_BACKGROUND], kBitmapUnflipped);
	pd->sprite->addSprite(backgroundSprite);
}

void updateInventory(float deltaTime) {
	pd->sprite->updateAndDrawSprites();

	PDButtons pushed;
	pd->system->getButtonState(NULL, &pushed, NULL);
	if (pushed & kButtonB) {
		pd->sprite->removeAllSprites();
		pd->sprite->freeSprite(backgroundSprite);
		setScene(SCENE_GAME);
		createInventoryMenuItem();
		return;
	}
}