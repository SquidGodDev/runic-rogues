#define TARGET_EXTENSION 1

#include "helpers.h"
#include "game/entities/entities.h"
#include "game/entities/pickups.h"
#include "game/player/player.h"
#include "game/map/map.h"

#define PICKUP_DISTANCE 50.0f

#define PICKUP_ANIM_FRAME_TIME 0.2f
#define PICKUP_ANIM_FRAMES 8

Pickup pickups[MAX_PICKUP_COUNT];

void initPickups(void) {
    for (int i = 0; i < MAX_PICKUP_COUNT; i++) {
        pickups[i].active = 0;
    }
}

void spawnRandomPickup(void) {
    float x, y;
    getRandomEmptyPosition(&x, &y);
    Spell spell = randInt(1, SPELL_COUNT - 1);
    spawnPickup(x, y, spell);
}

void spawnPickup(float x, float y, Spell spell) {
    for (int i = 0; i < MAX_PICKUP_COUNT; i++) {
        if (!pickups[i].active) {
            pickups[i] = (Pickup) {
                .x = x,
                .y = y,
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

        if (distanceSquared(pickups[i].x, pickups[i].y, playerX, playerY) <= PICKUP_DISTANCE) {
            pickups[i].active = 0; // Pickup collected
            rechargeUtilitySpell();
            setHeldSpell(pickups[i].spell);
            spawnRandomPickup();
        }

        addEntityToRenderQueue(pickups[i].x, pickups[i].y, PICKUP, i, pickups[i].texture, pickups[i].animationFrame);
    }
}