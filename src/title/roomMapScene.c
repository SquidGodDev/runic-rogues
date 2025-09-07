#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/sceneManager.h"

#include "title/roomMapScene.h"
#include "game/rendering/textures.h"
#include "game/systems/progressionManager.h"

void initRoomMapScene(void) {
	incrementToNextRoom();
    switchScene(SCENE_GAME);
}

void updateRoomMapScene(float deltaTime) {
    // Nothing (for now)
}
