#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/helpers.h"
#include "game/entities/entities.h"
#include "game/entities/projectiles.h"
#include "game/entities/turret.h"
#include "game/player/player.h"
#include "game/player/spells/spells.h"
#include "game/enemies/enemies.h"
#include "game/map/map.h"
#include "game/rendering/textures.h"

// Buffer cap for simultaneously placed turrets. Tunable turret stats
// (lifetime, fire rate, damage, projectile) live on spellData[SPELL_TURRET].
#define MAX_TURRET_COUNT 4
#define TURRET_PROJECTILE_OFFSET 0.0f

typedef struct Turret {
    float x;
    float y;
    float lifetime;
    float fireCooldown;
    bool active;
} Turret;

static Turret turrets[MAX_TURRET_COUNT];

void initTurrets(void) {
    for (int i = 0; i < MAX_TURRET_COUNT; i++) {
        turrets[i].active = false;
    }
}

// Marches forward from (startX, startY) along (dirX, dirY) and returns the
// distance at which a wall (or the map edge) is first encountered.
static float forwardWallDistance(float startX, float startY, float dirXVal, float dirYVal, float maxDist) {
    for (float d = 0.0f; d <= maxDist; d += 1.0f) {
        int cellX = (int)(startX + dirXVal * d) / CELL_SIZE;
        int cellY = (int)(startY + dirYVal * d) / CELL_SIZE;
        if (cellX < 0 || cellX >= MAP_WIDTH || cellY < 0 || cellY >= MAP_HEIGHT || isBlocked(cellX, cellY)) {
            return d;
        }
    }
    return maxDist;
}

void spawnTurret(void) {
    float halfCell = CELL_SIZE / 2.0f;
    float wallDist = forwardWallDistance(playerX, playerY, dirX, dirY, CELL_SIZE * 2.0f);

    // Place half a cell ahead, but pull back so the turret stays at least half a
    // cell away from a wall the player is facing into.
    float forwardDist = halfCell;
    if (wallDist - forwardDist < halfCell) {
        forwardDist = wallDist - halfCell;
    }
    if (forwardDist < 0.0f) {
        forwardDist = 0.0f;
    }

    float turretX = playerX + dirX * forwardDist;
    float turretY = playerY + dirY * forwardDist;

    for (int i = 0; i < MAX_TURRET_COUNT; i++) {
        if (!turrets[i].active) {
            turrets[i] = (Turret){
                .x = turretX,
                .y = turretY,
                .lifetime = spellData[SPELL_TURRET].turretData.lifetime,
                .fireCooldown = 0.0f,
                .active = true
            };
            return;
        }
    }
}

static int findNearestEnemy(float x, float y, float maxRange) {
    int nearest = -1;
    float nearestDistSq = maxRange * maxRange;
    for (int i = 0; i < MAX_ENEMY_COUNT; i++) {
        Enemy *enemy = &enemies[i];
        if (!enemy->active || enemy->state == ENEMY_STATE_DEAD || enemy->health <= 0 || enemy->invisible) {
            continue;
        }
        float d = distanceSquared(x, y, enemy->x, enemy->y);
        if (d < nearestDistSq) {
            nearestDistSq = d;
            nearest = i;
        }
    }
    return nearest;
}

static void fireProjectileAtAngle(Turret *turret, SpellData *data, float angle) {
    float deltaX = cosf(angle);
    float deltaY = sinf(angle);
    float spawnX = turret->x + deltaX * TURRET_PROJECTILE_OFFSET;
    float spawnY = turret->y + deltaY * TURRET_PROJECTILE_OFFSET;
    firePlayerProjectile(spawnX, spawnY, deltaX, deltaY, data->damage, data->projectileData);
}

// Fires a full ring of projectiles evenly spaced around the turret.
static void fireRing(Turret *turret, SpellData *data) {
    int count = data->turretData.ringProjectileCount;
    if (count < 1) {
        count = 1;
    }

    for (int i = 0; i < count; i++) {
        fireProjectileAtAngle(turret, data, (PI * 2.0f / count) * i);
    }
}

// Fires a spread centered on the nearest enemy. Returns false when no enemy is
// in range so the caller can keep the turret ready to fire the moment one appears.
static bool fireAtNearest(Turret *turret, SpellData *data) {
    int target = findNearestEnemy(turret->x, turret->y, data->turretData.range);
    if (target == -1) {
        return false;
    }

    Enemy *enemy = &enemies[target];
    float baseAngle = atan2f(enemy->y - turret->y, enemy->x - turret->x);

    int count = data->turretData.targetedProjectileCount;
    if (count < 1) {
        count = 1;
    }
    float spread = data->turretData.targetedSpreadAngle;
    float startAngle = baseAngle - spread * (count - 1) / 2.0f;

    for (int i = 0; i < count; i++) {
        fireProjectileAtAngle(turret, data, startAngle + spread * i);
    }
    return true;
}

void updateTurrets(float deltaTime) {
    SpellData *data = &spellData[SPELL_TURRET];

    for (int i = 0; i < MAX_TURRET_COUNT; i++) {
        Turret *turret = &turrets[i];
        if (!turret->active) {
            continue;
        }

        turret->lifetime -= deltaTime;
        if (turret->lifetime <= 0.0f) {
            turret->active = false;
            continue;
        }

        if (turret->fireCooldown > 0.0f) {
            turret->fireCooldown -= deltaTime;
        }
        if (turret->fireCooldown <= 0.0f) {
            bool fired = true;
            if (data->turretData.fireMode == TURRET_FIRE_NEAREST) {
                fired = fireAtNearest(turret, data);
            } else {
                fireRing(turret, data);
            }
            if (fired) {
                turret->fireCooldown = data->turretData.fireInterval;
            }
        }

        addEntityToRenderQueue(turret->x, turret->y, TURRET, i, TEXTURE_TURRET, 0);
    }
}
