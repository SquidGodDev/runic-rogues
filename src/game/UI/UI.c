#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/helpers.h"
#include "globals/localization.h"
#include "game/UI/UI.h"
#include "game/UI/bossTitle.h"
#include "game/UI/runeHUD.h"
#include "game/player/player.h"
#include "game/player/playerStats.h"
#include "game/player/spells/spells.h"
#include "game/rendering/textures.h"
#include "game/rendering/particles.h"
#include "game/map/map.h"
#include "game/enemies/enemies.h"
#include "game/entities/entities.h"
#include "game/entities/pickups.h"
#include "game/entities/projectiles.h"
#include "game/entities/interactables.h"
#include "game/settings/settings.h"
#include "game/systems/progressionManager.h"
#include "game/systems/spawnManager.h"
#include "game/systems/bossCinematic.h"
#include "game/tutorial/tutorialScene.h"

#define drawPixel(frameBuffer, x, y) (frameBuffer[y * LCD_ROWSIZE + (x >> 3)] |= (1 << (7 - (x & 7))))

#define MAX_SCREEN_SHAKE_TIME 0.8f // Seconds
#define MAX_SCREEN_SHAKE_MAGNITUDE 10

#define INTERACTABLE_PROMPT_LABEL_X 100
#define INTERACTABLE_PROMPT_LABEL_Y 60
#define INTERACTABLE_BUTTON_PROMPT_X 100
#define INTERACTABLE_BUTTON_PROMPT_Y 80
#define SHOP_NAME_DESC_X 23
#define SHOP_NAME_DESC_Y 54

#define EXIT_ARROW_X 100
#define EXIT_ARROW_Y 30
#define EXIT_ARROW_ROTATION_COUNT 64
#define EXIT_ARROW_ROTATION_DEGREE_STEP (2 * PI / EXIT_ARROW_ROTATION_COUNT)
#define EXIT_ARROW_BOUNCE_HOPS 2
#define EXIT_ARROW_BOUNCE_HOP_FRAMES 6
#define EXIT_ARROW_BOUNCE_FRAMES (EXIT_ARROW_BOUNCE_HOPS * EXIT_ARROW_BOUNCE_HOP_FRAMES)
#define EXIT_ARROW_BOUNCE_HEIGHT 3

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

#define MAX_UI_ANIMATIONS 5

#define MAX_DIRECTION_INDICATORS 10

#define DAMAGE_INDICATOR_FLASH_TIME 0.3f
#define DAMAGE_INDICATOR_HOLD_TIME  0.6f
#define DAMAGE_INDICATOR_FADE_TIME  0.3f
#define DAMAGE_INDICATOR_DURATION   (DAMAGE_INDICATOR_FLASH_TIME + DAMAGE_INDICATOR_HOLD_TIME + DAMAGE_INDICATOR_FADE_TIME)

#define ATTACK_INTENT_FADE_IN_TIME  0.0f
#define ATTACK_INTENT_FLASH_TIME    1.0f
#define ATTACK_INTENT_FADE_OUT_TIME 0.3f
#define ATTACK_INTENT_MIN_DURATION  (ATTACK_INTENT_FADE_IN_TIME + ATTACK_INTENT_FLASH_TIME + ATTACK_INTENT_FADE_OUT_TIME)

#define DAMAGE_INDICATOR_RADIUS 22
#define ATTACK_INTENT_INDICATOR_RADIUS 30
#define INDICATOR_RADIUS 30
#define INDICATOR_ARC_SPAN 0.6f
#define INDICATOR_ARC_SEGMENTS 8
#define INDICATOR_THICKNESS 3
#define INDICATOR_OUTLINE 3
#define INDICATOR_ARROW_LENGTH 8
#define INDICATOR_ARROW_SPREAD 5

typedef enum {
    INDICATOR_TYPE_DAMAGE,
    INDICATOR_TYPE_ATTACK_INTENT,
} IndicatorType;

typedef struct DirectionIndicator {
    float sourceX;
    float sourceY;
    float timer;
    float totalDuration;
    IndicatorType type;
} DirectionIndicator;

static DirectionIndicator directionIndicators[MAX_DIRECTION_INDICATORS];

static float screenShakeTime;
static int shakeMagnitude;

#define HEALTH_FLASH_TIME 0.2f
static float healthFlashTimer;

static bool exitActive;
static int exitWorldX;
static int exitWorldY;
static int exitBounceFrame = -1;

static bool showBasicSpellUI = true;
static bool showSpecialSpellUI = true;
static bool showDashUI = true;

static PopupType popupType;
static PopupPosition popupPosition;
static int popupFrame;
static float popupCurFrameTime;
static bool popupActive;

typedef struct UIAnimation {
    LCDBitmapTable *bitmapTable;
    int drawX;
    int drawY;
    int currentFrame;
    int frameCount;
    float frameTime;
    float currentFrameTime;
} UIAnimation;
UIAnimation uiAnimations[MAX_UI_ANIMATIONS];

void initUI(void) {
    screenShakeTime = 0;
    shakeMagnitude = 0;
    healthFlashTimer = 0;
    pd->display->setOffset(0, 0);
    pd->graphics->setBackgroundColor(kColorBlack);

    exitActive = false;

    showBasicSpellUI = true;
    showSpecialSpellUI = true;
    showDashUI = true;

    for (int i = 0; i < MAX_DIRECTION_INDICATORS; i++) {
        directionIndicators[i].timer = 0;
    }

    popupActive = false;

    for (int i = 0; i < MAX_UI_ANIMATIONS; i++) {
        uiAnimations[i] = (UIAnimation) {
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

static void addDirectionIndicator(float sourceX, float sourceY, IndicatorType type, float duration) {
    // Find an empty slot or the oldest indicator
    int bestIndex = 0;
    float lowestTimer = directionIndicators[0].timer;
    for (int i = 0; i < MAX_DIRECTION_INDICATORS; i++) {
        if (directionIndicators[i].timer <= 0) {
            bestIndex = i;
            break;
        }
        if (directionIndicators[i].timer < lowestTimer) {
            lowestTimer = directionIndicators[i].timer;
            bestIndex = i;
        }
    }
    directionIndicators[bestIndex].sourceX = sourceX;
    directionIndicators[bestIndex].sourceY = sourceY;
    directionIndicators[bestIndex].timer = duration;
    directionIndicators[bestIndex].totalDuration = duration;
    directionIndicators[bestIndex].type = type;
}

void addDamageIndicator(float sourceX, float sourceY) {
    addDirectionIndicator(sourceX, sourceY, INDICATOR_TYPE_DAMAGE, DAMAGE_INDICATOR_DURATION);
}

void addAttackIntentIndicator(float sourceX, float sourceY, float duration) {
    // Changed my mind on the attack intent indicator duration, it looks better if it doesn't linger too long for long prep times
    duration = ATTACK_INTENT_MIN_DURATION;
    addDirectionIndicator(sourceX, sourceY, INDICATOR_TYPE_ATTACK_INTENT, duration);
}

// Returns false if the indicator should be skipped this frame (flash off-cycle).
// Otherwise writes a 0..1 scale factor into outFade.
static bool computeIndicatorFade(DirectionIndicator *ind, float *outFade) {
    float elapsed = ind->totalDuration - ind->timer;
    if (ind->type == INDICATOR_TYPE_DAMAGE) {
        if (elapsed < DAMAGE_INDICATOR_FLASH_TIME) {
            float cycleTime = DAMAGE_INDICATOR_FLASH_TIME / 3.0f;
            if (fmodf(elapsed, cycleTime) >= cycleTime / 2.0f) {
                return false;
            }
            *outFade = 1.0f;
        } else if (elapsed < DAMAGE_INDICATOR_FLASH_TIME + DAMAGE_INDICATOR_HOLD_TIME) {
            *outFade = 1.0f;
        } else {
            float fadeElapsed = elapsed - DAMAGE_INDICATOR_FLASH_TIME - DAMAGE_INDICATOR_HOLD_TIME;
            *outFade = 1.0f - fadeElapsed / DAMAGE_INDICATOR_FADE_TIME;
        }
    } else {
        float fadeOutStart = ind->totalDuration - ATTACK_INTENT_FADE_OUT_TIME;
        if (elapsed < ATTACK_INTENT_FADE_IN_TIME) {
            *outFade = elapsed / ATTACK_INTENT_FADE_IN_TIME;
        } else if (elapsed < ATTACK_INTENT_FADE_IN_TIME + ATTACK_INTENT_FLASH_TIME) {
            float flashElapsed = elapsed - ATTACK_INTENT_FADE_IN_TIME;
            float cycleTime = ATTACK_INTENT_FLASH_TIME / 3.0f;
            if (fmodf(flashElapsed, cycleTime) >= cycleTime / 2.0f) {
                return false;
            }
            *outFade = 1.0f;
        } else if (elapsed < fadeOutStart) {
            *outFade = 1.0f;
        } else {
            *outFade = (ind->totalDuration - elapsed) / ATTACK_INTENT_FADE_OUT_TIME;
        }
    }
    return true;
}

static void drawDirectionIndicators(float deltaTime) {
    LCDPattern diagonalPattern = {
        0xB6, 0x6D, 0xDB, 0xB6, 0x6D, 0xDB, 0xB6, 0x6D,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };

    for (int i = 0; i < MAX_DIRECTION_INDICATORS; i++) {
        DirectionIndicator *ind = &directionIndicators[i];
        if (ind->timer <= 0) {
            continue;
        }
        ind->timer -= deltaTime;
        if (ind->timer <= 0) {
            ind->timer = 0;
            continue;
        }

        float fade;
        if (!computeIndicatorFade(ind, &fade)) {
            continue;
        }
        if (fade <= 0) {
            continue;
        }

        LCDColor fillColor;
        if (ind->type == INDICATOR_TYPE_ATTACK_INTENT) {
            fillColor = (LCDColor)kColorWhite;
        } else {
            fillColor = (LCDColor)diagonalPattern;
        }

        // Recompute angle each frame so it tracks player turning
        float angle = atan2f(ind->sourceY - playerY, ind->sourceX - playerX) - playerAngle;
        while (angle > PI) {
            angle -= 2 * PI;
        }
        while (angle < -PI) {
            angle += 2 * PI;
        }

        // Draw arc around screen center pointing toward source
        float screenAngle = angle - PI / 2;

        float cx = SCREEN_WIDTH / 2.0f;
        float cy = SCREEN_HEIGHT / 2.0f;
        float arcSpan = INDICATOR_ARC_SPAN * fade;
        float radius = ind->type == INDICATOR_TYPE_ATTACK_INTENT ? ATTACK_INTENT_INDICATOR_RADIUS : DAMAGE_INDICATOR_RADIUS;
        float innerR = radius - INDICATOR_THICKNESS / 2.0f;
        float outerR = radius + INDICATOR_THICKNESS / 2.0f;
        float step = arcSpan / INDICATOR_ARC_SEGMENTS;

        // Black outline: expanded arc behind the filled one
        bool drawArc = ind->type == INDICATOR_TYPE_DAMAGE || ind->type == INDICATOR_TYPE_ATTACK_INTENT;
        float outInnerR = innerR - INDICATOR_OUTLINE;
        float outOuterR = outerR + INDICATOR_OUTLINE;
        float outArcSpan = arcSpan + step * 3;
        float outStep = outArcSpan / (INDICATOR_ARC_SEGMENTS + 3);
        if (drawArc) {
            for (int s = 0; s < INDICATOR_ARC_SEGMENTS + 3; s++) {
                float a0 = screenAngle - outArcSpan / 2.0f + s * outStep;
                float a1 = a0 + outStep;
                int ox0 = (int)(cx + cosf(a0) * outInnerR);
                int oy0 = (int)(cy + sinf(a0) * outInnerR);
                int ox1 = (int)(cx + cosf(a1) * outInnerR);
                int oy1 = (int)(cy + sinf(a1) * outInnerR);
                int ox2 = (int)(cx + cosf(a0) * outOuterR);
                int oy2 = (int)(cy + sinf(a0) * outOuterR);
                int ox3 = (int)(cx + cosf(a1) * outOuterR);
                int oy3 = (int)(cy + sinf(a1) * outOuterR);
                pd->graphics->fillTriangle(ox0, oy0, ox1, oy1, ox2, oy2, kColorBlack);
                pd->graphics->fillTriangle(ox1, oy1, ox2, oy2, ox3, oy3, kColorBlack);
            }
        }

        // Arrow geometry
        float arrowLength = INDICATOR_ARROW_LENGTH * fade;
        float arrowSpread = INDICATOR_ARROW_SPREAD * fade;
        bool drawArrow = ind->type == INDICATOR_TYPE_ATTACK_INTENT && arrowLength > 1 && arrowSpread > 1;
        float dirX = cosf(screenAngle);
        float dirY = sinf(screenAngle);
        float perpX = -dirY;
        float perpY = dirX;

        // Black outline arrow (drawn before filled arc so it layers correctly)
        // Offset vertices along edge normals so outline thickness matches the arc
        if (drawArrow) {
            float sideLen = sqrtf((arrowLength + 1) * (arrowLength + 1) + arrowSpread * arrowSpread);
            float tipOutline = INDICATOR_OUTLINE * sideLen / arrowSpread;
            float baseSpreadOutline = INDICATOR_OUTLINE * (sideLen + arrowSpread) / (arrowLength + 1);
            int outTipX = (int)(cx + dirX * (outerR + arrowLength + tipOutline));
            int outTipY = (int)(cy + dirY * (outerR + arrowLength + tipOutline));
            int outBx1 = (int)(cx + dirX * (outerR - INDICATOR_OUTLINE - 1) + perpX * (arrowSpread + baseSpreadOutline));
            int outBy1 = (int)(cy + dirY * (outerR - INDICATOR_OUTLINE - 1) + perpY * (arrowSpread + baseSpreadOutline));
            int outBx2 = (int)(cx + dirX * (outerR - INDICATOR_OUTLINE - 1) - perpX * (arrowSpread + baseSpreadOutline));
            int outBy2 = (int)(cy + dirY * (outerR - INDICATOR_OUTLINE - 1) - perpY * (arrowSpread + baseSpreadOutline));
            pd->graphics->fillTriangle(outTipX, outTipY, outBx1, outBy1, outBx2, outBy2, kColorBlack);
        }

        // Filled arc on top of outlines
        if (drawArc) {
            for (int s = 0; s < INDICATOR_ARC_SEGMENTS; s++) {
                float a0 = screenAngle - arcSpan / 2.0f + s * step;
                float a1 = a0 + step;
                int ix0 = (int)(cx + cosf(a0) * innerR);
                int iy0 = (int)(cy + sinf(a0) * innerR);
                int ix1 = (int)(cx + cosf(a1) * innerR);
                int iy1 = (int)(cy + sinf(a1) * innerR);
                int ix2 = (int)(cx + cosf(a0) * outerR);
                int iy2 = (int)(cy + sinf(a0) * outerR);
                int ix3 = (int)(cx + cosf(a1) * outerR);
                int iy3 = (int)(cy + sinf(a1) * outerR);
                pd->graphics->fillTriangle(ix0, iy0, ix1, iy1, ix2, iy2, fillColor);
                pd->graphics->fillTriangle(ix1, iy1, ix2, iy2, ix3, iy3, fillColor);
            }
        }

        // Filled arrow on top of outlines
        if (drawArrow) {
            int tipX = (int)(cx + dirX * (outerR + arrowLength));
            int tipY = (int)(cy + dirY * (outerR + arrowLength));
            int bx1 = (int)(cx + dirX * (outerR - 1) + perpX * arrowSpread);
            int by1 = (int)(cy + dirY * (outerR - 1) + perpY * arrowSpread);
            int bx2 = (int)(cx + dirX * (outerR - 1) - perpX * arrowSpread);
            int by2 = (int)(cy + dirY * (outerR - 1) - perpY * arrowSpread);
            pd->graphics->fillTriangle(tipX, tipY, bx1, by1, bx2, by2, fillColor);
        }
    }
}

void flashHealthUI(void) {
    healthFlashTimer = HEALTH_FLASH_TIME;
}

void drawHealth(float deltaTime) {
    if (healthFlashTimer > 0) {
        healthFlashTimer -= deltaTime;
        if (healthFlashTimer < 0) {
            healthFlashTimer = 0;
        }
    }
    bool flashing = healthFlashTimer > 0;

    int healthBarWidth = 64;
    int healthBarHeight = 4;
    int healthBarX = 1;
    int healthBarY = 1;
    pd->graphics->pushContext(NULL);
        if (flashing) {
            pd->graphics->setDrawMode(kDrawModeFillWhite);
        }
        pd->graphics->drawBitmap(BITMAPS[BITMAP_HEALTH_BAR], healthBarX, healthBarY, kBitmapUnflipped);
        pd->graphics->fillRoundRect(13 + healthBarX, 5 + healthBarY, healthBarWidth * (playerHealth / getPlayerMaxHealth()), healthBarHeight, 2, kColorWhite);
        char *healthString;
        pd->system->formatString(&healthString, "%d", (int)playerHealth);
        pd->graphics->setFont(fontSubEN);
        int tracking = -1;
        pd->graphics->setTextTracking(tracking);
        int textWidth = pd->graphics->getTextWidth(fontSubEN, healthString, strlen(healthString), kUTF8Encoding, tracking);
        pd->graphics->drawText(healthString, strlen(healthString), kUTF8Encoding, healthBarY + 43 - textWidth / 2, healthBarY + 3);
        pd->system->realloc(healthString, 0);
    pd->graphics->popContext();
}

void drawEntityToMinimap(float entityX, float entityY, uint8_t *texture, float playerAngleCos, float playerAngleSin, uint8_t *frameBuffer) {
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
                if (isOpaque(adjustedMapX, adjustedMapY)) {
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
    uint8_t *entityFrameBuffer = pd->graphics->getFrame();
    float playerAngleCos = cosf(rotatedPlayerAngle + PI);
    float playerAngleSin = sinf(rotatedPlayerAngle + PI);
    // Draw enemies
    for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
        Enemy curEnemy = enemies[i];
        if (!curEnemy.active || curEnemy.state == ENEMY_STATE_DEAD) {
            continue;
        }

        drawEntityToMinimap(curEnemy.x, curEnemy.y, TEXTURES[TEXTURE_MINIMAP_ENEMY], playerAngleCos, playerAngleSin, entityFrameBuffer);
    }
    
    // Draw interactables
    for (int i = 0; i < MAX_INTERACTABLE_COUNT; i++) {
        Interactable curInteractable = INTERACTABLES[i];
        if (!curInteractable.active) {
            continue;
        }

        if (curInteractable.type == INTERACTABLE_TYPE_EXIT) {
            drawEntityToMinimap(curInteractable.x, curInteractable.y, TEXTURES[TEXTURE_MINIMAP_EXIT], playerAngleCos, playerAngleSin, entityFrameBuffer);
        } else if (curInteractable.type == INTERACTABLE_TYPE_CAMPFIRE || curInteractable.type == INTERACTABLE_TYPE_EXTINGUISHED_CAMPFIRE) {
            drawEntityToMinimap(curInteractable.x, curInteractable.y, TEXTURES[TEXTURE_MINIMAP_CAMPFIRE], playerAngleCos, playerAngleSin, entityFrameBuffer);
        }
    }
    // Draw projectiles
    for (int i = 0; i < MAX_PROJECTILE_COUNT; i++) {
        if (!projectiles[i].active || !projectiles[i].isEnemy) {
            continue;
        }
        Projectile curProjectile = projectiles[i];

        drawEntityToMinimap(curProjectile.x, curProjectile.y, TEXTURES[TEXTURE_MINIMAP_PROJECTILE], playerAngleCos, playerAngleSin, entityFrameBuffer);
    }
    // Draw warnings
    for (int i = 0; i < MAX_PARTICLE_COUNT; i++) {
        if (!particles[i].active || particles[i].type != PARTICLE_TYPE_WARNING) {
            continue;
        }
        Particle curParticle = particles[i];

        drawEntityToMinimap(curParticle.x, curParticle.y, TEXTURES[TEXTURE_MINIMAP_WARNING], playerAngleCos, playerAngleSin, entityFrameBuffer);
    }
    // Draw pickups
    for (int i = 0; i < MAX_PICKUP_COUNT; i++) {
        Pickup curPickup = pickups[i];
        if (!curPickup.active) {
            continue;
        }

        drawEntityToMinimap(curPickup.x, curPickup.y, TEXTURES[TEXTURE_MINIMAP_CARD], playerAngleCos, playerAngleSin, entityFrameBuffer);
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
    }
}

void setArrowPosition(int worldX, int worldY) {
    exitActive = true;
    exitWorldX = worldX;
    exitWorldY = worldY;
    exitBounceFrame = 0;
}

void clearArrow(void) {
    exitActive = false;
}

void setSpellUIVisibility(bool basic, bool special, bool dash) {
    showBasicSpellUI = basic;
    showSpecialSpellUI = special;
    showDashUI = dash;
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

    int bounceOffset = 0;
    if (exitBounceFrame >= 0) {
        float hopFraction = (float)(exitBounceFrame % EXIT_ARROW_BOUNCE_HOP_FRAMES) / EXIT_ARROW_BOUNCE_HOP_FRAMES;
        bounceOffset = -(int)(sinf(hopFraction * PI) * EXIT_ARROW_BOUNCE_HEIGHT + 0.5f);
        exitBounceFrame++;
        if (exitBounceFrame >= EXIT_ARROW_BOUNCE_FRAMES) {
            exitBounceFrame = -1;
        }
    }

    drawCenteredBitmap(pd->graphics->getTableBitmap(BITMAP_TABLES[BITMAP_TABLE_EXIT_ARROW], rotationTableIndex), EXIT_ARROW_X, EXIT_ARROW_Y + bounceOffset);
}

void drawSpellUI(void) {
    static const int SPELL_SLOT_LEFT_X    = 3;
    static const int SPELL_SLOT_RIGHT_X   = 34;
    static const int SPELL_SLOT_Y         = 82;
    static const int SPELL_ICON_SIZE      = 24;
    static const int SPELL_COOLDOWN_INSET = 2;

    static const int BUTTON_ICON_LEFT_X  = 19;
    static const int BUTTON_ICON_RIGHT_X = 50;
    static const int BUTTON_ICON_Y       = 77;

    int basicSlotX   = SETTINGS.spellButtonsReversed ? SPELL_SLOT_LEFT_X  : SPELL_SLOT_RIGHT_X;
    int specialSlotX = SETTINGS.spellButtonsReversed ? SPELL_SLOT_RIGHT_X : SPELL_SLOT_LEFT_X;
    int spellX[SPELL_EQUIP_MAX] = { basicSlotX, specialSlotX };
    bool spellVisible[SPELL_EQUIP_MAX] = { showBasicSpellUI, showSpecialSpellUI };

    int cooldownInner = SPELL_ICON_SIZE - 2 * SPELL_COOLDOWN_INSET;

    for (int i = 0; i < SPELL_EQUIP_MAX; i++) {
        if (!spellVisible[i]) {
            continue;
        }
        LCDBitmap *icon = getSpellIcon(getDeckSpell(i));
        pd->graphics->drawBitmap(icon, spellX[i], SPELL_SLOT_Y, kBitmapUnflipped);

        float cooldownPercentage = getSpellCooldownPercentage(i);
        if (cooldownPercentage > 0.0f) {
            int cooldownCoverHeight = (int)(cooldownPercentage * cooldownInner);
            pd->graphics->fillRect(spellX[i] + SPELL_COOLDOWN_INSET, SPELL_SLOT_Y + SPELL_COOLDOWN_INSET, cooldownInner, cooldownCoverHeight, kColorBlack);
        }
    }

    // The left slot is always cast with B and the right with A; reveal each button
    // icon only when the spell occupying that slot is visible (respects the swap setting).
    bool leftIsBasic = (basicSlotX == SPELL_SLOT_LEFT_X);
    bool showLeftButton  = leftIsBasic ? showBasicSpellUI : showSpecialSpellUI;
    bool showRightButton = leftIsBasic ? showSpecialSpellUI : showBasicSpellUI;
    if (showLeftButton) {
        pd->graphics->drawBitmap(BITMAPS[BITMAP_SPELL_B_BUTTON_ICON], BUTTON_ICON_LEFT_X,  BUTTON_ICON_Y, kBitmapUnflipped);
    }
    if (showRightButton) {
        pd->graphics->drawBitmap(BITMAPS[BITMAP_SPELL_A_BUTTON_ICON], BUTTON_ICON_RIGHT_X, BUTTON_ICON_Y, kBitmapUnflipped);
    }

    if (!showDashUI) {
        return;
    }

    static const int UTILITY_BASE_X       = 3;
    static const int UTILITY_BASE_Y       = 108;
    static const int UTILITY_GAP          = 13;
    static const int UTILITY_CHARGE_INSET = 2;
    static const int UTILITY_CHARGE_SIZE  = 7;

    LCDPattern chargePattern = {
        0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };

    int filledCount = getUtilitySpellCount();
    float charge = getUtilitySpellCharge();
    int maxUtility = getMaxUtilitySpells();

    for (int i = 0; i < maxUtility; i++) {
        int x = UTILITY_BASE_X + i * UTILITY_GAP;
        int y = UTILITY_BASE_Y;

        if (i < filledCount) {
            pd->graphics->drawBitmap(BITMAPS[BITMAP_DASH_INDICATOR_FULL], x, y, kBitmapUnflipped);
        } else {
            pd->graphics->drawBitmap(BITMAPS[BITMAP_DASH_INDICATOR_EMPTY], x, y, kBitmapUnflipped);
            if (i == filledCount && charge > 0.0f) {
                int fillHeight = (int)(charge * UTILITY_CHARGE_SIZE);
                if (fillHeight > 0) {
                    int drawX = x + UTILITY_CHARGE_INSET;
                    int drawY = y + UTILITY_CHARGE_INSET + (UTILITY_CHARGE_SIZE - fillHeight);
                    pd->graphics->fillRect(drawX, drawY, UTILITY_CHARGE_SIZE, fillHeight, (LCDColor)chargePattern);
                }
            }
        }
    }
}

void drawBossHealthBar(void) {
    float healthPercentage;
    if (isBossDeathCinematicActive()) {
        healthPercentage = getBossHealthPercentage();
    } else if (isBossCinematicActive()) {
        healthPercentage = getBossCinematicProgress();
    } else {
        healthPercentage = getBossHealthPercentage();
    }
    int healthBarWidth = healthPercentage * 58;

    int healthBarX = 68;
    int healthBarY = 29;
    pd->graphics->drawBitmap(BITMAPS[BITMAP_BOSS_HEALTH_BAR], healthBarX, healthBarY, kBitmapUnflipped);
    pd->graphics->fillRect(healthBarX + 3, healthBarY + 3, healthBarWidth, 4, kColorWhite);
    
    int titleDrawX = 100;
    int titleDrawY = 21;
    BossTitle title;
    switch (getBossEnemyType()) {
        case ENEMY_TYPE_WRAITH_BOSS:
            title = BOSS_TITLE_WRAITH;
            break;
        case ENEMY_TYPE_DRAGON_BOSS:
            title = BOSS_TITLE_DRAGON;
            break;
        case ENEMY_TYPE_ELDER_BOSS:
            title = BOSS_TITLE_ELDER;
            break;
        default:
            return;
    }
    drawCenteredBitmap(getBossTitleBitmap(title), titleDrawX, titleDrawY);
}

void addUIAnimation(int bitmapTableIndex, float frameTime, int drawX, int drawY) {
    int index = -1;
    for (int i = 0; i < MAX_UI_ANIMATIONS; i++) {
        if (uiAnimations[i].bitmapTable == NULL) {
            index = i;
            break;
        }
    }
    if (index == -1) {
        return;
    }

    LCDBitmapTable *bitmapTable = BITMAP_TABLES[bitmapTableIndex];
    int frameCount;
    pd->graphics->getBitmapTableInfo(bitmapTable, NULL, &frameCount);

    uiAnimations[index] = (UIAnimation) {
        .bitmapTable = bitmapTable,
        .drawX = drawX,
        .drawY = drawY,
        .currentFrame = 0,
        .frameCount = frameCount,
        .frameTime = frameTime,
        .currentFrameTime = 0.0f
    };
}

void removeUIAnimation(int bitmapTableIndex) {
    for (int i = 0; i < MAX_UI_ANIMATIONS; i++) {
        if (uiAnimations[i].bitmapTable == BITMAP_TABLES[bitmapTableIndex]) {
            uiAnimations[i].bitmapTable = NULL;
            break;
        }
    }
}

static void drawUIAnimations(float deltaTime) {
    for (int i = 0; i < MAX_UI_ANIMATIONS; i++) {
        UIAnimation *overlay = &uiAnimations[i];
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

void showPopup(PopupType type, PopupPosition position) {
    popupType = type;
    popupPosition = position;
    popupFrame = 0;
    popupCurFrameTime = 0;
    popupActive = true;
}

bool isPopupActive(void) {
    return popupActive;
}

void drawPopup(float deltaTime) {
    if (popupActive) {
        popupCurFrameTime += deltaTime;
        if (popupCurFrameTime >= POPUP_FRAME_TIME) {
            popupCurFrameTime = 0;
            popupFrame++;
            if (popupFrame >= POPUP_MAX_FRAME) {
                popupActive = false;
                return;
            }
        }

        LCDBitmap *popupBitmap = getPopupBitmap(popupType, popupFrame);
        int popupY;
        switch (popupPosition) {
            case POPUP_POSITION_TOP:
                popupY = 20;
                break;
            case POPUP_POSITION_BOTTOM:
                popupY = 100;
                break;
            case POPUP_POSITION_CENTER:
            default:
                popupY = 60;
                break;
        }
        drawCenteredBitmap(popupBitmap, 100, popupY);
    }
}

void drawRoomInfo(void) {
    if (isTutorialActive()) {
        StringKey objectiveKey;
        int objectiveCurrent, objectiveTarget;
        if (!getTutorialObjectiveText(&objectiveKey, &objectiveCurrent, &objectiveTarget)) {
            return;
        }
        pd->graphics->pushContext(NULL);
            bool isJP = SETTINGS.language == LANGUAGE_JP;
            pd->graphics->setFont(isJP ? fontSubJPDoubleOutline : fontSubENDoubleOutline);
            pd->graphics->setTextTracking(isJP ? -1 : -3);
            char *objectiveString;
            pd->system->formatString(&objectiveString, getLocString(objectiveKey), objectiveCurrent, objectiveTarget);
            pd->graphics->drawText(objectiveString, strlen(objectiveString), kUTF8Encoding, 6, 16);
            pd->system->realloc(objectiveString, 0);
        pd->graphics->popContext();
        return;
    }

    int roomIndex = getCurrentRoomIndex();
    int totalRooms = getDungeonRoomCount();
    if (isAreaCompleted() || roomIndex >= totalRooms - 1) {
        return;
    }

    pd->graphics->pushContext(NULL);
        bool isJP = SETTINGS.language == LANGUAGE_JP;
        pd->graphics->setFont(isJP ? fontSubJPDoubleOutline : fontSubENDoubleOutline);
        pd->graphics->setTextTracking(isJP ? -1 : -3);

        char *roomString;
        if (roomIndex >= totalRooms - 1) {
            pd->system->formatString(&roomString, getLocString(STR_ROOM_BOSS));
        } else {
            pd->system->formatString(&roomString, getLocString(STR_ROOM_FORMAT), roomIndex + 1, totalRooms - 1);
        }
        pd->graphics->drawText(roomString, strlen(roomString), kUTF8Encoding, 6, 16);
        pd->system->realloc(roomString, 0);
    pd->graphics->popContext();
}

void drawExperienceBar(void) {
    float experiencePercent = getExperiencePercentToNextLevel();
    int expBarWidth = 200;
    int expBarHeight = 3;
    pd->graphics->drawBitmap(BITMAPS[BITMAP_EXPERIENCE_BAR], 0, 115, kBitmapUnflipped);
    pd->graphics->fillRect(0, 117, expBarWidth * experiencePercent, expBarHeight, kColorWhite);
}

void updateScreenShake(float deltaTime) {
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
}

void drawUI(float deltaTime, bool duringCinematic) {
    updateScreenShake(deltaTime);

    if (getBossActive()) {
        drawBossHealthBar();
    }

    if (duringCinematic) {
        return;
    }

    drawHealth(deltaTime);
    drawRuneHUD();
    drawUIAnimations(deltaTime);
    drawWeapon(deltaTime);
    drawSpellUI();
    // drawMinimap();
    drawExitArrow();
    drawInteractablePrompt();
    drawRoomInfo();
    // drawExperienceBar();
    drawDirectionIndicators(deltaTime);
    drawPopup(deltaTime);
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

void screenShakeCustom(float duration, float magnitude) {
    screenShakeTime = duration;
    shakeMagnitude = (int)magnitude;
}