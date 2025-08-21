#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "game/gameScene.h"

int eventHandler(PlaydateAPI *playdate, PDSystemEvent event, uint32_t arg)
{
	if (event == kEventInitLua) {
		pd = playdate;
		pd->system->resetElapsedTime();
		pd->display->setScale(400 / SCREEN_WIDTH);
		pd->display->setRefreshRate(TARGET_FPS);

		srand(pd->system->getSecondsSinceEpoch(NULL));

		initSystems();
		initRunStart();
		pd->lua->addFunction(updateGameScene, "GameScene.update", NULL);
		pd->lua->addFunction(initGameScene, "GameScene.init", NULL);
	}

	return 0;
}
