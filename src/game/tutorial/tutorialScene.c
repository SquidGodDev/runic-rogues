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
#include "game/entities/pickups.h"
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

static bool openedRuneInv;
static bool closedRuneInv;
static bool entranceClosed;
static int r1ArrowStep;
static int r1ArrowPickupIndex;
static int r2DummyStage;
static int r3ClusterStage;
static bool prevDialogActive;

static const char *r1Pages[] = {
    "Use the d-pad to move and the crank to look around.",
    "You can dock the crank to use d-pad left and right to turn instead."
};
static const char *r2Pages[] = {
    "Press the A button to cast your basic spell.",
    "Hold A to keep casting whenever it's off cooldown."
};
static const char *r3Pages[] = {
    "Press the B button to cast your special spell.",
    "It hits harder but has a longer cooldown."
};
static const char *r4Pages[] = {
    "Double-tap a d-pad direction to dash.",
    "Dashes are limited but replenish over time.",
    "You're briefly invincible while dashing.",
    "Dash across the pit ahead!"
};
static const char *r5PreShopPages[] = {
    "Each cleared room rewards you with a choice of three runes.",
    "Try picking one!"
};
static const char *r5PostShopPages[] = {
    "You can view the runes you've collected from the system menu.",
    "Open it now to take a look."
};
static const char *r6Pages[] = {
    "A pack of slimes blocks your path. Defeat them all to finish the tutorial!"
};

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

static void spawnDummyClusterAtCell(int anchorCellX, int anchorCellY) {
    int dx[3] = {0, -1, 1};
    int dy[3] = {0, -1, -1};
    for (int i = 0; i < 3; i++) {
        int cellX = anchorCellX + dx[i];
        int cellY = anchorCellY + dy[i];
        int worldX = cellX * CELL_SIZE + CELL_SIZE / 2;
        int worldY = cellY * CELL_SIZE + CELL_SIZE / 2;
        spawnEnemy(ENEMY_TYPE_DUMMY, worldX, worldY, false);
    }
    setArrowPosition(anchorCellX * CELL_SIZE + CELL_SIZE / 2,
                     anchorCellY * CELL_SIZE + CELL_SIZE / 2);
}

static int dummiesAlive(void) {
    return getActiveEnemyCount();
}

void initTutorialScene(void) {
    initGameScene();

    tPhase = TUTORIAL_R1_DIALOG;
    r1ArrowStep = 0;
    r1ArrowPickupIndex = -1;
    r2DummyStage = 0;
    r3ClusterStage = 0;
    openedRuneInv = false;
    closedRuneInv = false;
    entranceClosed = false;
    prevDialogActive = false;

    openTutorialDialog(r1Pages);
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
                    tPhase = TUTORIAL_R1_TO_R2;
                    closeEntranceGate();
                    openRoomGate(0);
                    setArrowToRoom(1);
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
            int alive = dummiesAlive();
            if (alive == 0) {
                Point minP = getRoomInteriorMin(1);
                Point maxP = getRoomInteriorMax(1);
                if (r2DummyStage == 0) {
                    spawnDummyAtCell((int)maxP.x - 2, (int)minP.y + 2);
                    r2DummyStage = 1;
                } else if (r2DummyStage == 1) {
                    spawnDummyAtCell((int)maxP.x - 2, (int)maxP.y - 2);
                    r2DummyStage = 2;
                } else {
                    openRoomGate(1);
                    setArrowToRoom(2);
                    tPhase = TUTORIAL_R2_TO_R3;
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
                Point minP = getRoomInteriorMin(2);
                Point maxP = getRoomInteriorMax(2);
                if (r3ClusterStage == 0) {
                    spawnDummyClusterAtCell((int)maxP.x - 2, (int)minP.y + 2);
                    r3ClusterStage = 1;
                } else if (r3ClusterStage == 1) {
                    spawnDummyClusterAtCell((int)maxP.x - 2, (int)maxP.y - 2);
                    r3ClusterStage = 2;
                } else {
                    openRoomGate(2);
                    setArrowToRoom(3);
                    tPhase = TUTORIAL_R3_TO_R4;
                }
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
            Point minP = getRoomInteriorMin(3);
            Point maxP = getRoomInteriorMax(3);
            int chasmX = (minP.x + maxP.x) / 2;
            for (int y = minP.y; y <= maxP.y; y++) {
                setMapCell(chasmX, y, MAP_CHASM);
            }
            int farX = (chasmX + maxP.x) / 2;
            int farY = (minP.y + maxP.y) / 2;
            setArrowPosition(farX * CELL_SIZE + CELL_SIZE / 2, farY * CELL_SIZE + CELL_SIZE / 2);
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
            Point minP = getRoomInteriorMin(3);
            Point maxP = getRoomInteriorMax(3);
            int chasmX = (minP.x + maxP.x) / 2;
            int playerCellX = (int)playerX / CELL_SIZE;
            if (playerCellX > chasmX) {
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
            if (gameState == GAME_STATE_ACTIVE) {
                openTutorialDialog(r5PostShopPages);
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
            openTutorialDialog(r6Pages);
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
        bool dialogActive = isTutorialDialogActive();
        setPlayerInputDisabled(dialogActive);
        if (prevDialogActive && !dialogActive) {
            suppressHeldPlayerButtons();
        }
        prevDialogActive = dialogActive;

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
