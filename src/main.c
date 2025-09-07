#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/sceneManager.h"
#include "game/gameScene.h"

int eventHandler(PlaydateAPI *playdate, PDSystemEvent event, uint32_t arg)
{
	if (event == kEventInit) {
		pd = playdate;
		pd->system->resetElapsedTime();
		pd->display->setScale(400 / SCREEN_WIDTH);
		pd->display->setRefreshRate(TARGET_FPS);

		srand(pd->system->getSecondsSinceEpoch(NULL));

		initSystems();
		initRunStart();
		initSceneManager();
		pd->system->setUpdateCallback(updateSceneManager, NULL);
	}

	return 0;
}
