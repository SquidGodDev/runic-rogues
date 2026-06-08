#pragma once

typedef enum Scene {
    SCENE_LOADING,
    SCENE_CLASS_SELECT,
    SCENE_TITLE,
    SCENE_GAME,
    SCENE_ROOM_MAP,
    SCENE_AREA_MAP,
    SCENE_GAME_OVER,
    SCENE_TUTORIAL
} Scene;

void switchScene(Scene newScene);
void initSceneManager(void);
int updateSceneManager(void* userdata);
bool isSceneTransitioning(void);