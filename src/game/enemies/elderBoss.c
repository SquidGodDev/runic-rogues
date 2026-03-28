#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "game/map/map.h"
#include "game/player/player.h"
#include "game/rendering/particles.h"
#include "game/enemies/enemies.h"

#define ELDER_BOSS_PREP_TIME 2.0f
#define ELDER_BOSS_ATTACK2_COUNT 5
#define ELDER_BOSS_ATTACK2_SPACING CELL_SIZE

void elderBossUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	if (curEnemy->state == ENEMY_STATE_IDLE) {
		if (playerInAttackDetectionRange(index)) {
			transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
		} else if (curEnemy->stateTimer <= 0) {
			curEnemy->stateTimer = ELDER_BOSS_PREP_TIME;
			transitionToState(curEnemy, ENEMY_STATE_PREP);

			for (int dir = 0; dir < 8; dir++) {
				float angle = dir * (PI / 4);
				float dx = cosf(angle);
				float dy = sinf(angle);
				for (int root = 1; root <= ELDER_BOSS_ATTACK2_COUNT; root++) {
					float offsetX = dx * (float)(root * ELDER_BOSS_ATTACK2_SPACING);
					float offsetY = dy * (float)(root * ELDER_BOSS_ATTACK2_SPACING);
					spawnParticle(PARTICLE_TYPE_WARNING, curEnemy->x + offsetX, curEnemy->y + offsetY, PARTICLE_TYPE_BOSS_ROOT_ATTACK, true /* enemyParticle */);
				}
			}

		} else {
			moveEnemy(curEnemy, index, playerX, playerY, curEnemyData->moveSpeed);
		}
	} else if (curEnemy->state == ENEMY_STATE_PREP) {
		if (curEnemy->stateTimer <= 0) {
			curEnemy->stateTimer = ELDER_BOSS_IDLE_TIME;
			curEnemy->state = ENEMY_STATE_IDLE;
		}
	} else if (curEnemy->state == ENEMY_STATE_ATTACK_1) {
		if (!curEnemy->attackedPlayer && curEnemy->animationFrame >= curEnemyData->attackActiveFrame) {
			if (playerInAttackDistance(index)) {
				curEnemy->attackedPlayer = true;
				damagePlayer(curEnemyData->attackDamage);
			}
		}

		if (attackAnimationReset) {
			curEnemy->attackedPlayer = false;
			if (playerInAttackDetectionRange(index)) {
				transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
			} else {
				transitionToState(curEnemy, ENEMY_STATE_IDLE);
			}
		}
	}
}
