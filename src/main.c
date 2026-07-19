#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/sceneManager.h"
#include "globals/saveSystem.h"
#include "globals/localization.h"
#include "game/settings/settings.h"
#include "game/systems/runeManager.h"
#include "game/player/spells/spells.h"
#include "game/player/classes/classData.h"

int eventHandler(PlaydateAPI *playdate, PDSystemEvent event, uint32_t arg)
{
	if (event == kEventInit) {
		pd = playdate;
		pd->system->resetElapsedTime();
		pd->display->setScale(400 / SCREEN_WIDTH);

		srand(pd->system->getSecondsSinceEpoch(NULL));

		initSettings();
		initLocalization();
		initRuneLocalization();
		initSpellLocalization();
		initClassLocalization();
		initSaveSystem();
		applyTargetFps();
		initSceneManager();
		pd->system->setUpdateCallback(updateSceneManager, NULL);
	}

	return 0;
}
