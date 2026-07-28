#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/helpers.h"
#include "globals/localization.h"
#include "globals/sceneManager.h"
#include "title/classSelectScene.h"
#include "game/player/classes/classData.h"
#include "game/gameScene.h"
#include "game/rendering/textures.h"
#include "game/player/spells/spellData.h"
#include "game/systems/audioManager.h"
#include "game/systems/runeManager.h"
#include "game/systems/difficultyManager.h"

#define TITLE_Y 5
#define MODEL_CENTER_X 100
#define MODEL_Y_OFFSET 12

#define LEFT_ARROW_X 1
#define RIGHT_ARROW_X 187
#define ARROW_Y 51

// Info popup, at rest, occupies (7, 9) and is 186x111. All popup-relative
// layout below is expressed as offsets from that origin so the whole popup can
// slide horizontally (class switch) and vertically (open/close) as one unit. The
// background was widened 8px on each side and extended 16px at the top (to make
// room for the difficulty selector above the footer), so the content spreads out
// into the extra width and the footer's Y offset grew to stay put.
#define INFO_REST_X 7
#define INFO_REST_Y 9
#define INFO_BG_WIDTH 186
#define INFO_BG_HEIGHT 111

// Scrollable content, rendered per class into a tall bitmap. The bitmap is drawn
// flush with the popup's left edge and widened to fill the extra background room.
#define CONTENT_WIDTH 176
#define CONTENT_X_INSET 0
#define CONTENT_ICON_X 5
#define CONTENT_NAME_X 35
#define CONTENT_DESC_X 7
#define CONTENT_DESC_Y 29
#define CONTENT_BLOCK_SPACING 2
#define CONTENT_BOTTOM_PADDING 6  // extra scroll room past the last line
#define CONTENT_DESC_MAX_WIDTH 166
#define CONTENT_BLOCK_COUNT 3      // basic spell, special spell, starter rune

// The content bitmap is anchored so its first icon sits at absolute y 14 (offset
// 5 from the popup origin). Scroll math measures from there to the separator bar
// at y 85, so a view height of 71. The clip rect, however, extends up to the
// popup's inner border (offset 2) so scrolled content runs to the edge rather
// than cutting off short of it.
#define CONTENT_VIEW_TOP_OFFSET 5  // 14 - 9
#define CONTENT_VIEW_HEIGHT 71     // 85 - 14
#define CONTENT_CLIP_TOP_OFFSET 2  // inner border, 11 - 9
#define CONTENT_CLIP_HEIGHT 74     // 85 - 11

// Scrollbar track, popup-relative, styled to match the settings-menu scrollbar
// (5px rounded-rect outline with a 1px inner thumb line). Top-left at (183, 15).
#define SCROLLBAR_X_OFFSET 176     // 183 - 7
#define SCROLLBAR_Y_OFFSET 6       // 15 - 9
#define SCROLLBAR_WIDTH 5
#define SCROLLBAR_HEIGHT 66

// Footer prompts, popup-relative: "Back" left-anchored at x 27 next to the B
// button and "Select" right-anchored at x 172 next to the A button, both
// vertically centered on the button icons whose center sits at y 111. The footer
// keeps its absolute position after the popup grew upward, so its Y offset grew.
#define FOOTER_CENTER_Y_OFFSET 102     // 111 - 9
#define FOOTER_BACK_X_OFFSET 20        // 27 - 7
#define FOOTER_SELECT_RIGHT_OFFSET 165 // 172 - 7

// Difficulty selector, popup-relative. The description text is centered at
// absolute (100, 93); the difficulty number is centered at x 100 on the footer
// text row; the arrows sit at absolute (82, 108) and (110, 108).
#define DIFFICULTY_CENTER_X_OFFSET 93   // 100 - 7
#define DIFFICULTY_TEXT_Y_OFFSET 84     // 93 - 9
#define DIFFICULTY_LEFT_ARROW_X_OFFSET 75  // 82 - 7
#define DIFFICULTY_RIGHT_ARROW_X_OFFSET 103 // 110 - 7
#define DIFFICULTY_ARROW_Y_OFFSET 99    // 108 - 9

#define SCROLL_SPEED 160.0f
#define CRANK_SCROLL_FACTOR 0.5f

// Pre-rendered model animation plays at its authored rate, independent of the
// selectable display refresh rate.
#define CLASS_3D_MODEL_FRAME_TIME 1.0f / DEFAULT_TARGET_FPS
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
    [CLASS_KNIGHT] = {
        .bitmapTablePath = BITMAP_TABLE_CLASS_SELECT_KNIGHT,
        .introEndFrame = 76
    },
    [CLASS_ROGUE] = {
        .bitmapTablePath = BITMAP_TABLE_CLASS_SELECT_ROGUE,
        .introEndFrame = 109
    },
    [CLASS_RANGER] = {
        .bitmapTablePath = BITMAP_TABLE_CLASS_SELECT_RANGER,
        .introEndFrame = 121
    },
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

typedef struct ClassInfoContent {
    LCDBitmap *bitmap;
    int contentHeight;
} ClassInfoContent;

static Class3DModelData class3DModels[CLASS_COUNT];
static ClassInfoContent classInfo[CLASS_COUNT];

static int selectedIndex;
static float displaySelected;
static float slideStartValue;
static float slideTime;

// Currently-selected difficulty per class (0..DIFFICULTY_LEVEL_COUNT-1). The
// player can browse any level, but starting a run at a level above the class's
// unlocked difficulty is blocked and the description reads "Locked".
static int selectedDifficulty[CLASS_COUNT];

// Info popup open/close animation. infoDisplay eases between 0 (fully off the
// bottom of the screen) and 1 (at rest). infoOpen is the target the slide eases
// toward, so toggling it mid-animation cleanly reverses direction.
static bool infoOpen;
static float infoDisplay;
static float infoSlideStart;
static float infoSlideTime;
static float scrollOffset;

static void buildClassInfoContent(void) {
    Language lang = SETTINGS.language;
    LCDFont *nameFont = (lang == LANGUAGE_JP) ? fontTitleJP : fontTitleEN;
    LCDFont *descFont = (lang == LANGUAGE_JP) ? fontSubJP : fontSubEN;
    int nameTracking = (lang == LANGUAGE_JP) ? -6 : -1;
    int descTracking = (lang == LANGUAGE_JP) ? 1 : -2;
    int descLeading = (lang == LANGUAGE_JP) ? 2 : 0;
    int nameFontHeight = pd->graphics->getFontHeight(nameFont);

    for (int i = 0; i < CLASS_COUNT; i++) {
        if (classInfo[i].bitmap != NULL) {
            pd->graphics->freeBitmap(classInfo[i].bitmap);
            classInfo[i].bitmap = NULL;
        }

        ClassData *cd = &classData[i];
        Spell basic = cd->basicSpell;
        Spell special = cd->specialSpell;
        RuneType rune = (RuneType)cd->starterRune;

        LCDBitmap *blockIcons[CONTENT_BLOCK_COUNT] = {
            getSpellIcon(basic),
            getSpellIcon(special),
            BITMAPS[getRuneIconIndex(rune)]
        };
        const char *blockNames[CONTENT_BLOCK_COUNT] = {
            (lang == LANGUAGE_JP) ? baseSpellData[basic].name_jp : baseSpellData[basic].name,
            (lang == LANGUAGE_JP) ? baseSpellData[special].name_jp : baseSpellData[special].name,
            getRuneName(rune, lang)
        };
        const char *blockDescs[CONTENT_BLOCK_COUNT] = {
            (lang == LANGUAGE_JP) ? baseSpellData[basic].description_jp : baseSpellData[basic].description,
            (lang == LANGUAGE_JP) ? baseSpellData[special].description_jp : baseSpellData[special].description,
            getRuneDescription(rune, lang, 1)
        };

        // Tag the basic and special spell descriptions with their role. Owned
        // strings are freed once the content bitmap is rendered.
        const char *blockTags[CONTENT_BLOCK_COUNT] = {
            getLocString(STR_CLASS_SELECT_BASIC_TAG),
            getLocString(STR_CLASS_SELECT_SPECIAL_TAG),
            NULL
        };
        char *ownedDescs[CONTENT_BLOCK_COUNT] = { NULL, NULL, NULL };
        for (int b = 0; b < CONTENT_BLOCK_COUNT; b++) {
            if (blockTags[b] != NULL) {
                pd->system->formatString(&ownedDescs[b], "%s %s", blockTags[b], blockDescs[b]);
                if (ownedDescs[b] != NULL) {
                    blockDescs[b] = ownedDescs[b];
                }
            }
        }

        int blockTops[CONTENT_BLOCK_COUNT];
        int descHeights[CONTENT_BLOCK_COUNT];
        int y = 0;
        for (int b = 0; b < CONTENT_BLOCK_COUNT; b++) {
            blockTops[b] = y;
            descHeights[b] = pd->graphics->getTextHeightForMaxWidth(descFont, blockDescs[b], strlen(blockDescs[b]), CONTENT_DESC_MAX_WIDTH, kUTF8Encoding, kWrapWord, descTracking, descLeading);
            y += CONTENT_DESC_Y + descHeights[b] + CONTENT_BLOCK_SPACING;
        }
        int contentHeight = y - CONTENT_BLOCK_SPACING + CONTENT_BOTTOM_PADDING;
        classInfo[i].contentHeight = contentHeight;

        LCDBitmap *bitmap = pd->graphics->newBitmap(CONTENT_WIDTH, contentHeight, kColorClear);
        if (bitmap == NULL) {
            pd->system->logToConsole("classSelect: content bitmap alloc failed for class %d", i);
            classInfo[i].bitmap = NULL;
            continue;
        }

        pd->graphics->pushContext(bitmap);
        for (int b = 0; b < CONTENT_BLOCK_COUNT; b++) {
            int top = blockTops[b];
            int iconHeight = 0;
            pd->graphics->getBitmapData(blockIcons[b], NULL, &iconHeight, NULL, NULL, NULL);
            // The rune icon (last block) carries an extra outline of padding; the
            // spell icons don't, so nudge them down and right by 2 to line their
            // art up with the rune's.
            int iconInset = (b == CONTENT_BLOCK_COUNT - 1) ? 0 : 2;
            int iconX = CONTENT_ICON_X + iconInset;
            int iconY = top + iconInset;
            pd->graphics->setDrawMode(kDrawModeCopy);
            pd->graphics->drawBitmap(blockIcons[b], iconX, iconY, kBitmapUnflipped);

            pd->graphics->setFont(nameFont);
            pd->graphics->setTextTracking(nameTracking);
            pd->graphics->setTextLeading(0);
            int nameY = iconY + (iconHeight - nameFontHeight) / 2 + ((lang == LANGUAGE_JP) ? 0 : 1);
            int nameX = CONTENT_NAME_X + ((lang == LANGUAGE_JP) ? - 3 : 0);
            pd->graphics->drawText(blockNames[b], strlen(blockNames[b]), kUTF8Encoding, nameX, nameY);

            pd->graphics->setFont(descFont);
            pd->graphics->setTextTracking(descTracking);
            pd->graphics->setTextLeading(descLeading);
            pd->graphics->drawTextInRect(blockDescs[b], strlen(blockDescs[b]), kUTF8Encoding, CONTENT_DESC_X, top + CONTENT_DESC_Y, CONTENT_DESC_MAX_WIDTH, descHeights[b], kWrapWord, kAlignTextLeft);
        }
        pd->graphics->setDrawMode(kDrawModeCopy);
        pd->graphics->setTextLeading(0);
        pd->graphics->popContext();

        for (int b = 0; b < CONTENT_BLOCK_COUNT; b++) {
            if (ownedDescs[b] != NULL) {
                pd->system->realloc(ownedDescs[b], 0);
            }
        }

        classInfo[i].bitmap = bitmap;
    }
}

void initClassSelectScene(void) {
    selectedIndex = 0;
    displaySelected = 0.0f;
    slideStartValue = 0.0f;
    slideTime = 0.0f;

    infoOpen = false;
    infoDisplay = 0.0f;
    infoSlideStart = 0.0f;
    infoSlideTime = 0.0f;
    scrollOffset = 0.0f;

    for (int i = 0; i < CLASS_COUNT; i++) {
        selectedDifficulty[i] = 0;
    }

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

    buildClassInfoContent();
}

static void drawInfoScrollbar(int popupX, int popupY, int contentHeight, float scroll) {
    if (contentHeight - CONTENT_VIEW_HEIGHT <= 0) {
        return;
    }

    int scrollbarX = popupX + SCROLLBAR_X_OFFSET;
    int scrollbarY = popupY + SCROLLBAR_Y_OFFSET;
    int thumbH = SCROLLBAR_HEIGHT * CONTENT_VIEW_HEIGHT / contentHeight;
    int thumbY = scrollbarY + (int)(SCROLLBAR_HEIGHT * scroll / (float)contentHeight + 0.5f);
    pd->graphics->drawRoundRect(scrollbarX, scrollbarY, SCROLLBAR_WIDTH, SCROLLBAR_HEIGHT, 2, 1, kColorWhite);
    pd->graphics->fillRect(scrollbarX + 2, thumbY + 3, 1, thumbH - 6, kColorWhite);
}

static void drawInfoFooter(int popupX, int popupY) {
    LCDFont *font = (SETTINGS.language == LANGUAGE_JP) ? fontSubJP : fontSubEN;
    const char *back = getLocString(STR_CLASS_SELECT_BACK);
    const char *select = getLocString(STR_CLASS_SELECT_SELECT);
    int footerY = popupY + FOOTER_CENTER_Y_OFFSET - pd->graphics->getFontHeight(font) / 2 + 1;

    pd->graphics->setFont(font);
    pd->graphics->setDrawMode(kDrawModeCopy);
    int textTracking = (SETTINGS.language == LANGUAGE_JP) ? -2 : -2;
    pd->graphics->setTextTracking(textTracking);
    pd->graphics->drawText(back, strlen(back), kUTF8Encoding, popupX + FOOTER_BACK_X_OFFSET, footerY);
    int selectWidth = pd->graphics->getTextWidth(font, select, strlen(select), kUTF8Encoding, textTracking);
    pd->graphics->drawText(select, strlen(select), kUTF8Encoding, popupX + FOOTER_SELECT_RIGHT_OFFSET - selectWidth, footerY);
}

static void drawInfoDifficulty(int popupX, int popupY, int classIndex) {
    bool isJapanese = (SETTINGS.language == LANGUAGE_JP);
    LCDFont *font = isJapanese ? fontSubJP : fontSubEN;
    int difficulty = selectedDifficulty[classIndex];
    bool locked = difficulty > getClassUnlockedDifficulty((ClassType)classIndex);
    int centerX = popupX + DIFFICULTY_CENTER_X_OFFSET;
    // JP glyphs sit lower and read cramped at negative tracking, so nudge them up
    // 2px and drop the tracking to 0.
    int textTracking = isJapanese ? 0 : -2;
    int jpYNudge = isJapanese ? -2 : 0;

    pd->graphics->setFont(font);
    pd->graphics->setDrawMode(kDrawModeCopy);
    pd->graphics->setTextTracking(textTracking);

    // Difficulty description (or "Locked") centered in the space above the footer.
    const char *desc = locked ? getLocString(STR_DIFFICULTY_LOCKED) : getLocString(STR_DIFFICULTY_0 + difficulty);
    int descWidth = pd->graphics->getTextWidth(font, desc, strlen(desc), kUTF8Encoding, textTracking);
    pd->graphics->drawText(desc, strlen(desc), kUTF8Encoding, centerX - descWidth / 2, popupY + DIFFICULTY_TEXT_Y_OFFSET + jpYNudge);

    // Difficulty number, centered on the footer text row between the arrows.
    int footerY = popupY + FOOTER_CENTER_Y_OFFSET - pd->graphics->getFontHeight(font) / 2 + 1;
    char numStr[2] = { (char)('0' + difficulty), '\0' };
    int numWidth = pd->graphics->getTextWidth(font, numStr, 1, kUTF8Encoding, textTracking);
    pd->graphics->drawText(numStr, 1, kUTF8Encoding, centerX - numWidth / 2, footerY + jpYNudge);

    // Arrows flanking the number; hidden at the ends so the value doesn't wrap.
    if (difficulty > 0) {
        pd->graphics->drawBitmap(BITMAPS[BITMAP_DIFFICULTY_LEFT_ARROW], popupX + DIFFICULTY_LEFT_ARROW_X_OFFSET, popupY + DIFFICULTY_ARROW_Y_OFFSET, kBitmapUnflipped);
    }
    if (difficulty < DIFFICULTY_LEVEL_COUNT - 1) {
        pd->graphics->drawBitmap(BITMAPS[BITMAP_DIFFICULTY_RIGHT_ARROW], popupX + DIFFICULTY_RIGHT_ARROW_X_OFFSET, popupY + DIFFICULTY_ARROW_Y_OFFSET, kBitmapUnflipped);
    }
}

static void drawInfoPopups(int popupY) {
    for (int i = 0; i < CLASS_COUNT; i++) {
        float popupXf = (float)INFO_REST_X + ((float)i - displaySelected) * (float)SCREEN_WIDTH;
        if (popupXf <= -(float)INFO_BG_WIDTH || popupXf >= (float)SCREEN_WIDTH) {
            continue;
        }
        int popupX = (int)popupXf;

        pd->graphics->drawBitmap(BITMAPS[BITMAP_CLASS_SELECT_INFO_BACKGROUND], popupX, popupY, kBitmapUnflipped);

        float scroll = (i == selectedIndex) ? scrollOffset : 0.0f;
        int viewTop = popupY + CONTENT_VIEW_TOP_OFFSET;
        if (classInfo[i].bitmap != NULL) {
            pd->graphics->setClipRect(popupX + CONTENT_X_INSET, popupY + CONTENT_CLIP_TOP_OFFSET, CONTENT_WIDTH, CONTENT_CLIP_HEIGHT);
            pd->graphics->drawBitmap(classInfo[i].bitmap, popupX + CONTENT_X_INSET, viewTop - (int)scroll, kBitmapUnflipped);
            pd->graphics->clearClipRect();
        }

        drawInfoScrollbar(popupX, popupY, classInfo[i].contentHeight, scroll);
        drawInfoDifficulty(popupX, popupY, i);
        drawInfoFooter(popupX, popupY);
    }
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

    if (infoSlideTime > 0.0f) {
        infoSlideTime -= deltaTime;
        float target = infoOpen ? 1.0f : 0.0f;
        if (infoSlideTime <= 0.0f) {
            infoSlideTime = 0.0f;
            infoDisplay = target;
        } else {
            float progress = 1.0f - (infoSlideTime / SLIDE_DURATION);
            float eased = easeOutCubic(progress);
            infoDisplay = infoSlideStart + (target - infoSlideStart) * eased;
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
        float groupX = (float)MODEL_CENTER_X + ((float)i - displaySelected) * (float)SCREEN_WIDTH;
        int modelDrawX = (int)(groupX - m->width / 2.0f);
        int modelDrawY = (SCREEN_HEIGHT - m->height) / 2 + MODEL_Y_OFFSET;
        LCDBitmap *frame = pd->graphics->getTableBitmap(m->bitmapTable, m->currentFrame);
        pd->graphics->drawBitmap(frame, modelDrawX, modelDrawY, kBitmapUnflipped);
    }

    bool isJapanese = (SETTINGS.language == LANGUAGE_JP);
    pd->graphics->setFont(isJapanese ? fontTitleJP : fontTitleEN);
    pd->graphics->setDrawMode(kDrawModeInverted);
    pd->graphics->setTextTracking(0);
    ClassData *cd = &classData[selectedIndex];
    const char *title = isJapanese ? cd->nameJP : cd->name;
    int titleWidth = pd->graphics->getTextWidth(isJapanese ? fontTitleJP : fontTitleEN, title, strlen(title), kUTF8Encoding, 0);
    pd->graphics->drawText(title, strlen(title), kUTF8Encoding, (SCREEN_WIDTH - titleWidth) / 2, TITLE_Y);
    pd->graphics->setDrawMode(kDrawModeCopy);

    if (selectedIndex > 0) {
        pd->graphics->drawBitmap(BITMAPS[BITMAP_CLASS_SELECT_LEFT_ARROW], LEFT_ARROW_X, ARROW_Y, kBitmapUnflipped);
    }
    if (selectedIndex < CLASS_COUNT - 1) {
        pd->graphics->drawBitmap(BITMAPS[BITMAP_CLASS_SELECT_RIGHT_ARROW], RIGHT_ARROW_X, ARROW_Y, kBitmapUnflipped);
    }

    if (infoDisplay > 0.0f) {
        int popupY = (int)lerp((float)SCREEN_HEIGHT, (float)INFO_REST_Y, infoDisplay);
        drawInfoPopups(popupY);
    }

    PDButtons current, pushed;
    pd->system->getButtonState(&current, &pushed, NULL);
    float crankChange = pd->system->getCrankChange();

    if (!isSceneTransitioning()) {
        if (infoOpen) {
            // Left/right adjust the difficulty; they no longer switch class here.
            int *difficulty = &selectedDifficulty[selectedIndex];
            if (pushed & kButtonLeft) {
                if (*difficulty > 0) {
                    playSound(SFX_UI_RUNE_SLIDE);
                    (*difficulty)--;
                } else {
                    playSound(SFX_CLASS_ERROR);
                }
            } else if (pushed & kButtonRight) {
                if (*difficulty < DIFFICULTY_LEVEL_COUNT - 1) {
                    playSound(SFX_UI_RUNE_SLIDE);
                    (*difficulty)++;
                } else {
                    playSound(SFX_CLASS_ERROR);
                }
            }

            int scrollMax = classInfo[selectedIndex].contentHeight - CONTENT_VIEW_HEIGHT;
            if (scrollMax < 0) {
                scrollMax = 0;
            }
            if (current & kButtonUp) {
                scrollOffset -= SCROLL_SPEED * deltaTime;
            }
            if (current & kButtonDown) {
                scrollOffset += SCROLL_SPEED * deltaTime;
            }
            scrollOffset += crankChange * CRANK_SCROLL_FACTOR;
            if (scrollOffset < 0.0f) {
                scrollOffset = 0.0f;
            }
            if (scrollOffset > (float)scrollMax) {
                scrollOffset = (float)scrollMax;
            }

            if (pushed & kButtonA) {
                if (*difficulty > getClassUnlockedDifficulty((ClassType)selectedIndex)) {
                    // Difficulty not yet unlocked for this class.
                    playSound(SFX_CLASS_ERROR);
                } else {
                    playSound(SFX_CLASS_SELECT);
                    setCurrentClass((ClassType)selectedIndex);
                    setRunDifficulty(*difficulty);
                    initRunStart();
                    switchScene(SCENE_AREA_MAP);
                }
            } else if (pushed & kButtonB) {
                playSound(SFX_CLASS_BACK);
                infoOpen = false;
                infoSlideStart = infoDisplay;
                infoSlideTime = SLIDE_DURATION;
            }
        } else {
            int newIndex = selectedIndex;
            if (pushed & kButtonLeft) {
                if (selectedIndex > 0) {
                    playSound(SFX_UI_RUNE_SLIDE);
                    newIndex = selectedIndex - 1;
                } else {
                    playSound(SFX_CLASS_ERROR);
                }
            } else if (pushed & kButtonRight) {
                if (selectedIndex < CLASS_COUNT - 1) {
                    playSound(SFX_UI_RUNE_SLIDE);
                    newIndex = selectedIndex + 1;
                } else {
                    playSound(SFX_CLASS_ERROR);
                }
            }
            if (newIndex != selectedIndex) {
                slideStartValue = displaySelected;
                selectedIndex = newIndex;
                slideTime = SLIDE_DURATION;
                scrollOffset = 0.0f;
                Class3DModelData *m = &class3DModels[newIndex];
                m->phase = CLASS_ANIM_INTRO;
                m->currentFrame = 0;
                m->currentFrameTime = 0.0f;
            }

            if (pushed & kButtonA) {
                playSound(SFX_UI_RUNE_SLIDE);
                infoOpen = true;
                infoSlideStart = infoDisplay;
                infoSlideTime = SLIDE_DURATION;
            } else if (pushed & kButtonB) {
                playSound(SFX_CLASS_BACK);
                switchScene(SCENE_TITLE);
            }
        }
    }
}
