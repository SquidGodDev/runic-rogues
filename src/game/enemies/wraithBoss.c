#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "game/map/map.h"
#include "game/player/player.h"
#include "game/entities/projectiles.h"
#include "game/enemies/enemies.h"
#include "game/enemies/enemyInternal.h"

#define WRAITH_BOSS_PREP_TIME 1.0f
#define WRAITH_BOSS_ATTACK1_TIME 2.5f
#define WRAITH_BOSS_ATTACK2_TIME 2.0f
#define WRAITH_BOSS_ATTACK3_TIME 5.0f
#define WRAITH_BOSS_RUSH_COUNT 3
#define WRAITH_BOSS_PROJECTILE_COOLDOWN 0.5f

#define WRAITH_STATE_DASH        ENEMY_STATE_ATTACK_1
#define WRAITH_STATE_PROJECTILE  ENEMY_STATE_ATTACK_2
#define WRAITH_STATE_SUMMON      ENEMY_STATE_ATTACK_3

void wraithBossUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	if (curEnemy->attackCooldown <= 0 && playerInAttackDistance(index)) {
		curEnemy->attackCooldown = curEnemyData->attackCooldown;
		damagePlayer(curEnemyData->attackDamage);
	}

	if (curEnemy->state == ENEMY_STATE_IDLE) {
		if (curEnemy->stateTimer <= 0) {
			transitionToState(curEnemy, ENEMY_STATE_PREP);
			curEnemy->stateTimer = WRAITH_BOSS_PREP_TIME;
		}
	} else if (curEnemy->state == ENEMY_STATE_PREP) {
		if (curEnemy->stateTimer <= 0) {
			if (curEnemy->nextState == WRAITH_STATE_DASH) {
				// == Dash attack ==
				transitionToState(curEnemy, WRAITH_STATE_DASH);
				curEnemy->stateTimer = WRAITH_BOSS_ATTACK1_TIME;
				curEnemy->stateCount++;

				// Project line towards player and set target to first wall hit
				float targetX, targetY;
				castRayToWall(curEnemy->x, curEnemy->y, playerX, playerY, &targetX, &targetY);
				curEnemy->targetX = targetX;
				curEnemy->targetY = targetY;

				// Rush attack 3 times
				if (curEnemy->stateCount >= WRAITH_BOSS_RUSH_COUNT) {
					curEnemy->stateCount = 0;
					curEnemy->nextState = WRAITH_STATE_PROJECTILE;
				}
			} else if (curEnemy->nextState == WRAITH_STATE_PROJECTILE) {
				// == Projectile attack ==
				transitionToState(curEnemy, WRAITH_STATE_PROJECTILE);
				curEnemy->stateTimer = WRAITH_BOSS_ATTACK2_TIME;
				curEnemy->nextState = WRAITH_STATE_SUMMON;
			} else if (curEnemy->nextState == WRAITH_STATE_SUMMON) {
				// == Summon wraiths ==
				transitionToState(curEnemy, ENEMY_STATE_IDLE);
				curEnemy->stateTimer = WRAITH_BOSS_ATTACK3_TIME;
				curEnemy->nextState = WRAITH_STATE_DASH;
				curEnemy->targetX = MAP_WIDTH / 2 * CELL_SIZE;
				curEnemy->targetY = MAP_HEIGHT / 2 * CELL_SIZE;
				spawnEnemy(ENEMY_TYPE_WRAITH, curEnemy->x, curEnemy->y, true /*isMinion*/);
				spawnEnemy(ENEMY_TYPE_WRAITH, curEnemy->x - CELL_SIZE, curEnemy->y, true /*isMinion*/);
				spawnEnemy(ENEMY_TYPE_WRAITH, curEnemy->x + CELL_SIZE, curEnemy->y, true /*isMinion*/);
			}
		}
	} else if (curEnemy->state == WRAITH_STATE_DASH || curEnemy->state == WRAITH_STATE_PROJECTILE || curEnemy->state == WRAITH_STATE_SUMMON) {
		if (curEnemy->stateTimer <= 0) {
			transitionToState(curEnemy, ENEMY_STATE_IDLE);
			curEnemy->stateTimer = WRAITH_BOSS_IDLE_TIME;
		}

		if (curEnemy->state == WRAITH_STATE_DASH) {
			moveEnemy(curEnemy, index, curEnemy->targetX, curEnemy->targetY, curEnemyData->abilityDataPrimary);
		} else if (curEnemy->state == WRAITH_STATE_PROJECTILE) {
			if (curEnemy->attackCooldown <= 0) {
				curEnemy->attackCooldown = WRAITH_BOSS_PROJECTILE_COOLDOWN;
				float distanceToPlayer = sqrtf(distanceSquared(playerX, playerY, curEnemy->x, curEnemy->y));
				float deltaX = (playerX - curEnemy->x) / distanceToPlayer;
				float deltaY = (playerY - curEnemy->y) / distanceToPlayer;
				float projectileSpeed = curEnemyData->abilityDataSecondary;
				float damage = curEnemyData->attackDamage;
				float projectileX = curEnemy->x + deltaX * PROJECTILE_OFFSET;
				float projectileY = curEnemy->y + deltaY * PROJECTILE_OFFSET;
				fireEnemyProjectile(projectileX, projectileY, deltaX, deltaY, projectileSpeed, damage);
			}
		} else if (curEnemy->state == WRAITH_STATE_SUMMON) {
			moveEnemy(curEnemy, index, curEnemy->targetX, curEnemy->targetY, curEnemyData->moveSpeed);
		}
	}
}

#undef WRAITH_STATE_DASH
#undef WRAITH_STATE_PROJECTILE
#undef WRAITH_STATE_SUMMON
