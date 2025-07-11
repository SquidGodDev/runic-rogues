#define TARGET_EXTENSION 1

#include "helpers.h"
#include "game/entities/entities.h"
#include "game/entities/pickups.h"
#include "game/player/player.h"
#include "game/map/map.h"

#define PICKUP_DISTANCE 80.0f

#define PICKUP_ANIM_FRAME_TIME 0.2f
#define PICKUP_ANIM_FRAMES 8

Pickup pickups[MAX_PICKUP_COUNT];

void initPickups(void) {
    for (int i = 0; i < MAX_PICKUP_COUNT; i++) {
        pickups[i].active = 0;
    }
}

int otherPickupAtPosition(int mapX, int mapY) {
    for (int i = 0; i < MAX_PICKUP_COUNT; i++) {
        if (pickups[i].active) {
            if (pickups[i].mapX == mapX && pickups[i].mapY == mapY) {
                return 1;
            }
        }
    }
    return 0;
}

void spawnRandomPickup(void) {
    int mapX, mapY;
    do {
        getRandomEmptyMapPosition(&mapX, &mapY);
    } while (otherPickupAtPosition(mapX, mapY));

    Spell spell = randInt(1, SPELL_COUNT - 1);
    // Spell spell = SPELL_ROOT_ATTACK;
    spawnPickup(mapX, mapY, spell);
}

void spawnPickup(int mapX, int mapY, Spell spell) {
    float x = mapX * CELL_SIZE + CELL_SIZE / 2;
    float y = mapY * CELL_SIZE + CELL_SIZE / 2;
    for (int i = 0; i < MAX_PICKUP_COUNT; i++) {
        if (!pickups[i].active) {
            pickups[i] = (Pickup) {
                .x = x,
                .y = y,
                .mapX = mapX,
                .mapY = mapY,
                .active = 1,
                .animationFrame = 0,
                .animationTime = PICKUP_ANIM_FRAME_TIME,
                .texture = getSpellPickupTexture(spell),
                .spell = spell
            };
            return;
        }
    }
}

void updatePickups(float deltaTime) {
    for (int i = 0; i < MAX_PICKUP_COUNT; i++) {
        if (!pickups[i].active) {
            continue;
        }

        pickups[i].animationTime -= deltaTime;
        if (pickups[i].animationTime <= 0) {
            pickups[i].animationTime = PICKUP_ANIM_FRAME_TIME;
            pickups[i].animationFrame++;
            if (pickups[i].animationFrame >= PICKUP_ANIM_FRAMES) {
                pickups[i].animationFrame = 0;
            }
        }

        if (!heldSpellsFull() && distanceSquared(pickups[i].x, pickups[i].y, playerX, playerY) <= PICKUP_DISTANCE) {
            pickups[i].active = 0; // Pickup collected
            rechargeUtilitySpell();
            addHeldSpell(pickups[i].spell);
            spawnRandomPickup();
        }

        addEntityToRenderQueue(pickups[i].x, pickups[i].y, PICKUP, i, pickups[i].texture, pickups[i].animationFrame);
    }
}