#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"

#include "globals/sceneManager.h"
#include "title/titleScene.h"
#include "title/gameOverScene.h"
#include "game/gameScene.h"

static Scene currentScene;

void switchScene(Scene newScene) {
    currentScene = newScene;
    switch (currentScene) {
        case SCENE_TITLE:
            initTitleScene();
            break;
        case SCENE_GAME:
            initGameScene();
            break;
        case SCENE_GAME_OVER:
            initGameOverScene();
            break;
    }
}

void initSceneManager(void) {
    currentScene = SCENE_TITLE;
    pd->system->resetElapsedTime();
}

int updateSceneManager(void* userdata) {
    float deltaTime = pd->system->getElapsedTime(); // In seconds
	pd->system->resetElapsedTime();
    switch (currentScene) {
        case SCENE_TITLE:
            updateTitleScene(deltaTime);
            break;
        case SCENE_GAME:
            updateGameScene(deltaTime);
            break;
        case SCENE_GAME_OVER:
            updateGameOverScene(deltaTime);
            break;
    }

    return 1;
}
