#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/helpers.h"

#include "globals/sceneManager.h"
#include "title/loadingScene.h"
#include "title/classSelectScene.h"
#include "title/titleScene.h"
#include "title/roomMapScene.h"
#include "title/gameOverScene.h"
#include "game/gameScene.h"

#define TRANSITION_CENTER_X 100
#define TRANSITION_CENTER_Y 60
#define TRANSITION_TIME 0.5f // seconds
#define TRANSITION_WAIT_TIME 0.2f // seconds
#define TRANSITION_CIRCLE_BORDER 3 // pixels
#define MIN_RADIUS 0
#define MID_RADIUS 40
#define MAX_RADIUS 250

typedef enum TransitionState {
    TRANSITION_NONE,
    TRANSITION_EXITING_1,
    TRANSITION_EXITING_2,
    TRANSITION_ENTERING
} TransitionState;

static Scene currentScene;
static Scene nextScene;
static TransitionState transitionState;
static int transitionStartRadius;
static int transitionEndRadius;

static float transitionTimer;
LCDBitmap* transitionBitmap;
LCDBitmap* transitionMask;

void initSceneManager(void) {
    transitionTimer = 0;
    transitionState = TRANSITION_NONE;
    currentScene = SCENE_LOADING;
    initLoadingScene();
    pd->system->resetElapsedTime();
    transitionBitmap = pd->graphics->newBitmap(200, 120, kColorClear);
    transitionMask = pd->graphics->getBitmapMask(transitionBitmap);
}

void switchScene(Scene newScene) {
    nextScene = newScene;
    transitionState = TRANSITION_EXITING_1;
    transitionTimer = TRANSITION_TIME;
    transitionStartRadius = MAX_RADIUS;
    transitionEndRadius = MID_RADIUS;
    pd->graphics->clearBitmap(transitionBitmap, kColorClear);
}

void initNextScene(void) {
    currentScene = nextScene;
    switch (currentScene) {
        case SCENE_LOADING:
            initLoadingScene();
            break;
        case SCENE_CLASS_SELECT:
            initClassSelectScene();
            break;
        case SCENE_TITLE:
            initTitleScene();
            break;
        case SCENE_GAME:
            initGameScene();
            break;
        case SCENE_ROOM_MAP:
            initRoomMapScene();
            break;
        case SCENE_GAME_OVER:
            initGameOverScene();
            break;
    }
}

int updateSceneManager(void* userdata) {
    float deltaTime = pd->system->getElapsedTime(); // In seconds
	pd->system->resetElapsedTime();
    switch (currentScene) {
        case SCENE_LOADING:
            updateLoadingScene(deltaTime);
            break;
        case SCENE_CLASS_SELECT:
            updateClassSelectScene(deltaTime);
            break;
        case SCENE_TITLE:
            updateTitleScene(deltaTime);
            break;
        case SCENE_GAME:
            updateGameScene(deltaTime);
            break;
        case SCENE_ROOM_MAP:
            updateRoomMapScene(deltaTime);
            break;
        case SCENE_GAME_OVER:
            updateGameOverScene(deltaTime);
            break;
    }

    if (transitionTimer > 0) {
        float transitionValue;
        if (transitionState == TRANSITION_EXITING_1 || transitionState == TRANSITION_EXITING_2) {
            transitionValue = easeOutCubic(1.0f - transitionTimer / TRANSITION_TIME);
        } else {
            transitionValue = easeInCubic(1.0f - transitionTimer / TRANSITION_TIME);
        }

        int radius = lerp((float)transitionStartRadius, (float)transitionEndRadius, transitionValue);
        int borderWidth = radius > TRANSITION_CIRCLE_BORDER ? TRANSITION_CIRCLE_BORDER : radius;
        int drawRadius = radius + borderWidth;

        pd->graphics->clearBitmap(transitionBitmap, kColorBlack);
        pd->graphics->pushContext(transitionBitmap);
            pd->graphics->fillEllipse(TRANSITION_CENTER_X - drawRadius, TRANSITION_CENTER_Y - drawRadius, drawRadius * 2, drawRadius * 2, 0, 360, kColorWhite);
        pd->graphics->popContext();
        
        pd->graphics->clearBitmap(transitionMask, kColorWhite);
        pd->graphics->pushContext(transitionMask);
            pd->graphics->fillEllipse(TRANSITION_CENTER_X - radius, TRANSITION_CENTER_Y - radius, radius * 2, radius * 2, 0, 360, kColorBlack);
        pd->graphics->popContext();

        pd->graphics->drawBitmap(transitionBitmap, 0, 0, kBitmapUnflipped);

        transitionTimer -= deltaTime;
        if (transitionTimer < 0) {
            if (transitionState == TRANSITION_EXITING_1) {
                transitionState = TRANSITION_EXITING_2;
                transitionStartRadius = MID_RADIUS;
                transitionEndRadius = MIN_RADIUS;
                transitionTimer = TRANSITION_TIME;
            } else if (transitionState == TRANSITION_EXITING_2) {
                initNextScene();
                transitionState = TRANSITION_ENTERING;
                transitionStartRadius = MIN_RADIUS;
                transitionEndRadius = MAX_RADIUS;
                transitionTimer = TRANSITION_TIME;
            } else {
                transitionTimer = 0;
                transitionState = TRANSITION_NONE;
                pd->graphics->clearBitmap(transitionBitmap, kColorClear);
            }
        }
    }

    return 1;
}

bool isSceneTransitioning(void) {
    return transitionState == TRANSITION_EXITING_1 || transitionState == TRANSITION_EXITING_2;
}
