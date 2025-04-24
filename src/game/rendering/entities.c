#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "helpers.h"
#include "game/enemies/enemies.h"
#include "game/rendering/entities.h"
#include "game/player/player.h"
#include "game/rendering/particles.h"
#include "game/rendering/textures.h"

#define MAX_ENTITY_COUNT 20
#define ENTITY_TEXTURE_SIZE 32

static int queuedEntities = 0;

typedef struct Entity {
    float x;
    float y;
    uint32_t type;
    uint32_t index;
    uint8_t *texture;
    int animationFrame;
} Entity;

Entity entities[MAX_ENTITY_COUNT] = { 0 };

typedef struct EntityPair {
    int order;
    float dist;
} EntityPair;

int entityPairComparison(const void *a, const void *b) {
    return ((EntityPair *)b)->dist - ((EntityPair *)a)->dist;
}

void sortEntities(int *order, float *dist, int count) {
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

void renderEntities(float *zBuffer) {
    if (queuedEntities <= 0) {
        return;
    }

    uint8_t *frameBufferU8 = (uint8_t *)pd->graphics->getFrame();
    int entityOrder[queuedEntities];
    float entityDistances[queuedEntities];
    for (int i = 0; i < queuedEntities; i++) {
        entityOrder[i] = i;
        Entity curEntity = entities[i];
        float entityDistance = distanceSquared(playerX, playerY, curEntity.x, curEntity.y);
        entityDistances[i] = entityDistance;
    }
    sortEntities(entityOrder, entityDistances, queuedEntities);

    //transform entity with the inverse camera matrix
    // [ planeX   dirX ] -1                                       [ dirY      -dirX ]
    // [               ]       =  1/(planeX*dirY-dirX*planeY) *   [                 ]
    // [ planeY   dirY ]                                          [ -planeY  planeX ]
    float inverseDeterminant = 1.0f / (planeX * dirY - dirX * planeY);
    int fixedPointScreenHeight = SCREEN_HEIGHT << 7;
    for (int i = 0; i < queuedEntities; i++) {
        Entity curEntity = entities[entityOrder[i]];
        uint8_t *entityTexture = curEntity.texture;
        int anchorTopLeft = 0;
        int hitFlash = 0;
        int frameOffset = curEntity.animationFrame * ENTITY_TEXTURE_SIZE * ENTITY_TEXTURE_SIZE;
        if (curEntity.type == PROJECTILE) {
            anchorTopLeft = 1;
        } else if (curEntity.type == ENEMY) {
            hitFlash = enemies[curEntity.index].hitFlashTime > 0;
        }

        float entityX = (curEntity.x - playerX) / CELL_SIZE;
        float entityY = (curEntity.y - playerY) / CELL_SIZE;

        float transformY = inverseDeterminant * (-planeY * entityX + planeX * entityY);
        if (transformY > 0) {
            float invTransformY = fastDiv(1, transformY);
            float transformX = inverseDeterminant * (dirX * entityY - dirY * entityX);
            float entityDistance = transformY * CELL_SIZE;
            if (entityDistance > FOG_MAX_DIST) {
                continue;
            }

            int fogPatternIndex = (entityDistance - FOG_MIN_DIST) / FOG_STEP;
            int *fogPattern = fogPatterns[fogPatternIndex];

            int entityHeight = abs((int)((float)SCREEN_HEIGHT * invTransformY));
            int entityWidth = entityHeight;

            int halfEntityWidth = (entityWidth >> 1);
            int halfEntityHeight = (entityHeight >> 1);
            int screenXOffset = 0;//anchorTopLeft ? halfEntityWidth / 2 : 0;
            int screenYOffset = anchorTopLeft ? halfEntityHeight / 2 : 0;
            int entityScreenX = (int)((float)(HALF_SCREEN_WIDTH) * (1 + transformX * invTransformY));

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

            int textureYCache[SCREEN_HEIGHT] = { [0 ... SCREEN_HEIGHT - 1] = -1 };

            for (int drawX = drawStartX; drawX < drawEndX; drawX++) {
                if (entityDistance < zBuffer[drawX]) {
                    int fogShadeBit = drawX % 8;
                    int textureX = (int)(((drawX - screenXOffset - (-(entityWidth >> 1) + entityScreenX)) << 8) * ENTITY_TEXTURE_SIZE / entityWidth) >> 8;
                    for (int drawY = drawStartY; drawY < drawEndY; drawY++) {

                        int textureYIndex = textureYCache[drawY];
                        if (textureYIndex == -1) {
                            int d = ((drawY - screenYOffset) << 8) - fixedPointScreenHeight + fixedPointEntityHeight; // 256 and 128 factors to avoid floats
                            int textureY = ((d * ENTITY_TEXTURE_SIZE) / entityHeight) >> 8;
                            textureYIndex = ENTITY_TEXTURE_SIZE * textureY + frameOffset;
                            textureYCache[drawY] = textureYIndex;
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

void addEntityToRenderQueue(float x, float y, int type, int index, uint8_t *texture, int animationFrame) {
    if (queuedEntities >= MAX_ENTITY_COUNT) {
        return;
    }
    Entity curEntity = (struct Entity){ x, y, type, index, texture, animationFrame };
    entities[queuedEntities] = curEntity;
    queuedEntities += 1;
}