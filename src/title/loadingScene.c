#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/sceneManager.h"
#include "title/loadingScene.h"
#include "game/rendering/textures.h"
#include "game/systems/audioManager.h"

#define BAR_WIDTH 120
#define BAR_HEIGHT 8
#define BAR_X ((SCREEN_WIDTH - BAR_WIDTH) / 2)
#define BAR_Y ((SCREEN_HEIGHT - BAR_HEIGHT) / 2)
#define LABEL_Y (BAR_Y + BAR_HEIGHT + 4)
#define FRAME_BUDGET_MS (1000 / TARGET_FPS)

static int textureCount;
static int audioCount;
static int totalSteps;
static int stepsDone;
static bool finished;

void initLoadingScene(void) {
    textureCount = getTextureAssetCount();
    audioCount = getAudioAssetCount();
    totalSteps = textureCount + audioCount;
    stepsDone = 0;
    finished = false;
}

void updateLoadingScene(float deltaTime) {
    unsigned int frameStartMs = pd->system->getCurrentTimeMilliseconds();
    while (stepsDone < totalSteps) {
        if (stepsDone < textureCount) {
            loadTextureAsset(stepsDone);
        } else {
            loadAudioAsset(stepsDone - textureCount);
        }
        stepsDone++;
        if (pd->system->getCurrentTimeMilliseconds() - frameStartMs >= FRAME_BUDGET_MS) {
            break;
        }
    }

    pd->graphics->clear(kColorBlack);

    pd->graphics->drawRect(BAR_X, BAR_Y, BAR_WIDTH, BAR_HEIGHT, kColorWhite);
    int fillWidth = (BAR_WIDTH - 4) * stepsDone / totalSteps;
    if (fillWidth > 0) {
        pd->graphics->fillRect(BAR_X + 2, BAR_Y + 2, fillWidth, BAR_HEIGHT - 4, kColorWhite);
    }

    const char *label;
    if (stepsDone >= totalSteps) {
        label = "Loaded!";
    } else if (stepsDone < textureCount) {
        label = "Loading textures...";
    } else {
        label = "Loading audio...";
    }
    int labelLen = strlen(label);
    pd->graphics->setFont(fontSubEN);
    pd->graphics->setDrawMode(kDrawModeInverted);
    int labelWidth = pd->graphics->getTextWidth(fontSubEN, label, labelLen, kUTF8Encoding, 0);
    pd->graphics->drawText(label, labelLen, kUTF8Encoding, (SCREEN_WIDTH - labelWidth) / 2, LABEL_Y);
    pd->graphics->setDrawMode(kDrawModeCopy);

    if (!finished && stepsDone >= totalSteps) {
        finished = true;
        switchScene(SCENE_CLASS_SELECT);
    }
}
