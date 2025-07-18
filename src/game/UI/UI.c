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

#define drawPixel(frameBuffer, x, y) (frameBuffer[y * LCD_ROWSIZE + (x >> 3)] |= (1 << (7 - (x & 7))))

#define MAX_SCREEN_SHAKE_TIME 0.8f // Seconds
#define MAX_SCREEN_SHAKE_MAGNITUDE 10

#define INTERACTABLE_PROMPT_LABEL_X 100
#define INTERACTABLE_PROMPT_LABEL_Y 60
#define INTERACTABLE_BUTTON_PROMPT_X 100
#define INTERACTABLE_BUTTON_PROMPT_Y 80

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

static int exitWorldX;
static int exitWorldY;

static int showBossHealthBar;

typedef struct SpellOverlay {
    LCDBitmapTable *bitmapTable;
    int drawX;
    int drawY;
    int currentFrame;
    int frameCount;
    float frameTime;
    float currentFrameTime;
} SpellOverlay;
SpellOverlay currentSpellOverlay;

void initUI(int isBossRoom) {
    screenShakeTime = 0;
    shakeMagnitude = 0;
    pd->display->setOffset(0, 0);
    pd->graphics->setBackgroundColor(kColorBlack);

    showBossHealthBar = isBossRoom;
    currentSpellOverlay = (SpellOverlay) {0};
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

    switch (curInteractable.type) {
        case INTERACTABLE_TYPE_EXIT:
            if (SETTINGS.language == LANGUAGE_JP) {
                drawCenteredBitmap(BITMAPS[BITMAP_PROMPT_EXIT_JP], INTERACTABLE_PROMPT_LABEL_X, INTERACTABLE_PROMPT_LABEL_Y);
            } else {
                drawCenteredBitmap(BITMAPS[BITMAP_PROMPT_EXIT], INTERACTABLE_PROMPT_LABEL_X, INTERACTABLE_PROMPT_LABEL_Y);
            }
            drawCenteredBitmap(BITMAPS[BITMAP_PROMPT_A_BUTTON], INTERACTABLE_BUTTON_PROMPT_X, INTERACTABLE_BUTTON_PROMPT_Y);
            break;
        case INTERACTABLE_TYPE_CAMPFIRE:
            if (SETTINGS.language == LANGUAGE_JP) {
                drawCenteredBitmap(BITMAPS[BITMAP_PROMPT_REST_JP], INTERACTABLE_PROMPT_LABEL_X, INTERACTABLE_PROMPT_LABEL_Y);
            } else {
                drawCenteredBitmap(BITMAPS[BITMAP_PROMPT_REST], INTERACTABLE_PROMPT_LABEL_X, INTERACTABLE_PROMPT_LABEL_Y);
            }
            drawCenteredBitmap(BITMAPS[BITMAP_PROMPT_A_BUTTON], INTERACTABLE_BUTTON_PROMPT_X, INTERACTABLE_BUTTON_PROMPT_Y);
            break;
        default:
            break;
    }
}

void setExitArrowPosition(int worldX, int worldY) {
    exitWorldX = worldX;
    exitWorldY = worldY;
}

void drawExitArrow(void) {
    if (!isRoomCompleted()) {
        return;
    }

    float angleToExit = atan2f(exitWorldY - playerY, exitWorldX - playerX) - playerAngle;
    int rotationTableIndex = (int)(angleToExit / EXIT_ARROW_ROTATION_DEGREE_STEP) % EXIT_ARROW_ROTATION_COUNT;
    if (rotationTableIndex < 0) {
        rotationTableIndex += EXIT_ARROW_ROTATION_COUNT;
    }

    drawCenteredBitmap(pd->graphics->getTableBitmap(BITMAP_TABLES[BITMAP_TABLE_EXIT_ARROW], rotationTableIndex), EXIT_ARROW_X, EXIT_ARROW_Y);
}

void drawSpellUI(void) {
    int manaBarX = 5, manaBarY = 108;
    pd->graphics->drawBitmap(BITMAPS[BITMAP_MANA_BAR], manaBarX, manaBarY, kBitmapUnflipped);
    int manaBarWidth = getManaPercentage() * 65;
    pd->graphics->fillRect(manaBarX + 2, manaBarY + 2, manaBarWidth, 6, kColorWhite);

    int spellIconBaseX = 6, spellIconBaseY = 91;
    int spellIconGap = 14;
    for (int i = 0; i < getMaxHeldSpells(); i++) {
        Spell heldSpell = getHeldSpell(i);
        LCDBitmap *spellIcon = getSpellIcon(heldSpell);
        pd->graphics->drawBitmap(spellIcon, spellIconBaseX + i * spellIconGap, spellIconBaseY, kBitmapUnflipped);
    }

    int selectedSpellIndex = getSelectedSpellIndex();
    pd->graphics->drawBitmap(BITMAPS[BITMAP_SELECTED_SPELL_CURSOR], spellIconBaseX + selectedSpellIndex * spellIconGap - 2, spellIconBaseY - 2, kBitmapUnflipped);

    int utilityIconBaseX = 6, utilityIconBaseY = 83;
    int utilityIconGap = 8;
    for (int i = 0; i < getMaxUtilitySpells(); i++) {
        LCDBitmap *utilitySpellBitmap = i < getUtilitySpellCount() ? BITMAPS[BITMAP_DASH_INDICATOR_FULL] : BITMAPS[BITMAP_DASH_INDICATOR_EMPTY];
        pd->graphics->drawBitmap(utilitySpellBitmap, utilityIconBaseX + i * utilityIconGap, utilityIconBaseY, kBitmapUnflipped);
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

void setSpellOverlay(int bitmapTableIndex, float frameTime, int drawX, int drawY) {
    LCDBitmapTable *bitmapTable = BITMAP_TABLES[bitmapTableIndex];
    int frameCount;
    pd->graphics->getBitmapTableInfo(bitmapTable, NULL, &frameCount);
    currentSpellOverlay.bitmapTable = bitmapTable;
    currentSpellOverlay.frameCount = frameCount;
    currentSpellOverlay.frameTime = frameTime;
    currentSpellOverlay.drawX = drawX;
    currentSpellOverlay.drawY = drawY;
    currentSpellOverlay.currentFrame = 0;
    currentSpellOverlay.currentFrameTime = 0.0f;
}

void clearSpellOverlay(void) {
    currentSpellOverlay.bitmapTable = NULL;
}

void drawSpellOverlay(float deltaTime) {
    if (currentSpellOverlay.bitmapTable == NULL) {
        return;
    }

    currentSpellOverlay.currentFrameTime += deltaTime;
    if (currentSpellOverlay.currentFrameTime >= currentSpellOverlay.frameTime) {
        currentSpellOverlay.currentFrame++;
        if (currentSpellOverlay.currentFrame >= currentSpellOverlay.frameCount) {
            currentSpellOverlay.currentFrame = 0;
        }
        currentSpellOverlay.currentFrameTime = 0;
    }

    LCDBitmap *overlayBitmap = pd->graphics->getTableBitmap(currentSpellOverlay.bitmapTable, currentSpellOverlay.currentFrame);
    pd->graphics->drawBitmap(overlayBitmap, currentSpellOverlay.drawX, currentSpellOverlay.drawY, kBitmapUnflipped);
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
    drawInteractablePrompt();
    drawExitArrow();

    if (showBossHealthBar && !isRoomCompleted()) {
        drawBossHealthBar();
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