#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/helpers.h"
#include "globals/sceneManager.h"

#include "title/areaMapScene.h"
#include "game/rendering/textures.h"
#include "game/systems/progressionManager.h"
#include "game/systems/audioManager.h"
#include "game/player/classes/classData.h"

#define MAP_AREA_COUNT 3
#define MAP_LINE_COUNT (MAP_AREA_COUNT - 1)

#define CORNER_SIZE 12
#define ICON_SIZE 18
#define AREA_ICON_Y 56
#define CLASS_ICON_Y 31
#define LINE_Y 64
#define LINE_HEIGHT 2
#define LINE_WIDTH 37

#define REVEAL_DURATION 0.5f
#define REVEAL_STAGGER 0.1f
#define REVEAL_DRIFT 10.0f
#define INTRO_START_DELAY_TIME 1.0f
#define CLASS_REVEAL_START (REVEAL_STAGGER * 4.0f + 1.5f)
#define CLASS_SLIDE_DURATION 0.75f
#define CLASS_SLIDE_START_X (-(float)ICON_SIZE)
#define INTRO_REVEAL_TIME (CLASS_REVEAL_START + CLASS_SLIDE_DURATION)
#define INTRO_PRE_POP_WAIT_TIME 0.5f
#define INTRO_END_WAIT_TIME 1.0f

// Reveal fade settles on a dither stencil rather than full opacity
#define DITHER_REVEAL_OPACITY 0.5f
#define AREA_DITHER_OPACITY 0.6f

#define ADVANCE_START_DELAY_TIME 0.5f
#define ADVANCE_MOVE_TIME 1.0f
#define POP_TIME 0.3f
#define POP_HEIGHT 3.0f
#define END_WAIT_TIME 1.0f

#define FADE_STENCIL_COUNT 5

static const int areaIconX[MAP_AREA_COUNT] = {30, 91, 152};
static const int lineX[MAP_LINE_COUNT] = {51, 112};

// Reveal start offsets, interleaved left to right: area0, line0, area1, line1, area2
static const float areaRevealStart[MAP_AREA_COUNT] = {0.0f, 0.4f, 0.8f};
static const float lineRevealStart[MAP_LINE_COUNT] = {0.2f, 0.6f};

static const BitmapPath areaIconBitmaps[MAP_AREA_COUNT] = {
    BITMAP_AREA_MAP_TWILIGHT_COURT_ICON,
    BITMAP_AREA_MAP_LOST_ARCHIVES_ICON,
    BITMAP_AREA_MAP_ECHOING_CAVERNS_ICON
};

static const BitmapPath classIconBitmaps[CLASS_COUNT] = {
    [CLASS_MAGE] = BITMAP_AREA_MAP_MAGE_ICON,
    [CLASS_KNIGHT] = BITMAP_AREA_MAP_KNIGHT_ICON,
    [CLASS_ROGUE] = BITMAP_AREA_MAP_ROGUE_ICON,
    [CLASS_RANGER] = BITMAP_AREA_MAP_RANGER_ICON
};

// Bayer dither densities from ~87.5% down to ~12.5%, used as an opacity ramp
static const uint8_t fadeStencilPatterns[FADE_STENCIL_COUNT][8] = {
    {0xFF, 0xDD, 0xFF, 0x77, 0xFF, 0xDD, 0xFF, 0x77},
    {0xFF, 0x55, 0xFF, 0x55, 0xFF, 0x55, 0xFF, 0x55},
    {0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55},
    {0xAA, 0x00, 0xAA, 0x00, 0xAA, 0x00, 0xAA, 0x00},
    {0x88, 0x00, 0x22, 0x00, 0x88, 0x00, 0x22, 0x00}
};

typedef enum MapPhase {
    MAP_PHASE_INTRO_START_DELAY,
    MAP_PHASE_INTRO_REVEAL,
    MAP_PHASE_INTRO_PRE_POP_WAIT,
    MAP_PHASE_INTRO_POP,
    MAP_PHASE_INTRO_END_WAIT,
    MAP_PHASE_ADVANCE_START_DELAY,
    MAP_PHASE_ADVANCE_MOVE,
    MAP_PHASE_ADVANCE_POP,
    MAP_PHASE_ADVANCE_END_WAIT,
    MAP_PHASE_DONE
} MapPhase;

typedef struct AreaIconState {
    bool visible;
    bool solid;
} AreaIconState;

static LCDBitmap *fadeStencils[FADE_STENCIL_COUNT] = {NULL};

static MapPhase phase;
static float phaseTimer;
static float phaseDuration;
static int step; // Advance target area index
static int popIndex;
static AreaIconState areas[MAP_AREA_COUNT];
static float areaReveal[MAP_AREA_COUNT];
static float lineReveal[MAP_LINE_COUNT];
static float lineDitherWidth[MAP_LINE_COUNT];
static float lineSolidWidth[MAP_LINE_COUNT];
static bool classVisible;
static float classX;
static float classYOffset;
static float classOpacity;
static float popOffset;

static LCDBitmap *buildPatternStencil(const uint8_t pattern[8]) {
    LCDBitmap *bmp = pd->graphics->newBitmap(32, 8, kColorWhite);
    int w, h, rowbytes;
    uint8_t *mask = NULL;
    uint8_t *data = NULL;
    pd->graphics->getBitmapData(bmp, &w, &h, &rowbytes, &mask, &data);
    for (int y = 0; y < 8; y++) {
        for (int b = 0; b < rowbytes; b++) {
            data[y * rowbytes + b] = pattern[y];
        }
    }
    return bmp;
}

static void buildStencils(void) {
    if (fadeStencils[0] != NULL) {
        return;
    }
    for (int i = 0; i < FADE_STENCIL_COUNT; i++) {
        fadeStencils[i] = buildPatternStencil(fadeStencilPatterns[i]);
    }
}

static void setPhase(MapPhase newPhase, float duration) {
    phase = newPhase;
    phaseTimer = duration;
    phaseDuration = duration;
}

// Picks the stencil index for a partial opacity, or -1 for fully opaque / -2 for invisible
static int opacityStencilIndex(float opacity) {
    if (opacity <= 0.0f) {
        return -2;
    }
    int level = (int)(opacity * (FADE_STENCIL_COUNT + 1));
    if (level >= FADE_STENCIL_COUNT + 1) {
        return -1;
    }
    if (level < 1) {
        level = 1;
    }
    return FADE_STENCIL_COUNT - level; // denser stencil as level rises
}

static void drawIconOpacity(LCDBitmap *icon, int x, int y, float opacity) {
    int stencilIndex = opacityStencilIndex(opacity);
    if (stencilIndex == -2) {
        return;
    }
    if (stencilIndex == -1) {
        pd->graphics->drawBitmap(icon, x, y, kBitmapUnflipped);
        return;
    }
    pd->graphics->pushContext(NULL);
        pd->graphics->setStencilImage(fadeStencils[stencilIndex], 1);
        pd->graphics->drawBitmap(icon, x, y, kBitmapUnflipped);
    pd->graphics->popContext();
}

static void drawLineOpacity(int x, int y, int width, float opacity) {
    int stencilIndex = opacityStencilIndex(opacity);
    if (stencilIndex == -2) {
        return;
    }
    if (stencilIndex == -1) {
        pd->graphics->fillRect(x, y, width, LINE_HEIGHT, kColorWhite);
        return;
    }
    pd->graphics->pushContext(NULL);
        pd->graphics->setStencilImage(fadeStencils[stencilIndex], 1);
        pd->graphics->fillRect(x, y, width, LINE_HEIGHT, kColorWhite);
    pd->graphics->popContext();
}

void initAreaMapScene(void) {
    if (getCurrentSong() == MUSIC_TITLE) {
        stopSong();
    }

    buildStencils();

    classVisible = false;
    classYOffset = 0.0f;
    classOpacity = 1.0f;
    popOffset = 0.0f;
    step = 0;
    for (int i = 0; i < MAP_AREA_COUNT; i++) {
        areaReveal[i] = 1.0f;
    }
    for (int i = 0; i < MAP_LINE_COUNT; i++) {
        lineReveal[i] = 1.0f;
    }

    Area area = getCurrentArea();
    if (area != AREA_TWILIGHT_COURT && area != AREA_LOST_ARCHIVES && area != AREA_ECHOING_CAVERNS) {
        // Tutorial/test areas have no spot on the map
        for (int i = 0; i < MAP_AREA_COUNT; i++) {
            areas[i] = (AreaIconState){.visible = false, .solid = false};
        }
        for (int i = 0; i < MAP_LINE_COUNT; i++) {
            lineDitherWidth[i] = 0.0f;
            lineSolidWidth[i] = 0.0f;
        }
        phase = MAP_PHASE_DONE;
        switchScene(SCENE_GAME);
        return;
    }

    // 0 = current area on run start; 1/2 = area being advanced to
    int target = 0;
    if (area == AREA_LOST_ARCHIVES) {
        target = 1;
    } else if (area == AREA_ECHOING_CAVERNS) {
        target = 2;
    }
    step = target;
    popIndex = target;

    if (target == 0) {
        // Run start: icons/lines drift up and fade in dithered left to right, the
        // class icon slides in, then the first area icon pops to solid
        for (int i = 0; i < MAP_AREA_COUNT; i++) {
            areas[i] = (AreaIconState){.visible = true, .solid = false};
            areaReveal[i] = 0.0f;
        }
        for (int i = 0; i < MAP_LINE_COUNT; i++) {
            lineDitherWidth[i] = LINE_WIDTH;
            lineSolidWidth[i] = 0.0f;
            lineReveal[i] = 0.0f;
        }
        classVisible = true;
        classOpacity = 1.0f;
        classYOffset = 0.0f;
        classX = CLASS_SLIDE_START_X;
        setPhase(MAP_PHASE_INTRO_START_DELAY, INTRO_START_DELAY_TIME);
    } else {
        for (int i = 0; i < MAP_AREA_COUNT; i++) {
            areas[i].visible = true;
            areas[i].solid = i < target;
        }
        for (int i = 0; i < MAP_LINE_COUNT; i++) {
            lineDitherWidth[i] = LINE_WIDTH;
            lineSolidWidth[i] = (i < target - 1) ? LINE_WIDTH : 0.0f;
        }
        classVisible = true;
        classX = areaIconX[target - 1];
        setPhase(MAP_PHASE_ADVANCE_START_DELAY, ADVANCE_START_DELAY_TIME);
    }
}

static void advancePhase(void) {
    switch (phase) {
        case MAP_PHASE_INTRO_START_DELAY:
            setPhase(MAP_PHASE_INTRO_REVEAL, INTRO_REVEAL_TIME);
            break;
        case MAP_PHASE_INTRO_REVEAL:
            for (int i = 0; i < MAP_AREA_COUNT; i++) {
                areaReveal[i] = 1.0f;
            }
            for (int i = 0; i < MAP_LINE_COUNT; i++) {
                lineReveal[i] = 1.0f;
            }
            classX = areaIconX[0];
            classOpacity = 1.0f;
            classYOffset = 0.0f;
            setPhase(MAP_PHASE_INTRO_PRE_POP_WAIT, INTRO_PRE_POP_WAIT_TIME);
            break;
        case MAP_PHASE_INTRO_PRE_POP_WAIT:
            areas[popIndex].solid = true;
            setPhase(MAP_PHASE_INTRO_POP, POP_TIME);
            break;
        case MAP_PHASE_INTRO_POP:
            popOffset = 0.0f;
            setPhase(MAP_PHASE_INTRO_END_WAIT, INTRO_END_WAIT_TIME);
            break;
        case MAP_PHASE_INTRO_END_WAIT:
            phase = MAP_PHASE_DONE;
            switchScene(SCENE_GAME);
            break;
        case MAP_PHASE_ADVANCE_START_DELAY:
            setPhase(MAP_PHASE_ADVANCE_MOVE, ADVANCE_MOVE_TIME);
            break;
        case MAP_PHASE_ADVANCE_MOVE:
            classX = areaIconX[step];
            lineSolidWidth[step - 1] = LINE_WIDTH;
            areas[popIndex].solid = true;
            setPhase(MAP_PHASE_ADVANCE_POP, POP_TIME);
            break;
        case MAP_PHASE_ADVANCE_POP:
            popOffset = 0.0f;
            setPhase(MAP_PHASE_ADVANCE_END_WAIT, END_WAIT_TIME);
            break;
        case MAP_PHASE_ADVANCE_END_WAIT:
            phase = MAP_PHASE_DONE;
            switchScene(SCENE_GAME);
            break;
        case MAP_PHASE_DONE:
            break;
    }
}

static void updateAnimation(float deltaTime) {
    if (phase == MAP_PHASE_DONE) {
        return;
    }

    phaseTimer -= deltaTime;
    float progress = 1.0f;
    if (phaseDuration > 0.0f) {
        progress = clampFloat(1.0f - phaseTimer / phaseDuration, 0.0f, 1.0f);
    }

    switch (phase) {
        case MAP_PHASE_INTRO_REVEAL: {
            float elapsed = progress * phaseDuration;
            for (int i = 0; i < MAP_AREA_COUNT; i++) {
                areaReveal[i] = clampFloat((elapsed - areaRevealStart[i]) / REVEAL_DURATION, 0.0f, 1.0f);
            }
            for (int i = 0; i < MAP_LINE_COUNT; i++) {
                lineReveal[i] = clampFloat((elapsed - lineRevealStart[i]) / REVEAL_DURATION, 0.0f, 1.0f);
            }
            float classProgress = clampFloat((elapsed - CLASS_REVEAL_START) / CLASS_SLIDE_DURATION, 0.0f, 1.0f);
            classX = lerp(CLASS_SLIDE_START_X, (float)areaIconX[0], easeOutCubic(classProgress));
            break;
        }
        case MAP_PHASE_ADVANCE_MOVE: {
            float eased = easeInOutCubic(progress);
            classX = lerp((float)areaIconX[step - 1], (float)areaIconX[step], eased);
            lineSolidWidth[step - 1] = LINE_WIDTH * eased;
            break;
        }
        case MAP_PHASE_INTRO_POP:
        case MAP_PHASE_ADVANCE_POP:
            popOffset = -POP_HEIGHT * sinf(progress * PI);
            break;
        default:
            break;
    }

    if (phaseTimer <= 0.0f) {
        advancePhase();
    }
}

static void drawLines(void) {
    LCDPattern checkerLinePattern = {
        0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };

    for (int i = 0; i < MAP_LINE_COUNT; i++) {
        if (lineReveal[i] < 1.0f) {
            float eased = easeOutCubic(lineReveal[i]);
            int y = LINE_Y + (int)roundf(REVEAL_DRIFT * (1.0f - eased));
            drawLineOpacity(lineX[i], y, LINE_WIDTH, eased * DITHER_REVEAL_OPACITY);
            continue;
        }
        int ditherWidth = (int)lineDitherWidth[i];
        if (ditherWidth > 0) {
            pd->graphics->fillRect(lineX[i], LINE_Y, ditherWidth, LINE_HEIGHT, (LCDColor)checkerLinePattern);
        }
        int solidWidth = (int)lineSolidWidth[i];
        if (solidWidth > 0) {
            pd->graphics->fillRect(lineX[i], LINE_Y, solidWidth, LINE_HEIGHT, kColorWhite);
        }
    }
}

static void drawAreaIcons(void) {
    for (int i = 0; i < MAP_AREA_COUNT; i++) {
        if (!areas[i].visible) {
            continue;
        }
        LCDBitmap *icon = BITMAPS[areaIconBitmaps[i]];
        if (areaReveal[i] < 1.0f) {
            float eased = easeOutCubic(areaReveal[i]);
            int y = AREA_ICON_Y + (int)roundf(REVEAL_DRIFT * (1.0f - eased));
            drawIconOpacity(icon, areaIconX[i], y, eased * AREA_DITHER_OPACITY);
            continue;
        }
        int y = AREA_ICON_Y + ((i == popIndex) ? (int)roundf(popOffset) : 0);
        if (areas[i].solid) {
            pd->graphics->drawBitmap(icon, areaIconX[i], y, kBitmapUnflipped);
        } else {
            pd->graphics->pushContext(NULL);
                pd->graphics->setStencilImage(fadeStencils[opacityStencilIndex(AREA_DITHER_OPACITY)], 1);
                pd->graphics->drawBitmap(icon, areaIconX[i], y, kBitmapUnflipped);
            pd->graphics->popContext();
        }
    }
}

static void drawMap(void) {
    pd->graphics->clear(kColorBlack);

    pd->graphics->drawBitmap(BITMAPS[BITMAP_AREA_MAP_CORNER_TL], 0, 0, kBitmapUnflipped);
    pd->graphics->drawBitmap(BITMAPS[BITMAP_AREA_MAP_CORNER_TR], SCREEN_WIDTH - CORNER_SIZE, 0, kBitmapUnflipped);
    pd->graphics->drawBitmap(BITMAPS[BITMAP_AREA_MAP_CORNER_BL], 0, SCREEN_HEIGHT - CORNER_SIZE, kBitmapUnflipped);
    pd->graphics->drawBitmap(BITMAPS[BITMAP_AREA_MAP_CORNER_BR], SCREEN_WIDTH - CORNER_SIZE, SCREEN_HEIGHT - CORNER_SIZE, kBitmapUnflipped);

    drawLines();
    drawAreaIcons();

    if (classVisible) {
        LCDBitmap *icon = BITMAPS[classIconBitmaps[getCurrentClassType()]];
        int x = (int)roundf(classX);
        int y = CLASS_ICON_Y + (int)roundf(classYOffset);
        drawIconOpacity(icon, x, y, classOpacity);
    }
}

void updateAreaMapScene(float deltaTime) {
    updateAnimation(deltaTime);
    drawMap();
}
