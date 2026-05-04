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
#include "game/tutorial/tutorialScene.h"

// === Transition style selector ===
// Swap this to switch between transition effects.
#define TRANSITION_STYLE_CIRCLE 0
#define TRANSITION_STYLE_FADE   1
#define TRANSITION_STYLE TRANSITION_STYLE_FADE

// === Common timing ===
#define TRANSITION_TIME 0.5f // seconds (per phase)
#define TRANSITION_OUT_TIME 1.2f
#define TRANSITION_WAIT_TIME 0.2f // seconds

// === Circle transition tuning ===
#define TRANSITION_CENTER_X 100
#define TRANSITION_CENTER_Y 60
#define TRANSITION_CIRCLE_BORDER 3 // pixels
#define MIN_RADIUS 0
#define MID_RADIUS 40
#define MAX_RADIUS 250

// === Fade transition tuning ===
// Easing applied to the fade darkness (0 -> 1). Swap to easeInCubic / easeOutCubic /
// linearEasing / etc. for a different feel.
static inline float linearEasing(float x) { return x; }
#define FADE_IN_EASING easeInOutQuad
#define FADE_OUT_EASING easeInCubic
// FADE_RAMP_WIDTH: how soft/wide the gradient between fully-dark and fully-clear regions is.
// 1.0 = uniform fade (no closing-in). 0.0 = hard circular wipe with no dither softness.
// Around 0.5–0.8 gives a soft closure.
#define FADE_RAMP_WIDTH 0.5f

typedef enum TransitionState {
    TRANSITION_NONE,
    TRANSITION_EXITING_1,
    TRANSITION_EXITING_2,
    TRANSITION_WAIT,
    TRANSITION_ENTERING
} TransitionState;

// A single scene-fade animation. Two slots run in parallel so a fresh switchScene()
// during ENTERING can overlay its EXITING on top of the still-playing ENTERING.
typedef struct Transition {
    TransitionState state;
    float timer;
    int startRadius;
    int endRadius;
} Transition;

static Scene currentScene;
static Scene nextScene;
static Transition primary;   // Cycles through all phases.
static Transition trailing;  // Holds an ENTERING preempted by switchScene; otherwise NONE.

LCDBitmap* transitionBitmap;
LCDBitmap* transitionMask;

// Fade transition state — per-pixel threshold table baking together distance-from-center
// and 8x8 Bayer offset. Each entry is the global darkness byte (0..255) at which that
// pixel flips to black. Computed once at init.
static uint8_t fadeThreshold[SCREEN_HEIGHT][SCREEN_WIDTH];
static uint8_t* fadeBitmapMask = NULL;
static int fadeBitmapRowbytes = 0;

// 8x8 Bayer ordered dither matrix, values 0..63.
static const uint8_t bayer8x8[8][8] = {
    { 0, 32,  8, 40,  2, 34, 10, 42},
    {48, 16, 56, 24, 50, 18, 58, 26},
    {12, 44,  4, 36, 14, 46,  6, 38},
    {60, 28, 52, 20, 62, 30, 54, 22},
    { 3, 35, 11, 43,  1, 33,  9, 41},
    {51, 19, 59, 27, 49, 17, 57, 25},
    {15, 47,  7, 39, 13, 45,  5, 37},
    {63, 31, 55, 23, 61, 29, 53, 21}
};

static void buildFadeThresholdTable(void) {
    const float cx = SCREEN_WIDTH * 0.5f;
    const float cy = SCREEN_HEIGHT * 0.5f;
    const float maxD = sqrtf(cx * cx + cy * cy);
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            float dx = (float)x - cx;
            float dy = (float)y - cy;
            float dNorm = sqrtf(dx * dx + dy * dy) / maxD;
            if (dNorm > 1.0f) {
                dNorm = 1.0f;
            }
            float t = (1.0f - dNorm) * (1.0f - FADE_RAMP_WIDTH)
                    + FADE_RAMP_WIDTH * (bayer8x8[y & 7][x & 7] / 64.0f);
            int b = (int)(t * 255.0f + 0.5f);
            if (b < 0) {
                b = 0;
            }
            if (b > 255) {
                b = 255;
            }
            fadeThreshold[y][x] = (uint8_t)b;
        }
    }
}

void initSceneManager(void) {
    primary.state = TRANSITION_NONE;
    primary.timer = 0;
    trailing.state = TRANSITION_NONE;
    trailing.timer = 0;
    currentScene = SCENE_LOADING;
    initLoadingScene();
    pd->system->resetElapsedTime();
    transitionBitmap = pd->graphics->newBitmap(SCREEN_WIDTH, SCREEN_HEIGHT, kColorClear);
    transitionMask = pd->graphics->getBitmapMask(transitionBitmap);

    #if TRANSITION_STYLE == TRANSITION_STYLE_FADE
        pd->graphics->clearBitmap(transitionBitmap, kColorBlack);
        int w, h;
        uint8_t* dataPtr = NULL;
        pd->graphics->getBitmapData(transitionBitmap, &w, &h, &fadeBitmapRowbytes, &fadeBitmapMask, &dataPtr);
        buildFadeThresholdTable();
    #endif
}

void switchScene(Scene newScene) {
    nextScene = newScene;

    // If we interrupted a scene-in fade, let it keep playing in the trailing slot
    // so it composites under the new EXITING. Any older trailing is discarded.
    if (primary.state == TRANSITION_ENTERING) {
        trailing = primary;
    }

    primary.state = TRANSITION_EXITING_1;
    primary.timer = TRANSITION_TIME;
    primary.startRadius = MAX_RADIUS;
    primary.endRadius = MID_RADIUS;
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
        case SCENE_TUTORIAL:
            initTutorialScene();
            break;
    }
}

#if TRANSITION_STYLE == TRANSITION_STYLE_CIRCLE
// Returns the clear-circle radius for a transition. NONE returns MAX_RADIUS so it
// doesn't darken the composite (we combine via min-radius).
static int computeCircleRadius(const Transition* t) {
    if (t->state == TRANSITION_NONE) {
        return MAX_RADIUS;
    }
    if (t->state == TRANSITION_WAIT) {
        return MIN_RADIUS;
    }
    float phaseDuration = (t->state == TRANSITION_ENTERING) ? TRANSITION_OUT_TIME : TRANSITION_TIME;
    float progress = 1.0f - t->timer / phaseDuration;
    float eased = (t->state == TRANSITION_ENTERING) ? easeInCubic(progress) : easeOutCubic(progress);
    return (int)lerp((float)t->startRadius, (float)t->endRadius, eased);
}

static void drawCircleTransition(void) {
    int r1 = computeCircleRadius(&primary);
    int r2 = computeCircleRadius(&trailing);
    int radius = r1 < r2 ? r1 : r2;

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
}
#else
// Returns the global darkness (0..1) contribution of a single transition.
static float computeFadeDarkness(const Transition* t) {
    switch (t->state) {
        case TRANSITION_NONE:
            return 0.0f;
        case TRANSITION_EXITING_1: {
            float d = 0.5f * (1.0f - t->timer / TRANSITION_TIME);
            return FADE_IN_EASING(d);
        }
        case TRANSITION_EXITING_2: {
            float d = 0.5f + 0.5f * (1.0f - t->timer / TRANSITION_TIME);
            return FADE_IN_EASING(d);
        }
        case TRANSITION_WAIT:
            return 1.0f;
        case TRANSITION_ENTERING: {
            float d = t->timer / TRANSITION_OUT_TIME;
            return FADE_OUT_EASING(d);
        }
    }
    return 0.0f;
}

static void drawFadeTransition(void) {
    float d1 = computeFadeDarkness(&primary);
    float d2 = computeFadeDarkness(&trailing);
    float darkness = d1 > d2 ? d1 : d2;

    if (darkness <= 0.0f) {
        return;
    }
    if (darkness >= 1.0f) {
        pd->graphics->fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, kColorBlack);
        return;
    }

    int darknessByte = (int)(darkness * 255.0f + 0.5f);

    // Walk the threshold table and rebuild the bitmap mask. Pixel becomes opaque
    // (drawn black) when global darkness exceeds that pixel's threshold.
    const int bytesPerRow = SCREEN_WIDTH / 8;
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        const uint8_t* thresholdRow = fadeThreshold[y];
        uint8_t* maskRow = fadeBitmapMask + y * fadeBitmapRowbytes;
        for (int byteIdx = 0; byteIdx < bytesPerRow; byteIdx++) {
            int xBase = byteIdx * 8;
            uint8_t maskByte = 0;
            if (darknessByte > thresholdRow[xBase + 0]) maskByte |= 0x80;
            if (darknessByte > thresholdRow[xBase + 1]) maskByte |= 0x40;
            if (darknessByte > thresholdRow[xBase + 2]) maskByte |= 0x20;
            if (darknessByte > thresholdRow[xBase + 3]) maskByte |= 0x10;
            if (darknessByte > thresholdRow[xBase + 4]) maskByte |= 0x08;
            if (darknessByte > thresholdRow[xBase + 5]) maskByte |= 0x04;
            if (darknessByte > thresholdRow[xBase + 6]) maskByte |= 0x02;
            if (darknessByte > thresholdRow[xBase + 7]) maskByte |= 0x01;
            maskRow[byteIdx] = maskByte;
        }
    }

    pd->graphics->drawBitmap(transitionBitmap, 0, 0, kBitmapUnflipped);
}
#endif

static void advanceTransition(Transition* t, float deltaTime) {
    if (t->state == TRANSITION_NONE) {
        return;
    }
    t->timer -= deltaTime;
    if (t->timer >= 0) {
        return;
    }

    switch (t->state) {
        case TRANSITION_EXITING_1:
            t->state = TRANSITION_EXITING_2;
            t->startRadius = MID_RADIUS;
            t->endRadius = MIN_RADIUS;
            t->timer = TRANSITION_TIME;
            break;
        case TRANSITION_EXITING_2:
            t->state = TRANSITION_WAIT;
            t->startRadius = MIN_RADIUS;
            t->endRadius = MAX_RADIUS;
            t->timer = TRANSITION_WAIT_TIME;
            break;
        case TRANSITION_WAIT:
            initNextScene();
            t->state = TRANSITION_ENTERING;
            t->startRadius = MIN_RADIUS;
            t->endRadius = MAX_RADIUS;
            t->timer = TRANSITION_OUT_TIME;
            break;
        case TRANSITION_ENTERING:
        case TRANSITION_NONE:
            t->timer = 0;
            t->state = TRANSITION_NONE;
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
        case SCENE_TUTORIAL:
            updateTutorialScene(deltaTime);
            break;
    }

    if (primary.state != TRANSITION_NONE || trailing.state != TRANSITION_NONE) {
        #if TRANSITION_STYLE == TRANSITION_STYLE_CIRCLE
            drawCircleTransition();
        #else
            drawFadeTransition();
        #endif
    }

    advanceTransition(&primary, deltaTime);
    advanceTransition(&trailing, deltaTime);

    return 1;
}

bool isSceneTransitioning(void) {
    return primary.state == TRANSITION_EXITING_1 || primary.state == TRANSITION_EXITING_2;
}
