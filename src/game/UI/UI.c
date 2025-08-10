#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "helpers.h"
#include "game/UI/UI.h"
#include "game/player/player.h"
#include "game/player/spells.h"
#include "game/rendering/textures.h"
#include "game/rendering/particles.h"
#include "game/map/map.h"
#include "game/enemies/enemies.h"
#include "game/entities/entities.h"
#include "game/entities/pickups.h"
#include "game/entities/projectiles.h"
#include "game/entities/interactables.h"
#include "game/settings/settings.h"
#include "game/systems/upgradeManager.h"
#include "game/systems/progressionManager.h"
#include "game/systems/waveManager.h"

#define drawPixel(frameBuffer, x, y) (frameBuffer[y * LCD_ROWSIZE + (x >> 3)] |= (1 << (7 - (x & 7))))

#define MAX_SCREEN_SHAKE_TIME 0.8f // Seconds
#define MAX_SCREEN_SHAKE_MAGNITUDE 10

#define INTERACTABLE_PROMPT_LABEL_X 100
#define INTERACTABLE_PROMPT_LABEL_Y 60
#define INTERACTABLE_BUTTON_PROMPT_X 100
#define INTERACTABLE_BUTTON_PROMPT_Y 80
#define SHOP_NAME_DESC_PADDING 4

#define EXIT_ARROW_X 100
#define EXIT_ARROW_Y 30
#define EXIT_ARROW_ROTATION_COUNT 64
#define EXIT_ARROW_ROTATION_DEGREE_STEP (2 * PI / EXIT_ARROW_ROTATION_COUNT)

#define MINIMAP_X 172
#define MINIMAP_Y 28

#define MINIMAP_SIZE 13 // Width/height in terms of cells
#define CELL_DRAW_SIZE 4 // Pixel size of each cell
#define HALF_CELL_DRAW_SIZE CELL_DRAW_SIZE / 2
#define ENTITY_DIST_SCALE_FACTOR CELL_DRAW_SIZE / CELL_SIZE
#define MINIMAP_PIXEL_SIZE MINIMAP_SIZE * CELL_DRAW_SIZE
#define MINIMAP_HALF_PIXEL_SIZE MINIMAP_PIXEL_SIZE / 2
#define ENTITY_CHECK_DISTANCE powf((MINIMAP_SIZE / 2) * CELL_SIZE - CELL_SIZE / 2, 2.0f)

#define ICON_SIZE 6

static float screenShakeTime;
static int shakeMagnitude;

static bool exitActive;
static int exitWorldX;
static int exitWorldY;

static int showBossHealthBar;

static int popupImagetableIndex;
static int popupFrame;
static int popupMaxFrame;
static float popupCurFrameTime;
static float popupFrameTime;

typedef struct SpellOverlay {
    LCDBitmapTable *bitmapTable;
    int drawX;
    int drawY;
    int currentFrame;
    int frameCount;
    float frameTime;
    float currentFrameTime;
} SpellOverlay;
SpellOverlay spellOverlays[SPELL_DECK_MAX];

void initUI(int isBossRoom) {
    screenShakeTime = 0;
    shakeMagnitude = 0;
    pd->display->setOffset(0, 0);
    pd->graphics->setBackgroundColor(kColorBlack);

    exitActive = false;
    showBossHealthBar = isBossRoom;

    popupImagetableIndex = -1;

    for (int i = 0; i < SPELL_DECK_MAX; i++) {
        spellOverlays[i] = (SpellOverlay) {
            .bitmapTable = NULL,
            .drawX = 0,
            .drawY = 0,
            .currentFrame = 0,
            .frameCount = 0,
            .frameTime = 0.0f,
            .currentFrameTime = 0.0f
        };
    }
}

void drawPlayerHearts(void) {
    int fullHeartCount = playerHealth / 2;
    int hasHalfHeart = playerHealth % 2;
    int maxHeartSlots = PLAYER_STATS.maxHealth / 2;
    int hasHalfContainer = PLAYER_STATS.maxHealth % 2;
    
    int drawX = 2;
    int drawY = 2;

    for (int i = 0; i < maxHeartSlots; i++) {
        if (i < fullHeartCount) {
            pd->graphics->drawBitmap(BITMAPS[BITMAP_FULL_HEART], drawX, drawY, kBitmapUnflipped);
        } else if (hasHalfHeart && i == fullHeartCount) {
            pd->graphics->drawBitmap(BITMAPS[BITMAP_HALF_HEART], drawX, drawY, kBitmapUnflipped);
        } else {
            pd->graphics->drawBitmap(BITMAPS[BITMAP_EMPTY_HEART], drawX, drawY, kBitmapUnflipped);
        }
        drawX += 16;
    }

    if (hasHalfContainer) {
        if (hasHalfHeart && playerHealth == PLAYER_STATS.maxHealth) {
            pd->graphics->drawBitmap(BITMAPS[BITMAP_HALF_CONTAINER_FULL_HEART], drawX, drawY, kBitmapUnflipped);
        } else {
            pd->graphics->drawBitmap(BITMAPS[BITMAP_HALF_CONTAINER_EMPTY_HEART], drawX, drawY, kBitmapUnflipped);
        }
    }
}

void drawEntityToMinimap(float entityX, float entityY, uint8_t *texture, float playerAngleCos, float playerAngleSin) {
    uint8_t* frameBuffer = pd->graphics->getFrame();
    float distSquaredToPlayer = distanceSquared(playerX, playerY, entityX, entityY);
    if (distSquaredToPlayer <= ENTITY_CHECK_DISTANCE) {
        float entityVectorX = (playerX - entityX - 0.5f);
        float entityVectorY = (playerY - entityY - 0.5f);
        // Multiply by rotation matrix to rotate
        float rotatedEntityX = entityVectorX * playerAngleCos - entityVectorY * playerAngleSin;
        float rotatedEntityY = entityVectorX * playerAngleSin + entityVectorY * playerAngleCos;

        int entityMinimapX = MINIMAP_X + rotatedEntityX * ENTITY_DIST_SCALE_FACTOR - ICON_SIZE / 2;
        int entityMinimapY = MINIMAP_Y + rotatedEntityY * ENTITY_DIST_SCALE_FACTOR - ICON_SIZE / 2;

        for (uint8_t y = 0; y < ICON_SIZE; y++) {
            for (uint8_t x = 0; x < ICON_SIZE; x++) {
                if (texture[y * ICON_SIZE + x] == kColorWhite) {
                    drawPixel(frameBuffer, (entityMinimapX + x), (entityMinimapY + y));
                }
            }
        }
    }
}

void drawMinimap(void) {
    // Angle that matches orientation of walls
    float rotatedPlayerAngle = 2*PI - playerAngle - P2;

    // Draw background
    pd->graphics->drawBitmap(BITMAPS[BITMAP_MINIMAP_BORDER], MINIMAP_X - MINIMAP_HALF_PIXEL_SIZE, MINIMAP_Y - MINIMAP_HALF_PIXEL_SIZE, kBitmapUnflipped);

    if (SETTINGS.drawMinimapWalls) {
        // == Wall layer ==
        LCDBitmap *minimapBitmap = pd->graphics->newBitmap(MINIMAP_PIXEL_SIZE, MINIMAP_PIXEL_SIZE, kColorClear);

        pd->graphics->pushContext(minimapBitmap);
        int playerMapX = playerX / CELL_SIZE;
        int playerMapY = playerY / CELL_SIZE;
        int playerOffsetX = roundf((playerX - playerMapX * CELL_SIZE) / CELL_DRAW_SIZE) + CELL_DRAW_SIZE / 2;
        int playerOffsetY = roundf((playerY - playerMapY * CELL_SIZE) / CELL_DRAW_SIZE) + CELL_DRAW_SIZE / 2;

        // Draw walls
        LCDBitmap *minimapWallBitmap = BITMAPS[BITMAP_MINIMAP_WALL];
        for (int mapY = 0; mapY < MINIMAP_SIZE + 1; mapY++) {
            int adjustedMapY = playerMapY - (MINIMAP_SIZE + 1) / 2 + mapY;
            if (adjustedMapY < 0 || adjustedMapY >= MAP_HEIGHT) {
                continue;
            }
            for (int mapX = 0; mapX < MINIMAP_SIZE + 1; mapX++) {
                int adjustedMapX = playerMapX - (MINIMAP_SIZE + 1) / 2 + mapX;
                if (adjustedMapX < 0 || adjustedMapX >= MAP_WIDTH) {
                    continue;
                }
                int drawX = mapX * CELL_DRAW_SIZE - playerOffsetX;
                int drawY = mapY * CELL_DRAW_SIZE - playerOffsetY;
                if (isBlocked(adjustedMapX, adjustedMapY)) {
                    pd->graphics->drawBitmap(minimapWallBitmap, drawX, drawY, kBitmapUnflipped);
                }
            }
        }

        pd->graphics->popContext();

        // Set circle mask
        LCDBitmap *baseLayerMask = pd->graphics->getBitmapMask(minimapBitmap);
        pd->graphics->pushContext(baseLayerMask);
        pd->graphics->drawBitmap(BITMAPS[BITMAP_MINIMAP_MASK], 0, 0, kBitmapUnflipped);
        pd->graphics->popContext();
        pd->graphics->setBitmapMask(minimapBitmap, baseLayerMask);

        // Draw wall layer
        pd->graphics->drawRotatedBitmap(minimapBitmap, MINIMAP_X, MINIMAP_Y, radToDeg(rotatedPlayerAngle), 0.5f, 0.5f, 1.0f, 1.0f);
        pd->graphics->freeBitmap(minimapBitmap);
        pd->graphics->freeBitmap(baseLayerMask);
        // == End of wall layer ==
    }

    // == Entity layer ==
    float playerAngleCos = cosf(rotatedPlayerAngle + PI);
    float playerAngleSin = sinf(rotatedPlayerAngle + PI);
    // Draw enemies
    for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
        Enemy curEnemy = enemies[i];
        if (!curEnemy.active || curEnemy.state == ENEMY_STATE_DEAD) {
            continue;
        }

        drawEntityToMinimap(curEnemy.x, curEnemy.y, TEXTURES[TEXTURE_MINIMAP_ENEMY], playerAngleCos, playerAngleSin);
    }
    
    // Draw interactables
    for (int i = 0; i < MAX_INTERACTABLE_COUNT; i++) {
        Interactable curInteractable = INTERACTABLES[i];
        if (!curInteractable.active) {
            continue;
        }

        if (curInteractable.type == INTERACTABLE_TYPE_EXIT) {
            drawEntityToMinimap(curInteractable.x, curInteractable.y, TEXTURES[TEXTURE_MINIMAP_EXIT], playerAngleCos, playerAngleSin);
        } else if (curInteractable.type == INTERACTABLE_TYPE_CAMPFIRE || curInteractable.type == INTERACTABLE_TYPE_EXTINGUISHED_CAMPFIRE) {
            drawEntityToMinimap(curInteractable.x, curInteractable.y, TEXTURES[TEXTURE_MINIMAP_CAMPFIRE], playerAngleCos, playerAngleSin);
        } else if (curInteractable.type == INTERACTABLE_TYPE_SHOP_ITEM) {
            drawEntityToMinimap(curInteractable.x, curInteractable.y, TEXTURES[TEXTURE_MINIMAP_SHOP_ITEM], playerAngleCos, playerAngleSin);
        }
    }
    // Draw projectiles
    for (int i = 0; i < MAX_PROJECTILE_COUNT; i++) {
        Projectile curProjectile = projectiles[i];
        if (!curProjectile.active || !curProjectile.isEnemy) {
            continue;
        }

        drawEntityToMinimap(curProjectile.x, curProjectile.y, TEXTURES[TEXTURE_MINIMAP_PROJECTILE], playerAngleCos, playerAngleSin);
    }
    // Draw warnings
    for (int i = 0; i < MAX_PARTICLE_COUNT; i++) {
        Particle curParticle = particles[i];
        if (!curParticle.active || curParticle.type != PARTICLE_TYPE_WARNING) {
            continue;
        }

        drawEntityToMinimap(curParticle.x, curParticle.y, TEXTURES[TEXTURE_MINIMAP_WARNING], playerAngleCos, playerAngleSin);
    }
    // Draw pickups
    for (int i = 0; i < MAX_PICKUP_COUNT; i++) {
        Pickup curPickup = pickups[i];
        if (!curPickup.active) {
            continue;
        }

        drawEntityToMinimap(curPickup.x, curPickup.y, TEXTURES[TEXTURE_MINIMAP_CARD], playerAngleCos, playerAngleSin);
    }
    // == End of entity layer ==
}

void drawInteractablePrompt(void) {
    int playerInteractableIndex = getPlayerInteractableIndex();
    if (playerInteractableIndex < 0 || playerInteractableIndex >= MAX_INTERACTABLE_COUNT) {
        return;
    }

    Interactable curInteractable = INTERACTABLES[playerInteractableIndex];
    if (!curInteractable.active) {
        return;
    }

    if (curInteractable.type == INTERACTABLE_TYPE_EXIT) {
        if (SETTINGS.language == LANGUAGE_JP) {
            drawCenteredBitmap(BITMAPS[BITMAP_PROMPT_EXIT_JP], INTERACTABLE_PROMPT_LABEL_X, INTERACTABLE_PROMPT_LABEL_Y);
        } else {
            drawCenteredBitmap(BITMAPS[BITMAP_PROMPT_EXIT], INTERACTABLE_PROMPT_LABEL_X, INTERACTABLE_PROMPT_LABEL_Y);
        }
        drawCenteredBitmap(BITMAPS[BITMAP_PROMPT_A_BUTTON], INTERACTABLE_BUTTON_PROMPT_X, INTERACTABLE_BUTTON_PROMPT_Y);
    } else if (curInteractable.type == INTERACTABLE_TYPE_CAMPFIRE) {
        if (SETTINGS.language == LANGUAGE_JP) {
            drawCenteredBitmap(BITMAPS[BITMAP_PROMPT_REST_JP], INTERACTABLE_PROMPT_LABEL_X, INTERACTABLE_PROMPT_LABEL_Y);
        } else {
            drawCenteredBitmap(BITMAPS[BITMAP_PROMPT_REST], INTERACTABLE_PROMPT_LABEL_X, INTERACTABLE_PROMPT_LABEL_Y);
        }
        drawCenteredBitmap(BITMAPS[BITMAP_PROMPT_A_BUTTON], INTERACTABLE_BUTTON_PROMPT_X, INTERACTABLE_BUTTON_PROMPT_Y);
    } else if (curInteractable.type == INTERACTABLE_TYPE_SHOP_ITEM) {
        if (SETTINGS.language == LANGUAGE_JP) {
            drawCenteredBitmap(curInteractable.shopItem.shopPageBitmapJP, INTERACTABLE_PROMPT_LABEL_X, INTERACTABLE_PROMPT_LABEL_Y);
        } else {
            drawCenteredBitmap(curInteractable.shopItem.shopPageBitmapEN, INTERACTABLE_PROMPT_LABEL_X, INTERACTABLE_PROMPT_LABEL_Y);
        }
    }
}

void setExitArrowPosition(int worldX, int worldY) {
    exitActive = true;
    exitWorldX = worldX;
    exitWorldY = worldY;
}

void drawExitArrow(void) {
    if (!exitActive) {
        return;
    }

    float angleToExit = atan2f(exitWorldY - playerY, exitWorldX - playerX) - playerAngle;
    int rotationTableIndex = (int)(angleToExit / EXIT_ARROW_ROTATION_DEGREE_STEP) % EXIT_ARROW_ROTATION_COUNT;
    if (rotationTableIndex < 0) {
        rotationTableIndex += EXIT_ARROW_ROTATION_COUNT;
    }

    drawCenteredBitmap(pd->graphics->getTableBitmap(BITMAP_TABLES[BITMAP_TABLE_EXIT_ARROW], rotationTableIndex), EXIT_ARROW_X, EXIT_ARROW_Y);
}

int spellDrawX[SPELL_DECK_MAX] = {22, 6, 22, 38, 22};
int spellDrawY[SPELL_DECK_MAX] = {71, 71, 53, 71, 89};
void drawSpellUI(void) {
    int spellIconHeight = 14;
    int spellIconWidth = 10;
    for (int i = 0; i < SPELL_DECK_MAX; i++) {
        Spell heldSpell = getDeckSpell(i);
        LCDBitmap *spellIcon = getSpellIcon(heldSpell);
        int drawX = spellDrawX[i];
        int drawY = spellDrawY[i];
        pd->graphics->drawBitmap(spellIcon, drawX, drawY, kBitmapUnflipped);
        float cooldownPercentage = getSpellCooldownPercentage(i);
        if (cooldownPercentage > 0.0f) {
            int cooldownCoverHeight = cooldownPercentage * spellIconHeight;
            pd->graphics->fillRect(drawX + 1, drawY + 1, spellIconWidth, cooldownCoverHeight, kColorBlack);
        }
    }

    int utilityIconBaseX = 39, utilityIconBaseY = 98;
    int utilityIconGap = 8;
    for (int i = 0; i < getMaxUtilitySpells(); i++) {
        LCDBitmap *utilitySpellBitmap = i < getUtilitySpellCount() ? BITMAPS[BITMAP_DASH_INDICATOR_FULL] : BITMAPS[BITMAP_DASH_INDICATOR_EMPTY];
        pd->graphics->drawBitmap(utilitySpellBitmap, utilityIconBaseX + i * utilityIconGap, utilityIconBaseY, kBitmapUnflipped);
    }

    int utilitySpellChargeBarX = utilityIconBaseX + getMaxUtilitySpells() * utilityIconGap;
    int utilitySpellChargeBarY = utilityIconBaseY;
    int utilitySpellChargeBarWidth = 2;
    int utilityChargeBarHeight = 5;
    pd->graphics->drawBitmap(BITMAPS[BITMAP_UTILITY_CHARGE_BAR], utilitySpellChargeBarX, utilitySpellChargeBarY, kBitmapUnflipped);
    if (getUtilitySpellCharge() > 0.0f) {
        int chargeBarHeight = getUtilitySpellCharge() * utilityChargeBarHeight + 1;
        int utilityChargeBarDrawY = utilitySpellChargeBarY + utilityChargeBarHeight - chargeBarHeight + 1;
        pd->graphics->fillRect(utilitySpellChargeBarX + 1, utilityChargeBarDrawY, utilitySpellChargeBarWidth, chargeBarHeight, kColorWhite);
    }
}

void drawBossHealthBar(void) {
    float healthPercentage = getBossHealthPercentage();
    int healthBarWidth = healthPercentage * 68;

    pd->graphics->drawBitmap(BITMAPS[BITMAP_BOSS_HEALTH_BAR], 64, 28, kBitmapUnflipped);
    pd->graphics->fillRect(66, 30, healthBarWidth, 4, kColorWhite);
    
    int titleDrawX = 100;
    int titleDrawY = 21;
    if (SETTINGS.language == LANGUAGE_EN) {
        if (getCurrentArea() == AREA_ECHOING_CAVERNS) {
            drawCenteredBitmap(BITMAPS[BITMAP_ELDER_BOSS_TITLE], titleDrawX, titleDrawY);
        } else if (getCurrentArea() == AREA_LOST_ARCHIVES) {
            drawCenteredBitmap(BITMAPS[BITMAP_WRAITH_BOSS_TITLE], titleDrawX, titleDrawY);
        }
    } else if (SETTINGS.language == LANGUAGE_JP) {
        if (getCurrentArea() == AREA_ECHOING_CAVERNS) {
            drawCenteredBitmap(BITMAPS[BITMAP_ELDER_BOSS_TITLE_JP], titleDrawX, titleDrawY);
        } else if (getCurrentArea() == AREA_LOST_ARCHIVES) {
            drawCenteredBitmap(BITMAPS[BITMAP_WRAITH_BOSS_TITLE_JP], titleDrawX, titleDrawY);
        }
    }
}

void drawWaveProgressBar(void) {
    float wavePercentage = getWavePercentage();
    int waveBarWidth = wavePercentage * 68;

    int progressBarX = 64;
    int progressBarY = 20;
    pd->graphics->drawBitmap(BITMAPS[BITMAP_BOSS_HEALTH_BAR], progressBarX, progressBarY, kBitmapUnflipped);
    pd->graphics->fillRect(progressBarX + 2, progressBarY + 2, waveBarWidth, 4, kColorWhite);
}

void setSpellOverlay(int index, int bitmapTableIndex, float frameTime, int drawX, int drawY) {
    LCDBitmapTable *bitmapTable = BITMAP_TABLES[bitmapTableIndex];
    int frameCount;
    pd->graphics->getBitmapTableInfo(bitmapTable, NULL, &frameCount);

    spellOverlays[index] = (SpellOverlay) {
        .bitmapTable = bitmapTable,
        .drawX = drawX,
        .drawY = drawY,
        .currentFrame = 0,
        .frameCount = frameCount,
        .frameTime = frameTime,
        .currentFrameTime = 0.0f
    };
}

void clearSpellOverlay(int index) {
    spellOverlays[index].bitmapTable = NULL;
}

void drawSpellOverlay(float deltaTime) {
    for (int i = 0; i < SPELL_DECK_MAX; i++) {
        SpellOverlay *overlay = &spellOverlays[i];
        if (overlay->bitmapTable == NULL) {
            continue;
        }

        overlay->currentFrameTime += deltaTime;
        if (overlay->currentFrameTime >= overlay->frameTime) {
            overlay->currentFrame++;
            if (overlay->currentFrame >= overlay->frameCount) {
                overlay->currentFrame = 0;
            }
            overlay->currentFrameTime = 0;
        }

        LCDBitmap *overlayBitmap = pd->graphics->getTableBitmap(overlay->bitmapTable, overlay->currentFrame);
        pd->graphics->drawBitmap(overlayBitmap, overlay->drawX, overlay->drawY, kBitmapUnflipped);
    }
}

void showRoomClearedPopup(void) {
    if (SETTINGS.language == LANGUAGE_EN) {
        popupImagetableIndex = BITMAP_TABLE_ROOM_CLEARED_POPUP_EN;
    } else if (SETTINGS.language == LANGUAGE_JP) {
        popupImagetableIndex = BITMAP_TABLE_ROOM_CLEARED_POPUP_JP;
    }
    popupMaxFrame = 30;
    popupFrame = 0;
    popupCurFrameTime = 0;
    popupFrameTime = 0.05f;
}

void drawPopup(float deltaTime) {
    if (popupImagetableIndex != -1) {
        popupCurFrameTime += deltaTime;
        if (popupCurFrameTime >= popupFrameTime) {
            popupCurFrameTime = 0;
            popupFrame++;
            if (popupFrame >= popupMaxFrame) {
                popupImagetableIndex = -1;
                return;
            }
        }

        LCDBitmap *popupBitmap = pd->graphics->getTableBitmap(BITMAP_TABLES[popupImagetableIndex], popupFrame);
        drawCenteredBitmap(popupBitmap, 100, 60);
    }
}

void drawUI(float deltaTime) {
    if (screenShakeTime > 0) {
        screenShakeTime -= deltaTime;
        if (screenShakeTime <= 0) {
            screenShakeTime = 0;
            pd->display->setOffset(0, 0);
        } else {
            int curShakeMagnitude = screenShakeTime * shakeMagnitude;
            int shakeX = randInt(-curShakeMagnitude, curShakeMagnitude);
            int shakeY = randInt(-curShakeMagnitude, curShakeMagnitude);
            pd->display->setOffset(shakeX, shakeY);
        }
    }

    drawPlayerHearts();
    drawMinimap();
    drawSpellOverlay(deltaTime);
    drawWeapon(deltaTime);
    drawSpellUI();
    drawExitArrow();
    drawPopup(deltaTime);
    drawInteractablePrompt();

    if (!isRoomCompleted()) {
        if (showBossHealthBar) {
            drawBossHealthBar();
        } else {
            drawWaveProgressBar();
        }
    }
}

void screenShake(ScreenShakeStrength strength) {
    switch (strength) {
        case SCREEN_SHAKE_LIGHT:
            screenShakeTime = MAX_SCREEN_SHAKE_TIME * 0.5f;
            shakeMagnitude = MAX_SCREEN_SHAKE_MAGNITUDE * 0.5f;
            break;
        case SCREEN_SHAKE_MEDIUM:
            screenShakeTime = MAX_SCREEN_SHAKE_TIME * 0.75f;
            shakeMagnitude = MAX_SCREEN_SHAKE_MAGNITUDE * 0.75f;
            break;
        case SCREEN_SHAKE_STRONG:
            screenShakeTime = MAX_SCREEN_SHAKE_TIME;
            shakeMagnitude = MAX_SCREEN_SHAKE_MAGNITUDE;
            break;
    }
}