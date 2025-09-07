#pragma once

typedef enum Scene {
    SCENE_TITLE,
    SCENE_GAME,
    SCENE_GAME_OVER
} Scene;

void switchScene(Scene newScene);
void initSceneManager(void);
int updateSceneManager(void* userdata);