#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/fixedPoint.h"
#include "globals/helpers.h"
#include "game/enemies/enemies.h"
#include "game/entities/entities.h"
#include "game/player/player.h"
#include "game/rendering/particles.h"
#include "game/rendering/textures.h"

#define MAX_ENTITY_COUNT 128
#define ENTITY_TEXTURE_SIZE 32

static int queuedEntities = 0;

typedef struct Entity {
    fixed_t x;
    fixed_t y;
    uint32_t type;
    uint32_t index;
    uint8_t *texture;
    int textureMinX;
    int textureMaxX;
    int textureMinY;
    int textureMaxY;
    int animationFrame;
} Entity;

Entity entities[MAX_ENTITY_COUNT] = { 0 };

typedef struct EntityPair {
    int order;
    fixed_t dist;
} EntityPair;

int entityPairComparison(const void *a, const void *b) {
    fixed_t diff = ((EntityPair *)b)->dist - ((EntityPair *)a)->dist;
    return (diff > 0) - (diff < 0);
}

void sortEntities(int *order, fixed_t *dist, int count) {
    EntityPair entityPairs[count];
    for (int i = 0; i < count; i++) {
        entityPairs[i].order = order[i];
        entityPairs[i].dist = dist[i];
    }

    qsort(entityPairs, count, sizeof(EntityPair), entityPairComparison);

    for (int i = 0; i < count; i++) {
        order[i] = entityPairs[i].order;
        dist[i] = entityPairs[i].dist;
    }
}

void initEntities(void) {
    queuedEntities = 0;
}

void renderEntities(int *zBuffer) {
    if (queuedEntities <= 0) {
        return;
    }

    fixed_t playerX_fp = FLOAT_TO_FP(playerX);
    fixed_t playerY_fp = FLOAT_TO_FP(playerY);
    fixed_t planeX_fp  = FLOAT_TO_FP(planeX);
    fixed_t planeY_fp  = FLOAT_TO_FP(planeY);
    fixed_t dirX_fp    = FLOAT_TO_FP(dirX);
    fixed_t dirY_fp    = FLOAT_TO_FP(dirY);

    uint8_t *frameBufferU8 = (uint8_t *)pd->graphics->getFrame();
    int entityOrder[queuedEntities];
    fixed_t entityDistances[queuedEntities];
    for (int i = 0; i < queuedEntities; i++) {
        entityOrder[i] = i;
        Entity *curEntity = &entities[i];
        int64_t dx = (int64_t)(playerX_fp - curEntity->x);
        int64_t dy = (int64_t)(playerY_fp - curEntity->y);
        entityDistances[i] = (fixed_t)((dx * dx + dy * dy) >> FP_SHIFT);
    }
    sortEntities(entityOrder, entityDistances, queuedEntities);

    //transform entity with the inverse camera matrix
    // [ planeX   dirX ] -1                                       [ dirY      -dirX ]
    // [               ]       =  1/(planeX*dirY-dirX*planeY) *   [                 ]
    // [ planeY   dirY ]                                          [ -planeY  planeX ]
    fixed_t determinant = FP_MUL(planeX_fp, dirY_fp) - FP_MUL(dirX_fp, planeY_fp);
    fixed_t inverseDeterminant = FP_DIV(FP_ONE, determinant);
    int fixedPointScreenHeight = SCREEN_HEIGHT << 7;
    for (int i = 0; i < queuedEntities; i++) {
        Entity *curEntity = &entities[entityOrder[i]];
        uint8_t *entityTexture = curEntity->texture;
        int anchorTopLeft = 0;
        int hitFlash = 0;
        int frameOffset = curEntity->animationFrame * ENTITY_TEXTURE_SIZE * ENTITY_TEXTURE_SIZE;
        if (curEntity->type == PROJECTILE) {
            anchorTopLeft = 1;
        } else if (curEntity->type == ENEMY) {
            hitFlash = enemies[curEntity->index].hitFlashTime > 0;
        }

        fixed_t entityX = (curEntity->x - playerX_fp) >> 4;
        fixed_t entityY = (curEntity->y - playerY_fp) >> 4;

        fixed_t transformY = FP_MUL(inverseDeterminant, FP_MUL(planeX_fp, entityY) - FP_MUL(planeY_fp, entityX));
        if (transformY > 0) {
            fixed_t invTransformY = FP_DIV(FP_ONE, transformY);
            fixed_t transformX = FP_MUL(inverseDeterminant, FP_MUL(dirX_fp, entityY) - FP_MUL(dirY_fp, entityX));
            int entityDistance = FP_TO_INT(transformY * CELL_SIZE);
            if (entityDistance > FOG_MAX_DIST) {
                continue;
            }

            int fogPatternIndex = entityDistance < FOG_MIN_DIST ? 0 : (entityDistance - FOG_MIN_DIST) / FOG_STEP;
            uint8_t *fogPattern = fogPatterns[fogPatternIndex];

            int entityHeight = (int)ABS(((int64_t)SCREEN_HEIGHT * invTransformY) >> FP_SHIFT);
            int entityWidth = entityHeight;

            int halfEntityWidth = (entityWidth >> 1);
            int halfEntityHeight = (entityHeight >> 1);
            int screenXOffset = 0;//anchorTopLeft ? halfEntityWidth / 2 : 0;
            int screenYOffset = anchorTopLeft ? halfEntityHeight / 2 : 0;
            int entityScreenX = FP_TO_INT((fixed_t)(HALF_SCREEN_WIDTH) * (FP_ONE + FP_MUL(transformX, invTransformY)));

            int drawStartY = -halfEntityHeight + HALF_SCREEN_HEIGHT;
            int drawEndY = halfEntityHeight + HALF_SCREEN_HEIGHT;
            if (anchorTopLeft) {
                drawStartY += screenYOffset;
                drawEndY += screenYOffset;
            }
            if (drawStartY < 0) { drawStartY = 0; }
            if (drawEndY >= SCREEN_HEIGHT) { drawEndY = SCREEN_HEIGHT; }
            int fixedPointEntityHeight = entityHeight << 7;

            int drawStartX = -halfEntityWidth + entityScreenX;
            int drawEndX = halfEntityWidth + entityScreenX;
            if (anchorTopLeft) {
                drawStartX += screenXOffset;
                drawEndX += screenXOffset;
            }
            if (drawStartX < 0) { drawStartX = 0; }
            if (drawEndX >= SCREEN_WIDTH) { drawEndX = SCREEN_WIDTH; }

            int textureYCache[SCREEN_HEIGHT];

            int textureYStep_fp8 = (ENTITY_TEXTURE_SIZE << 8) / entityHeight;
            int d0 = ((drawStartY - screenYOffset) << 8) - fixedPointScreenHeight + fixedPointEntityHeight;
            int textureYAccum_fp8 = (d0 * ENTITY_TEXTURE_SIZE) / entityHeight;
            for (int drawY = drawStartY; drawY < drawEndY; drawY++, textureYAccum_fp8 += textureYStep_fp8) {
                int textureY = textureYAccum_fp8 >> 8;
                if (textureY < curEntity->textureMinY || textureY > curEntity->textureMaxY) {
                    textureYCache[drawY] = -2;
                } else {
                    textureYCache[drawY] = ENTITY_TEXTURE_SIZE * textureY + frameOffset;
                }
            }

            int textureXStep_fp8 = (ENTITY_TEXTURE_SIZE << 8) / entityWidth;
            int unclampedDrawStartX = -halfEntityWidth + entityScreenX;
            int textureXAccum_fp8 = (drawStartX - unclampedDrawStartX) * textureXStep_fp8;

            for (int drawX = drawStartX; drawX < drawEndX; drawX++, textureXAccum_fp8 += textureXStep_fp8) {
                if (entityDistance < zBuffer[drawX]) {
                    int fogShadeBit = drawX % 8;
                    int textureX = textureXAccum_fp8 >> 8;
                    if (textureX < curEntity->textureMinX || textureX > curEntity->textureMaxX) {
                        continue;
                    }
                    for (int drawY = drawStartY; drawY < drawEndY; drawY++) {
                        int textureYIndex = textureYCache[drawY];
                        if (textureYIndex == -2) {
                            continue;
                        }

                        int fogRow = drawY % 8;
                        int fogShade = entityDistance < FOG_MIN_DIST ? 0 : fogPattern[fogRow];
                        int inFog = (fogShade >> fogShadeBit) & 1;
                        uint8_t color = entityTexture[textureYIndex + textureX];
                        if (!inFog && ((hitFlash && color < 2) || color == 1)) {
                            frameBufferU8[drawY * LCD_ROWSIZE + (drawX >> 3)] |= (1 << (7 - (drawX & 7)));
                        } else if (color == 0) {
                            frameBufferU8[drawY * LCD_ROWSIZE + (drawX >> 3)] &= ~(1 << (7 - (drawX & 7)));
                        }
                    }
                }
            }
        }
    }

    queuedEntities = 0;
}

void addEntityToRenderQueue(float x, float y, int type, int index, int textureIndex, int animationFrame) {
    if (queuedEntities >= MAX_ENTITY_COUNT) {
        return;
    }
    uint8_t *texture = TEXTURES[textureIndex];
    TextureBounds *bounds = TEXTURE_BOUNDS[textureIndex];
    Entity curEntity = (struct Entity){
        .x = FLOAT_TO_FP(x),
        .y = FLOAT_TO_FP(y),
        .type = type,
        .index = index,
        .texture = texture,
        .textureMinX = bounds->minX,
        .textureMaxX = bounds->maxX,
        .textureMinY = bounds->minY,
        .textureMaxY = bounds->maxY,
        .animationFrame = animationFrame
    };
    entities[queuedEntities] = curEntity;
    queuedEntities += 1;
}
