#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/helpers.h"
#include "globals/sceneManager.h"
#include "title/classSelectScene.h"
#include "game/player/classes/classData.h"
#include "game/gameScene.h"
#include "game/player/classes/classData.h"
#include "game/rendering/textures.h"

#define COL_Y 105
#define TITLE_Y 2
#define MODEL_Y_OFFSET 5

#define CLASS_3D_MODEL_FRAME_TIME 1.0f / TARGET_FPS
#define SLIDE_DURATION 0.35f

typedef struct ClassBitmapTableData {
    BitmapTablePath bitmapTablePath;
    int introEndFrame;
} ClassBitmapTableData;

static ClassBitmapTableData classBitmapTables[CLASS_COUNT] = {
    [CLASS_MAGE] = {
        .bitmapTablePath = BITMAP_TABLE_CLASS_SELECT_MAGE,
        .introEndFrame = 138
    },
    [CLASS_WARRIOR] = {
        .bitmapTablePath = BITMAP_TABLE_CLASS_SELECT_WARRIOR,
        .introEndFrame = 76
    },
    [CLASS_ROGUE] = {
        .bitmapTablePath = BITMAP_TABLE_CLASS_SELECT_ROGUE,
        .introEndFrame = 109
    },
    // [CLASS_RANGER] = {
    //     .bitmapTablePath = BITMAP_TABLE_CLASS_SELECT_RANGER,
    //     .introEndFrame = 121
    // },
};

typedef enum {
    CLASS_ANIM_DORMANT,
    CLASS_ANIM_INTRO,
    CLASS_ANIM_IDLE,
} ClassAnimPhase;

typedef struct Class3DModelData {
    LCDBitmapTable *bitmapTable;
    int width;
    int height;
    int introEndFrame;
    int currentFrame;
    int frameCount;
    float frameTime;
    float currentFrameTime;
    ClassAnimPhase phase;
} Class3DModelData;

static Class3DModelData class3DModels[CLASS_COUNT];

static int selectedIndex;
static float displaySelected;
static float slideStartValue;
static float slideTime;

void initClassSelectScene(void) {
    selectedIndex = 0;
    displaySelected = 0.0f;
    slideStartValue = 0.0f;
    slideTime = 0.0f;

    for (int i = 0; i < CLASS_COUNT; i++) {
        Class3DModelData *modelData = &class3DModels[i];
        modelData->bitmapTable = BITMAP_TABLES[classBitmapTables[i].bitmapTablePath];
        pd->graphics->getBitmapData(pd->graphics->getTableBitmap(modelData->bitmapTable, 0), &modelData->width, &modelData->height, NULL, NULL, NULL);
        modelData->introEndFrame = classBitmapTables[i].introEndFrame;
        modelData->currentFrame = 0;
        pd->graphics->getBitmapTableInfo(modelData->bitmapTable, &modelData->frameCount, NULL);
        modelData->frameTime = CLASS_3D_MODEL_FRAME_TIME;
        modelData->currentFrameTime = 0.0f;
        modelData->phase = CLASS_ANIM_DORMANT;
    }

    class3DModels[selectedIndex].phase = CLASS_ANIM_INTRO;
}

void updateClassSelectScene(float deltaTime) {
    pd->graphics->clear(kColorBlack);

    if (slideTime > 0.0f) {
        slideTime -= deltaTime;
        if (slideTime <= 0.0f) {
            slideTime = 0.0f;
            displaySelected = (float)selectedIndex;
        } else {
            float progress = 1.0f - (slideTime / SLIDE_DURATION);
            float eased = easeOutCubic(progress);
            displaySelected = slideStartValue + ((float)selectedIndex - slideStartValue) * eased;
        }
    }

    for (int i = 0; i < CLASS_COUNT; i++) {
        Class3DModelData *m = &class3DModels[i];
        if (m->phase == CLASS_ANIM_DORMANT) {
            continue;
        }
        m->currentFrameTime -= deltaTime;
        while (m->currentFrameTime <= 0.0f) {
            m->currentFrameTime += m->frameTime;
            m->currentFrame++;
            if (m->phase == CLASS_ANIM_INTRO) {
                if (m->currentFrame > m->introEndFrame - 1) {
                    m->phase = CLASS_ANIM_IDLE;
                    m->currentFrame = m->introEndFrame + 1;
                }
            } else {
                if (m->currentFrame > m->frameCount - 1) {
                    m->currentFrame = m->introEndFrame + 1;
                }
            }
        }
    }

    for (int i = 0; i < CLASS_COUNT; i++) {
        Class3DModelData *m = &class3DModels[i];
        float modelCenterX = (float)HALF_SCREEN_WIDTH + ((float)i - displaySelected) * (float)SCREEN_WIDTH;
        int drawX = (int)(modelCenterX - m->width / 2.0f);
        int drawY = (SCREEN_HEIGHT - m->height) / 2 + MODEL_Y_OFFSET;
        LCDBitmap *frame = pd->graphics->getTableBitmap(m->bitmapTable, m->currentFrame);
        pd->graphics->drawBitmap(frame, drawX, drawY, kBitmapUnflipped);
    }

    pd->graphics->setFont(fontTitleEN);
    pd->graphics->setDrawMode(kDrawModeInverted);

    const char *title = "Select Class";
    int titleWidth = pd->graphics->getTextWidth(fontTitleEN, title, strlen(title), kUTF8Encoding, 0);
    pd->graphics->drawText(title, strlen(title), kUTF8Encoding, (SCREEN_WIDTH - titleWidth) / 2, TITLE_Y);

    pd->graphics->setFont(fontSubEN);
    for (int i = 0; i < CLASS_COUNT; i++) {
        const char *name = classData[i].name;
        int nameWidth = pd->graphics->getTextWidth(fontSubEN, name, strlen(name), kUTF8Encoding, 0);
        float modelCenterX = (float)HALF_SCREEN_WIDTH + ((float)i - displaySelected) * (float)SCREEN_WIDTH;
        int textX = (int)(modelCenterX - nameWidth / 2.0f);
        pd->graphics->drawText(name, strlen(name), kUTF8Encoding, textX, COL_Y);
    }

    pd->graphics->setDrawMode(kDrawModeCopy);

    PDButtons pushed;
    pd->system->getButtonState(NULL, &pushed, NULL);

    if (!isSceneTransitioning()) {
        int newIndex = selectedIndex;
        if ((pushed & kButtonLeft) && selectedIndex > 0) {
            newIndex = selectedIndex - 1;
        } else if ((pushed & kButtonRight) && selectedIndex < CLASS_COUNT - 1) {
            newIndex = selectedIndex + 1;
        }
        if (newIndex != selectedIndex) {
            slideStartValue = displaySelected;
            selectedIndex = newIndex;
            slideTime = SLIDE_DURATION;
            Class3DModelData *m = &class3DModels[newIndex];
            m->phase = CLASS_ANIM_INTRO;
            m->currentFrame = 0;
            m->currentFrameTime = 0.0f;
        }
        if (pushed & kButtonA) {
            setCurrentClass((ClassType)selectedIndex);
            initRunStart();
            switchScene(SCENE_GAME);
        }
    }
}
