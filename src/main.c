#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/sceneManager.h"
#include "globals/saveSystem.h"
#include "game/settings/settings.h"

int eventHandler(PlaydateAPI *playdate, PDSystemEvent event, uint32_t arg)
{
	if (event == kEventInit) {
		pd = playdate;
		pd->system->resetElapsedTime();
		pd->display->setScale(400 / SCREEN_WIDTH);
		pd->display->setRefreshRate(TARGET_FPS);

		srand(pd->system->getSecondsSinceEpoch(NULL));

		initSettings();
		initSaveSystem();
		initSceneManager();
		pd->system->setUpdateCallback(updateSceneManager, NULL);
	}

	return 0;
}
