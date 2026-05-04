#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/helpers.h"
#include "globals/sceneManager.h"

#include "game/gameScene.h"
#include "game/map/map.h"
#include "game/UI/UI.h"
#include "game/UI/runeInventory.h"
#include "game/player/player.h"
#include "game/enemies/enemies.h"
#include "game/entities/interactables.h"
#include "game/systems/runStats.h"
#include "game/systems/shopManager.h"
#include "game/systems/runeManager.h"
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
    TUTORIAL_R5_WAITING,
    TUTORIAL_R5_TO_R6,
    TUTORIAL_R6_ENTER,
    TUTORIAL_R6_DIALOG,
    TUTORIAL_R6_WAITING,
    TUTORIAL_R6_EXIT_SPAWNED,
    TUTORIAL_DONE
} TutorialPhase;

static TutorialPhase tPhase;

static bool dpadPressed;
static bool crankMoved;
static int dashBaseline;
static bool openedRuneInv;
static bool closedRuneInv;
static bool entranceClosed;

static const char *r1Pages[] = {
    "Use the d-pad to move and the crank to look around.",
    "You can dock the crank to use d-pad left and right to turn instead."
};
static const char *r2Pages[] = {
    "Press the A button to cast your basic spell. Hold A to keep casting whenever it's off cooldown."
};
static const char *r3Pages[] = {
    "Press the B button to cast your special spell. It hits harder but has a longer cooldown."
};
static const char *r4Pages[] = {
    "Double-tap a d-pad direction to dash. Dashes are limited but replenish over time, and dashing makes you briefly invincible. Try dashing a few times!"
};
static const char *r5PreShopPages[] = {
    "Each cleared room rewards you with a choice of three runes that grant powerful upgrades. Pick one to take with you for the rest of the run."
};
static const char *r5PostShopPages[] = {
    "You can view the runes you've collected from the system menu. Open it now to take a look."
};
static const char *r6Pages[] = {
    "A pack of slimes blocks your path. Defeat them all to finish the tutorial!"
};

static void setArrowToRoom(int roomIndex) {
    Point p = getRoomWorldCenter(roomIndex);
    setArrowPosition(p.x, p.y);
}

static int dummiesAlive(void) {
    return getActiveEnemyCount();
}

void initTutorialScene(void) {
    initGameScene();

    tPhase = TUTORIAL_R1_DIALOG;
    dpadPressed = false;
    crankMoved = false;
    dashBaseline = 0;
    openedRuneInv = false;
    closedRuneInv = false;
    entranceClosed = false;

    openTutorialDialog(r1Pages, 1);
}

static void closeBehindIfEntered(int roomIndex, int gateIndex) {
    if (!entranceClosed && roomIndex == 1) {
        closeEntranceGate();
        entranceClosed = true;
    }
    closeRoomGate(gateIndex);
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
            PDButtons current;
            pd->system->getButtonState(&current, NULL, NULL);
            if (current & (kButtonUp | kButtonDown | kButtonLeft | kButtonRight)) {
                dpadPressed = true;
            }
            float crank = pd->system->getCrankChange();
            if (fabsf(crank) > 0.5f) {
                crankMoved = true;
            }
            if (dpadPressed || crankMoved) {
                tPhase = TUTORIAL_R1_TO_R2;
                openRoomGate(0);
                setArrowToRoom(1);
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
            Point center = getRoomWorldCenter(1);
            spawnEnemy(ENEMY_TYPE_DUMMY, (int)center.x, (int)center.y, false);
            openTutorialDialog(r2Pages, 1);
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
            int alive = dummiesAlive();
            if (alive == 0) {
                openRoomGate(1);
                setArrowToRoom(2);
                tPhase = TUTORIAL_R2_TO_R3;
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
            for (int i = 0; i < 3; i++) {
                Point p = getRoomSpawnPosition(2, i);
                spawnEnemy(ENEMY_TYPE_DUMMY, (int)p.x, (int)p.y, false);
            }
            openTutorialDialog(r3Pages, 1);
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
            int killed = 3 - alive;
            if (killed < 0) { killed = 0; }
            if (alive == 0) {
                openRoomGate(2);
                setArrowToRoom(3);
                tPhase = TUTORIAL_R3_TO_R4;
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
            dashBaseline = playerDashCount;
            openTutorialDialog(r4Pages, 1);
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
            int dashed = playerDashCount - dashBaseline;
            if (dashed < 0) { dashed = 0; }
            if (dashed > 3) { dashed = 3; }
            if (dashed >= 3) {
                openRoomGate(3);
                setArrowToRoom(4);
                tPhase = TUTORIAL_R4_TO_R5;
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
            openTutorialDialog(r5PreShopPages, 1);
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
            if (gameState == GAME_STATE_ACTIVE) {
                openTutorialDialog(r5PostShopPages, 1);
                tPhase = TUTORIAL_R5_DIALOG_POST_SHOP;
            }
            break;
        }
        case TUTORIAL_R5_DIALOG_POST_SHOP: {
            if (isTutorialDialogComplete()) {
                tPhase = TUTORIAL_R5_WAITING;
            }
            break;
        }
        case TUTORIAL_R5_WAITING: {
            if (gameState == GAME_STATE_RUNE_INVENTORY) {
                openedRuneInv = true;
            } else if (openedRuneInv && gameState == GAME_STATE_ACTIVE) {
                closedRuneInv = true;
            }
            if (openedRuneInv && closedRuneInv) {
                openRoomGate(4);
                setArrowToRoom(5);
                tPhase = TUTORIAL_R5_TO_R6;
            }
            break;
        }
        case TUTORIAL_R5_TO_R6: {
            if (getPlayerCurrentRoom() == 5) {
                tPhase = TUTORIAL_R6_ENTER;
            }
            break;
        }
        case TUTORIAL_R6_ENTER: {
            closeRoomGate(4);
            for (int i = 0; i < 3; i++) {
                Point p = getRoomSpawnPosition(5, i);
                spawnEnemy(ENEMY_TYPE_SLIME, (int)p.x, (int)p.y, false);
            }
            openTutorialDialog(r6Pages, 1);
            tPhase = TUTORIAL_R6_DIALOG;
            break;
        }
        case TUTORIAL_R6_DIALOG: {
            if (isTutorialDialogComplete()) {
                tPhase = TUTORIAL_R6_WAITING;
            }
            break;
        }
        case TUTORIAL_R6_WAITING: {
            int alive = getActiveEnemyCount();
            int killed = 3 - alive;
            if (killed < 0) { killed = 0; }
            if (alive == 0) {
                Point center = getRoomCenter(5);
                addInteractable(center.x, center.y, INTERACTABLE_TYPE_EXIT);
                Point worldCenter = getRoomWorldCenter(5);
                setArrowPosition(worldCenter.x, worldCenter.y);
                tPhase = TUTORIAL_R6_EXIT_SPAWNED;
            }
            break;
        }
        case TUTORIAL_R6_EXIT_SPAWNED: {
            int idx = getInteractableIndexAtPosition(playerX, playerY);
            if (idx >= 0 && INTERACTABLES[idx].active && INTERACTABLES[idx].type == INTERACTABLE_TYPE_EXIT) {
                tPhase = TUTORIAL_DONE;
                removeRuneInventoryMenuItem();
                freeRuneInventoryCache();
                clearArrow();
                switchScene(SCENE_TITLE);
            }
            break;
        }
        case TUTORIAL_DONE:
            break;
    }
}

void updateTutorialScene(float deltaTime) {
    runStatsTick(deltaTime);
    if (gameState == GAME_STATE_ACTIVE) {
        updateTutorialPhase(deltaTime);
        updateGameplaySubsystems(deltaTime);
        drawTutorialDialog();
    } else if (gameState == GAME_STATE_SHOP) {
        updateShop(deltaTime);
    } else if (gameState == GAME_STATE_RUNE_INVENTORY) {
        updateRuneInventory(deltaTime);
        if (tPhase == TUTORIAL_R5_WAITING) {
            openedRuneInv = true;
        }
    }

    if (SETTINGS.drawFPS) {
        pd->graphics->fillRect(183, 106, 17, 14, kColorBlack);
        pd->system->drawFPS(185, 108);
    }
    pd->lua->pushNil();
}
