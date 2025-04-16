#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "helpers.h"
#include "game/enemies/enemies.h"
#include "game/rendering/drawables.h"
#include "game/player/player.h"
#include "game/rendering/particles.h"
#include "game/rendering/textures.h"

#define MAX_DRAWABLE_COUNT 20
#define DRAWABLE_TEXTURE_SIZE 32

static int queuedDrawables = 0;

typedef struct Drawable {
    float x;
    float y;
    uint32_t type;
    uint32_t index;
    uint8_t *texture;
} Drawable;

Drawable drawables[MAX_DRAWABLE_COUNT] = { 0 };

typedef struct DrawablePair {
    int order;
    float dist;
} DrawablePair;

int drawablePairComparison(const void *a, const void *b) {
    return ((DrawablePair *)b)->dist - ((DrawablePair *)a)->dist;
}

void sortDrawables(int *order, float *dist, int count) {
    DrawablePair drawablePairs[count];
    for (int i = 0; i < count; i++) {
        drawablePairs[i].order = order[i];
        drawablePairs[i].dist = dist[i];
    }

    qsort(drawablePairs, count, sizeof(DrawablePair), drawablePairComparison);

    for (int i = 0; i < count; i++) {
        order[i] = drawablePairs[i].order;
        dist[i] = drawablePairs[i].dist;
    }
}

void initDrawables(void) {
    queuedDrawables = 0;
}

void renderDrawables(float *zBuffer) {
    if (queuedDrawables <= 0) {
        return;
    }

    uint8_t *frameBufferU8 = (uint8_t *)pd->graphics->getFrame();
    int drawableOrder[queuedDrawables];
    float drawableDistances[queuedDrawables];
    for (int i = 0; i < queuedDrawables; i++) {
        drawableOrder[i] = i;
        Drawable curDrawable = drawables[i];
        float drawableDistance = distanceSquared(playerX, playerY, curDrawable.x, curDrawable.y);
        drawableDistances[i] = drawableDistance;
    }
    sortDrawables(drawableOrder, drawableDistances, queuedDrawables);

    //transform drawable with the inverse camera matrix
    // [ planeX   dirX ] -1                                       [ dirY      -dirX ]
    // [               ]       =  1/(planeX*dirY-dirX*planeY) *   [                 ]
    // [ planeY   dirY ]                                          [ -planeY  planeX ]
    float inverseDeterminant = 1.0f / (planeX * dirY - dirX * planeY);
    int fixedPointScreenHeight = SCREEN_HEIGHT << 7;
    for (int i = 0; i < queuedDrawables; i++) {
        Drawable curDrawable = drawables[drawableOrder[i]];
        uint8_t *drawableTexture = curDrawable.texture;
        int anchorTopLeft = 0;
        int hitFlash = 0;
        int frameOffset = 0;
        if (curDrawable.type == PROJECTILE) {
            anchorTopLeft = 1;
        } else if (curDrawable.type == ENEMY) {
            int animationFrame = enemies[curDrawable.index].animationFrame;
            frameOffset = animationFrame * DRAWABLE_TEXTURE_SIZE * DRAWABLE_TEXTURE_SIZE;
            hitFlash = enemies[curDrawable.index].hitFlashTime > 0;
        } else if (curDrawable.type == PARTICLE) {
            int animationFrame = particles[curDrawable.index].animationFrame;
            frameOffset = animationFrame * DRAWABLE_TEXTURE_SIZE * DRAWABLE_TEXTURE_SIZE;
        }

        float drawableX = (curDrawable.x - playerX) / CELL_SIZE;
        float drawableY = (curDrawable.y - playerY) / CELL_SIZE;

        float transformY = inverseDeterminant * (-planeY * drawableX + planeX * drawableY);
        if (transformY > 0) {
            float invTransformY = fastDiv(1, transformY);
            float transformX = inverseDeterminant * (dirX * drawableY - dirY * drawableX);
            float drawableDistance = transformY * CELL_SIZE;
            if (drawableDistance > FOG_MAX_DIST) {
                continue;
            }

            int fogPatternIndex = (drawableDistance - FOG_MIN_DIST) / FOG_STEP;
            int *fogPattern = fogPatterns[fogPatternIndex];

            int drawableHeight = abs((int)((float)SCREEN_HEIGHT * invTransformY));
            int drawableWidth = drawableHeight;

            int halfDrawableWidth = (drawableWidth >> 1);
            int halfDrawableHeight = (drawableHeight >> 1);
            int screenXOffset = 0;//anchorTopLeft ? halfDrawableWidth / 2 : 0;
            int screenYOffset = anchorTopLeft ? halfDrawableHeight / 2 : 0;
            int drawableScreenX = (int)((float)(HALF_SCREEN_WIDTH) * (1 + transformX * invTransformY));

            int drawStartY = -halfDrawableHeight + HALF_SCREEN_HEIGHT;
            int drawEndY = halfDrawableHeight + HALF_SCREEN_HEIGHT;
            if (anchorTopLeft) {
                drawStartY += screenYOffset;
                drawEndY += screenYOffset;
            }
            if (drawStartY < 0) { drawStartY = 0; }
            if (drawEndY >= SCREEN_HEIGHT) { drawEndY = SCREEN_HEIGHT; }
            int fixedPointDrawableHeight = drawableHeight << 7;

            int drawStartX = -halfDrawableWidth + drawableScreenX;
            int drawEndX = halfDrawableWidth + drawableScreenX;
            if (anchorTopLeft) {
                drawStartX += screenXOffset;
                drawEndX += screenXOffset;
            }
            if (drawStartX < 0) { drawStartX = 0; }
            if (drawEndX >= SCREEN_WIDTH) { drawEndX = SCREEN_WIDTH; }

            int textureYCache[SCREEN_HEIGHT] = { [0 ... SCREEN_HEIGHT - 1] = -1 };

            for (int drawX = drawStartX; drawX < drawEndX; drawX++) {
                if (drawableDistance < zBuffer[drawX]) {
                    int fogShadeBit = drawX % 8;
                    int textureX = (int)(((drawX - screenXOffset - (-(drawableWidth >> 1) + drawableScreenX)) << 8) * DRAWABLE_TEXTURE_SIZE / drawableWidth) >> 8;
                    for (int drawY = drawStartY; drawY < drawEndY; drawY++) {

                        int textureYIndex = textureYCache[drawY];
                        if (textureYIndex == -1) {
                            int d = ((drawY - screenYOffset) << 8) - fixedPointScreenHeight + fixedPointDrawableHeight; // 256 and 128 factors to avoid floats
                            int textureY = ((d * DRAWABLE_TEXTURE_SIZE) / drawableHeight) >> 8;
                            textureYIndex = DRAWABLE_TEXTURE_SIZE * textureY + frameOffset;
                            textureYCache[drawY] = textureYIndex;
                        }
                        int fogRow = drawY % 8;
                        int fogShade = drawableDistance < FOG_MIN_DIST ? 0 : fogPattern[fogRow];
                        int inFog = (fogShade >> fogShadeBit) & 1;
                        uint8_t color = drawableTexture[textureYIndex + textureX];
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

    queuedDrawables = 0;
}

void addDrawableToRenderQueue(float x, float y, int type, int index, uint8_t *texture) {
    if (queuedDrawables >= MAX_DRAWABLE_COUNT) {
        return;
    }
    Drawable curDrawable = (struct Drawable){ x, y, type, index, texture };
    drawables[queuedDrawables] = curDrawable;
    queuedDrawables += 1;
}