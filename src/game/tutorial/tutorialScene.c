#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/helpers.h"
#include "globals/localization.h"
#include "globals/sceneManager.h"
#include "globals/saveSystem.h"

#include "game/gameScene.h"
#include "game/map/map.h"
#include "game/UI/UI.h"
#include "game/UI/quitConfirmDialog.h"
#include "game/UI/runeInventory.h"
#include "game/UI/gameMenu.h"
#include "game/player/player.h"
#include "game/enemies/enemies.h"
#include "game/entities/interactables.h"
#include "game/entities/pickups.h"
#include "game/systems/runStats.h"
#include "game/systems/shopManager.h"
#include "game/systems/runeManager.h"
#include "game/systems/spawnManager.h"
#include "game/systems/progressionManager.h"
#include "game/settings/settings.h"

#include "game/tutorial/tutorialScene.h"
#include "game/tutorial/tutorialDialog.h"

typedef enum {
    TUTORIAL_R1_DIALOG,
    TUTORIAL_R1_WAITING,
    TUTORIAL_R1_TO_R2,
    TUTORIAL_R2_ENTER,
    TUTORIAL_R2_DIALOG,
    TUTORIAL_R2_WAITING,
    TUTORIAL_R2_TO_R3,
    TUTORIAL_R3_ENTER,
    TUTORIAL_R3_DIALOG,
    TUTORIAL_R3_WAITING,
    TUTORIAL_R3_TO_R4,
    TUTORIAL_R4_ENTER,
    TUTORIAL_R4_DIALOG,
    TUTORIAL_R4_WAITING,
    TUTORIAL_R4_TO_R5,
    TUTORIAL_R5_ENTER,
    TUTORIAL_R5_DIALOG_PRE_SHOP,
    TUTORIAL_R5_SHOP_OPEN,
    TUTORIAL_R5_SHOP_WAIT,
    TUTORIAL_R5_DIALOG_POST_SHOP,
    TUTORIAL_R5_EXIT_SPAWNED,
    TUTORIAL_DONE
} TutorialPhase;

static TutorialPhase tPhase;

// Seconds a task can go uncompleted (measured from the end of the last dialog)
// before a reminder dialog reappears, then repeats.
#define TUTORIAL_REMINDER_TIME 20.0f

static bool entranceClosed;
static int r1ArrowStep;
static int r1ArrowPickupIndex;
static int r2Killed;
static int r4DashBaseline;
static int r4DashSfxPlayed;
static bool prevDialogActive;
static float reminderTimer;

static PDMenuItem *skipMenuItem;

static bool tutorialActive;

bool isTutorialActive(void) {
    return tutorialActive;
}

void setTutorialActive(bool active) {
    tutorialActive = active;
}

void removeSkipTutorialMenuItem(void) {
    if (skipMenuItem != NULL) {
        pd->system->removeMenuItem(skipMenuItem);
        skipMenuItem = NULL;
    }
}

void refreshSkipTutorialMenuItemTitle(void) {
    if (skipMenuItem != NULL) {
        pd->system->setMenuItemTitle(skipMenuItem, getLocString(STR_MENU_SKIP_TUTORIAL));
    }
}

static void onSkipTutorialSelect(void *userdata) {
    removeSkipTutorialMenuItem();
    setPlayerInputDisabled(false);
    tPhase = TUTORIAL_DONE;
    setTutorialCompleted(true);
    initRunStart();
    switchScene(SCENE_AREA_MAP);
}

static const StringKey r1Pages[] = {
    STR_TUTORIAL_R1_1,
    STR_TUTORIAL_R1_2,
    STR_TUTORIAL_R1_3,
    STR_TUTORIAL_R1_4
};
static const StringKey r2Pages[] = {
    STR_TUTORIAL_R2_1,
    STR_TUTORIAL_R2_2,
    STR_TUTORIAL_R2_3
};
static const StringKey r3Pages[] = {
    STR_TUTORIAL_R3_1,
    STR_TUTORIAL_R3_2,
    STR_TUTORIAL_R3_3
};
static const StringKey r4Pages[] = {
    STR_TUTORIAL_R4_1,
    STR_TUTORIAL_R4_2,
    STR_TUTORIAL_R4_3,
    STR_TUTORIAL_R4_4
};
static const StringKey r5PreShopPages[] = {
    STR_TUTORIAL_R5_1,
    STR_TUTORIAL_R5_2
};
static const StringKey r5PostShopPages[] = {
    STR_TUTORIAL_R6_1,
    STR_TUTORIAL_R6_2
};
static const StringKey nextRoomPages[] = {
    STR_TUTORIAL_NEXT_ROOM
};

// Reminder pages restate the controls and objective for each task that can stall.
static const StringKey r1ReminderPages[] = {
    STR_TUTORIAL_R1_2,
    STR_TUTORIAL_R1_4
};
static const StringKey r2ReminderPages[] = {
    STR_TUTORIAL_R2_1,
    STR_TUTORIAL_R2_3
};
static const StringKey r3ReminderPages[] = {
    STR_TUTORIAL_R3_1,
    STR_TUTORIAL_R3_3
};
static const StringKey r4ReminderPages[] = {
    STR_TUTORIAL_R4_1,
    STR_TUTORIAL_R4_4
};
static const StringKey exitReminderPages[] = {
    STR_TUTORIAL_R6_2
};

static const StringKey *getReminderPages(int *outCount) {
    switch (tPhase) {
        case TUTORIAL_R1_WAITING:
            *outCount = (int)(sizeof(r1ReminderPages) / sizeof(r1ReminderPages[0]));
            return r1ReminderPages;
        case TUTORIAL_R2_WAITING:
            *outCount = (int)(sizeof(r2ReminderPages) / sizeof(r2ReminderPages[0]));
            return r2ReminderPages;
        case TUTORIAL_R3_WAITING:
            *outCount = (int)(sizeof(r3ReminderPages) / sizeof(r3ReminderPages[0]));
            return r3ReminderPages;
        case TUTORIAL_R4_WAITING:
            *outCount = (int)(sizeof(r4ReminderPages) / sizeof(r4ReminderPages[0]));
            return r4ReminderPages;
        case TUTORIAL_R1_TO_R2:
        case TUTORIAL_R2_TO_R3:
        case TUTORIAL_R3_TO_R4:
        case TUTORIAL_R4_TO_R5:
            *outCount = (int)(sizeof(nextRoomPages) / sizeof(nextRoomPages[0]));
            return nextRoomPages;
        case TUTORIAL_R5_EXIT_SPAWNED:
            *outCount = (int)(sizeof(exitReminderPages) / sizeof(exitReminderPages[0]));
            return exitReminderPages;
        default:
            return NULL;
    }
}

// Reopens the current task's reminder dialog once TUTORIAL_REMINDER_TIME of
// dialog-free waiting elapses; the timer is held at zero while any dialog shows,
// so the countdown always restarts from the end of the previous dialog.
static void updateTutorialReminder(float deltaTime) {
    if (isTutorialDialogActive()) {
        reminderTimer = 0.0f;
        return;
    }
    int pageCount = 0;
    const StringKey *pages = getReminderPages(&pageCount);
    if (pages == NULL) {
        reminderTimer = 0.0f;
        return;
    }
    reminderTimer += deltaTime;
    if (reminderTimer >= TUTORIAL_REMINDER_TIME) {
        reminderTimer = 0.0f;
        openTutorialDialogPages(pages, pageCount);
    }
}

static void setArrowToRoom(int roomIndex) {
    Point p = getRoomWorldCenter(roomIndex);
    setArrowPosition(p.x, p.y);
}

static void spawnDummyAtCell(int cellX, int cellY) {
    int worldX = cellX * CELL_SIZE + CELL_SIZE / 2;
    int worldY = cellY * CELL_SIZE + CELL_SIZE / 2;
    spawnEnemy(ENEMY_TYPE_DUMMY, worldX, worldY, false);
    setArrowPosition(worldX, worldY);
}

static int dummiesAlive(void) {
    return getActiveEnemyCount();
}

// Gate the combat abilities and reveal their HUD together so the tutorial can
// unlock movement, basic spell, special spell, and dash one room at a time.
static void setTutorialAbilities(bool basic, bool special, bool dash) {
    setPlayerBasicSpellEnabled(basic);
    setPlayerSpecialSpellEnabled(special);
    setPlayerDashEnabled(dash);
    setSpellUIVisibility(basic, special, dash);
}

void initTutorialScene(void) {
    setCurrentArea(AREA_TUTORIAL);
    setTutorialActive(true);
    initGameScene();

    createGameMenuItem();

    removeSkipTutorialMenuItem();
    skipMenuItem = pd->system->addMenuItem(getLocString(STR_MENU_SKIP_TUTORIAL), onSkipTutorialSelect, NULL);

    tPhase = TUTORIAL_R1_DIALOG;
    r1ArrowStep = 0;
    r1ArrowPickupIndex = -1;
    r2Killed = 0;
    r4DashBaseline = 0;
    entranceClosed = false;
    prevDialogActive = false;
    reminderTimer = 0.0f;

    // Room 1 is movement only: no basic spell, special spell, or dash, and their
    // HUD stays hidden until each is unlocked in a later room.
    setTutorialAbilities(false, false, false);

    openTutorialDialog(r1Pages);
}

static void closeBehindIfEntered(int roomIndex, int gateIndex) {
    if (!entranceClosed && roomIndex == 1) {
        closeEntranceGate();
        entranceClosed = true;
    }
    closeRoomGate(gateIndex);
}

static void progressToNextRoom(TutorialPhase nextTPhase, int currentRoomIndex) {
    tPhase = nextTPhase;
    openRoomGate(currentRoomIndex);
    setArrowToRoom(currentRoomIndex + 1);
    playSound(SFX_TUTORIAL_PASS);
    openTutorialDialog(nextRoomPages);
}

static void updateTutorialPhase(float deltaTime) {
    updateTutorialDialog(deltaTime);

    switch (tPhase) {
        case TUTORIAL_R1_DIALOG: {
            if (isTutorialDialogComplete()) {
                tPhase = TUTORIAL_R1_WAITING;
            }
            break;
        }
        case TUTORIAL_R1_WAITING: {
            if (!entranceClosed && getPlayerCurrentRoom() == 0) {
                closeEntranceGate();
                entranceClosed = true;
            }
            if (r1ArrowPickupIndex < 0) {
                Point minP = getRoomInteriorMin(0);
                Point maxP = getRoomInteriorMax(0);
                Point center = getRoomCenter(0);
                int cellX, cellY;
                switch (r1ArrowStep) {
                    case 0: cellX = (int)maxP.x - 2; cellY = (int)minP.y + 2; break;
                    case 1: cellX = (int)maxP.x - 2; cellY = (int)maxP.y - 2; break;
                    default: cellX = (int)center.x; cellY = (int)center.y; break;
                }
                int worldX = cellX * CELL_SIZE + CELL_SIZE / 2;
                int worldY = cellY * CELL_SIZE + CELL_SIZE / 2;
                r1ArrowPickupIndex = spawnPickup(worldX, worldY, PICKUP_TYPE_ARROW);
                setArrowPosition(worldX, worldY);
            } else if (!pickups[r1ArrowPickupIndex].active) {
                r1ArrowPickupIndex = -1;
                r1ArrowStep++;
                if (r1ArrowStep >= 3) {
                    progressToNextRoom(TUTORIAL_R1_TO_R2, 0);
                }
            }
            break;
        }
        case TUTORIAL_R1_TO_R2: {
            if (getPlayerCurrentRoom() == 1) {
                tPhase = TUTORIAL_R2_ENTER;
            }
            break;
        }
        case TUTORIAL_R2_ENTER: {
            closeBehindIfEntered(1, 0);
            setCurrentRoomIndex(1);
            setTutorialAbilities(true, false, false);
            r2Killed = 0;
            Point center = getRoomWorldCenter(1);
            spawnEnemy(ENEMY_TYPE_DUMMY, (int)center.x, (int)center.y, false);
            openTutorialDialog(r2Pages);
            tPhase = TUTORIAL_R2_DIALOG;
            break;
        }
        case TUTORIAL_R2_DIALOG: {
            if (isTutorialDialogComplete()) {
                tPhase = TUTORIAL_R2_WAITING;
            }
            break;
        }
        case TUTORIAL_R2_WAITING: {
            if (dummiesAlive() == 0) {
                r2Killed++;
                Point minP = getRoomInteriorMin(1);
                Point maxP = getRoomInteriorMax(1);
                playSound(SFX_TUTORIAL_ARROW_PICKUP);
                if (r2Killed == 1) {
                    spawnDummyAtCell((int)maxP.x - 2, (int)minP.y + 2);
                } else if (r2Killed == 2) {
                    spawnDummyAtCell((int)maxP.x - 2, (int)maxP.y - 2);
                } else {
                    progressToNextRoom(TUTORIAL_R2_TO_R3, 1);
                }
            }
            break;
        }
        case TUTORIAL_R2_TO_R3: {
            if (getPlayerCurrentRoom() == 2) {
                tPhase = TUTORIAL_R3_ENTER;
            }
            break;
        }
        case TUTORIAL_R3_ENTER: {
            closeRoomGate(1);
            setCurrentRoomIndex(2);
            setTutorialAbilities(false, true, false);
            for (int i = 0; i < 3; i++) {
                Point p = getRoomSpawnPosition(2, i);
                spawnEnemy(ENEMY_TYPE_DUMMY, (int)p.x, (int)p.y, false);
            }
            openTutorialDialog(r3Pages);
            tPhase = TUTORIAL_R3_DIALOG;
            break;
        }
        case TUTORIAL_R3_DIALOG: {
            if (isTutorialDialogComplete()) {
                tPhase = TUTORIAL_R3_WAITING;
            }
            break;
        }
        case TUTORIAL_R3_WAITING: {
            int alive = dummiesAlive();
            if (alive == 0) {
                progressToNextRoom(TUTORIAL_R3_TO_R4, 2);
            }
            break;
        }
        case TUTORIAL_R3_TO_R4: {
            if (getPlayerCurrentRoom() == 3) {
                tPhase = TUTORIAL_R4_ENTER;
            }
            break;
        }
        case TUTORIAL_R4_ENTER: {
            closeRoomGate(2);
            setCurrentRoomIndex(3);
            // From room 4 on, every input stays enabled.
            setTutorialAbilities(true, true, true);
            clearArrow();
            r4DashBaseline = playerDashCount;
            r4DashSfxPlayed = 0;
            openTutorialDialog(r4Pages);
            tPhase = TUTORIAL_R4_DIALOG;
            break;
        }
        case TUTORIAL_R4_DIALOG: {
            if (isTutorialDialogComplete()) {
                tPhase = TUTORIAL_R4_WAITING;
            }
            break;
        }
        case TUTORIAL_R4_WAITING: {
            int dashes = playerDashCount - r4DashBaseline;
            while (r4DashSfxPlayed < dashes && r4DashSfxPlayed < 3) {
                playSound(SFX_TUTORIAL_ARROW_PICKUP);
                r4DashSfxPlayed++;
            }
            if (dashes >= 3) {
                progressToNextRoom(TUTORIAL_R4_TO_R5, 3);
            }
            break;
        }
        case TUTORIAL_R4_TO_R5: {
            if (getPlayerCurrentRoom() == 4) {
                tPhase = TUTORIAL_R5_ENTER;
            }
            break;
        }
        case TUTORIAL_R5_ENTER: {
            closeRoomGate(3);
            setCurrentRoomIndex(4);
            clearArrow();
            openTutorialDialog(r5PreShopPages);
            tPhase = TUTORIAL_R5_DIALOG_PRE_SHOP;
            break;
        }
        case TUTORIAL_R5_DIALOG_PRE_SHOP: {
            if (isTutorialDialogComplete()) {
                tPhase = TUTORIAL_R5_SHOP_OPEN;
            }
            break;
        }
        case TUTORIAL_R5_SHOP_OPEN: {
            RuneType runes[3] = {
                RUNE_TYPE_MAGNIFYING_GLASS,
                RUNE_TYPE_HEALTHY,
                RUNE_TYPE_ENCHANTED_AMULET
            };
            initShopWithRunes(runes);
            tPhase = TUTORIAL_R5_SHOP_WAIT;
            break;
        }
        case TUTORIAL_R5_SHOP_WAIT: {
            // The shop set gameState to SHOP; wait for it to close before playing
            // the final dialog.
            if (gameState == GAME_STATE_ACTIVE) {
                openTutorialDialog(r5PostShopPages);
                tPhase = TUTORIAL_R5_DIALOG_POST_SHOP;
            }
            break;
        }
        case TUTORIAL_R5_DIALOG_POST_SHOP: {
            if (isTutorialDialogComplete()) {
                Point center = getRoomCenter(4);
                addInteractable(center.x, center.y, INTERACTABLE_TYPE_EXIT);
                Point worldCenter = getRoomWorldCenter(4);
                setArrowPosition(worldCenter.x, worldCenter.y);
                tPhase = TUTORIAL_R5_EXIT_SPAWNED;
            }
            break;
        }
        case TUTORIAL_R5_EXIT_SPAWNED: {
            if (isTutorialDialogActive()) {
                break;
            }
            int idx = getInteractableIndexAtPosition(playerX, playerY);
            if (idx >= 0 && INTERACTABLES[idx].active && INTERACTABLES[idx].type == INTERACTABLE_TYPE_EXIT) {
                PDButtons pushed;
                pd->system->getButtonState(NULL, &pushed, NULL);
                if (pushed & kButtonA) {
                    removeSkipTutorialMenuItem();
                    tPhase = TUTORIAL_DONE;
                    setTutorialCompleted(true);
                    initRunStart();
                    switchScene(SCENE_AREA_MAP);
                }
            }
            break;
        }
        case TUTORIAL_DONE:
            break;
    }
}

void updateTutorialScene(float deltaTime) {
    tryCompleteQuitDialogInit();
    runStatsTick(deltaTime);
    if (tPhase == TUTORIAL_DONE) {
        return;
    }
    if (gameState == GAME_STATE_ACTIVE) {
        bool dialogActive = isTutorialDialogActive();
        setPlayerInputDisabled(dialogActive);
        if (prevDialogActive && !dialogActive) {
            suppressHeldPlayerButtons();
        }
        prevDialogActive = dialogActive;

        updateTutorialPhase(deltaTime);
        // Completing the tutorial swaps tutorialActive off and starts a scene
        // transition; skip the rest of the frame so the normal room-counter HUD
        // never flashes over the final frame.
        if (tPhase == TUTORIAL_DONE) {
            return;
        }
        updateTutorialReminder(deltaTime);
        updateGameplaySubsystems(deltaTime);
        drawTutorialDialog();
    } else if (gameState == GAME_STATE_SHOP) {
        updateShop(deltaTime);
    } else if (gameState == GAME_STATE_MENU) {
        updateGameMenu(deltaTime);
    } else if (gameState == GAME_STATE_QUIT_CONFIRM) {
        updateQuitConfirmDialog(deltaTime);
    } else if (gameState == GAME_STATE_QUITTING) {
        drawQuitScreenshot();
    }

    if (SETTINGS.drawFPS) {
        pd->graphics->fillRect(183, 106, 17, 14, kColorBlack);
        pd->system->drawFPS(185, 108);
    }
    pd->lua->pushNil();
}

bool getTutorialObjectiveText(StringKey *outKey, int *outCurrent, int *outTarget) {
    switch (tPhase) {
        case TUTORIAL_R1_DIALOG:
        case TUTORIAL_R1_WAITING:
            *outKey = STR_TUTORIAL_OBJ_ARROW;
            *outCurrent = r1ArrowStep;
            *outTarget = 3;
            return true;
        case TUTORIAL_R2_DIALOG:
        case TUTORIAL_R2_WAITING:
            *outKey = STR_TUTORIAL_OBJ_ENEMY;
            *outCurrent = r2Killed;
            *outTarget = 3;
            return true;
        case TUTORIAL_R4_DIALOG:
        case TUTORIAL_R4_WAITING: {
            int dashes = playerDashCount - r4DashBaseline;
            if (dashes < 0) { dashes = 0; }
            if (dashes > 3) { dashes = 3; }
            *outKey = STR_TUTORIAL_OBJ_DASH;
            *outCurrent = dashes;
            *outTarget = 3;
            return true;
        }
        default:
            return false;
    }
}
