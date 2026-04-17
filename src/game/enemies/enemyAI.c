#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "game/map/map.h"
#include "game/enemies/enemyAI.h"
#include "game/player/player.h"
#include "game/entities/projectiles.h"
#include "game/rendering/particles.h"
#include "game/UI/UI.h"

// === Elder Boss constants ===
#define ELDER_BOSS_PREP_TIME 2.0f
#define ELDER_BOSS_ATTACK2_COUNT 5
#define ELDER_BOSS_ATTACK2_SPACING CELL_SIZE

// === Wraith Boss constants ===
#define WRAITH_BOSS_PREP_TIME 1.0f
#define WRAITH_BOSS_ATTACK1_TIME 2.5f
#define WRAITH_BOSS_ATTACK2_TIME 2.0f
#define WRAITH_BOSS_ATTACK3_TIME 5.0f
#define WRAITH_BOSS_RUSH_COUNT 3
#define WRAITH_BOSS_PROJECTILE_COOLDOWN 0.5f

#define WRAITH_STATE_DASH        ENEMY_STATE_ATTACK_1
#define WRAITH_STATE_PROJECTILE  ENEMY_STATE_ATTACK_2
#define WRAITH_STATE_SUMMON      ENEMY_STATE_ATTACK_3

void proximityEnemyUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	// Constantly attack nearby player
	if (playerInAttackDistance(index)) {
		if (curEnemy->attackCooldown <= 0) {
			curEnemy->attackCooldown = curEnemyData->attackCooldown;
			damagePlayerFromDirection(curEnemyData->attackDamage, curEnemy->x, curEnemy->y);
		}
	} else {
		curEnemy->attackCooldown = curEnemyData->attackCooldown;
	}

	if (!playerInAttackDetectionRange(index)) {
		// Constantly move towards player if not in attack range
		moveEnemy(curEnemy, index, playerX, playerY, curEnemyData->moveSpeed);
	}
}

void meleeEnemyUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	if (curEnemy->state == ENEMY_STATE_IDLE) {
		// Begin prep phase if player is in range and cooldown is ready
		if (playerInAttackDetectionRange(index)) {
			if (curEnemy->attackCooldown <= 0) {
				curEnemy->attackCooldown = curEnemyData->attackCooldown;
				curEnemy->stateTimer = curEnemyData->prepTime;
				transitionToState(curEnemy, ENEMY_STATE_PREP);
				addAttackIntentIndicator(curEnemy->x, curEnemy->y, curEnemyData->prepTime);
			}
		} else {
			curEnemy->attackCooldown = curEnemyData->attackCooldown;
		}

		// Move towards player
		moveEnemy(curEnemy, index, playerX, playerY, curEnemyData->moveSpeed);
	} else if (curEnemy->state == ENEMY_STATE_PREP) {
		if (curEnemy->stateTimer <= 0) {
			transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
		}
	} else if (curEnemy->state == ENEMY_STATE_ATTACK_1) {
		// Damage player once attack animation passes active frame
		if (curEnemy->animationFrame >= curEnemyData->attackActiveFrame) {
			if (!curEnemy->attackedPlayer && playerInAttackDistance(index)) {
				curEnemy->attackedPlayer = true;
				damagePlayerFromDirection(curEnemyData->attackDamage, curEnemy->x, curEnemy->y);
			}
		}

		// Transition to post attack state after attack animation ends
		if (attackAnimationReset) {
			curEnemy->attackedPlayer = false;
			curEnemy->attackCooldown = curEnemyData->attackCooldown;
			curEnemy->stateTimer = curEnemyData->postAttackTime;
			curEnemy->animationFrame = curEnemyData->attackFrameEnd - 1;
			transitionToState(curEnemy, ENEMY_STATE_POST_ATTACK);
		}
	} else if (curEnemy->state == ENEMY_STATE_POST_ATTACK) {
		if (curEnemy->stateTimer <= 0) {
			transitionToState(curEnemy, ENEMY_STATE_IDLE);
		}
	}
}

void projectileEnemyUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	if (curEnemy->state == ENEMY_STATE_IDLE) {
		// Start shooting projectile if player is in range
		if (curEnemy->attackCooldown <= 0 && playerInAttackDetectionRange(index)) {
			curEnemy->attackCooldown = curEnemyData->attackCooldown;
			transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
			addAttackIntentIndicator(curEnemy->x, curEnemy->y, 0.0f);
		}

		// Choose a random target position to move towards
		if (curEnemy->abilityCooldown <= 0) {
			if (randInt(0, 100) < 33) {
				curEnemy->targetX = playerX;
				curEnemy->targetY = playerY;
			} else {
				curEnemy->targetX = curEnemy->x + randInt(-1, 1) * CELL_SIZE;
				curEnemy->targetY = curEnemy->y + randInt(-1, 1) * CELL_SIZE;
			}
			curEnemy->abilityCooldown = curEnemyData->abilityDataSecondary;
		}
		moveEnemy(curEnemy, index, curEnemy->targetX, curEnemy->targetY, curEnemyData->moveSpeed);
	} else if (curEnemy->state == ENEMY_STATE_ATTACK_1) {
		// Fire projectile towards player if attack animation passes active frame
		if (!curEnemy->attackedPlayer && curEnemy->animationFrame >= curEnemyData->attackActiveFrame) {
			curEnemy->attackedPlayer = true;
			float distanceToPlayer = sqrtf(distanceSquared(playerX, playerY, curEnemy->x, curEnemy->y));
			float deltaX = (playerX - curEnemy->x) / distanceToPlayer;
			float deltaY = (playerY - curEnemy->y) / distanceToPlayer;
			float projectileSpeed = curEnemyData->abilityDataPrimary;
			float damage = curEnemyData->attackDamage;
			float projectileX = curEnemy->x + deltaX * PROJECTILE_OFFSET;
			float projectileY = curEnemy->y + deltaY * PROJECTILE_OFFSET;
			fireEnemyProjectile(projectileX, projectileY, deltaX, deltaY, projectileSpeed, damage);
		}

		// Transition back to idle state after attack animation ends
		if (attackAnimationReset) {
			curEnemy->attackedPlayer = false;
			transitionToState(curEnemy, ENEMY_STATE_IDLE);
		}
	}
}

void rushEnemyUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	if (curEnemy->state == ENEMY_STATE_IDLE) {
		// Start rushing in fixed direction until hitting wall if player is in attack detection range
		if (curEnemy->attackCooldown <= 0 && playerInAttackDetectionRange(index)) {
			curEnemy->attackCooldown = curEnemyData->attackCooldown;
			curEnemy->abilityCooldown = curEnemyData->abilityDataSecondary;
			float targetX, targetY;
			castRayToWall(curEnemy->x, curEnemy->y, playerX, playerY, &targetX, &targetY);
			curEnemy->targetX = targetX;
			curEnemy->targetY = targetY;
			transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
		}

		// Move towards player at regular speed when idle
		moveEnemy(curEnemy, index, playerX, playerY, curEnemyData->moveSpeed);
	} else if (curEnemy->state == ENEMY_STATE_ATTACK_1) {
		// Damage player if nearby while rushing
		if (playerInAttackDistance(index)) {
			damagePlayerFromDirection(curEnemyData->attackDamage, curEnemy->x, curEnemy->y);
		}

		// Once rush timer is over, transition back to idle state
		if (curEnemy->abilityCooldown <= 0) {
			curEnemy->attackCooldown = curEnemyData->attackCooldown;
			transitionToState(curEnemy, ENEMY_STATE_IDLE);
		}

		// Move towards target position at rush speed
		moveEnemy(curEnemy, index, curEnemy->targetX, curEnemy->targetY, curEnemyData->abilityDataPrimary);
	}
}

void mummyUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	if (curEnemy->state == ENEMY_STATE_IDLE) {
		if (curEnemy->attackCooldown <= 0 && playerInAttackDetectionRange(index)) {
			curEnemy->attackCooldown = curEnemyData->attackCooldown;
			transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
		}

		moveEnemy(curEnemy, index, playerX, playerY, curEnemyData->moveSpeed);
	} else if (curEnemy->state == ENEMY_STATE_ATTACK_1) {
		if (curEnemy->animationFrame >= curEnemyData->attackActiveFrame) {
			if (!curEnemy->attackedPlayer) {
				curEnemy->attackedPlayer = true;
				spawnEnemy(ENEMY_TYPE_WRAITH, curEnemy->x, curEnemy->y, true /*isMinion*/);
			}
		}

		if (attackAnimationReset) {
			curEnemy->attackedPlayer = false;
			curEnemy->stateTimer = 0;
			curEnemy->stateCount = 0;
			transitionToState(curEnemy, ENEMY_STATE_IDLE);
		}
	}
}

void serpentUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	if (curEnemy->state == ENEMY_STATE_IDLE) {
		if (curEnemy->attackCooldown <= 0 && playerInAttackDetectionRange(index)) {
			curEnemy->attackCooldown = curEnemyData->attackCooldown;
			transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
		}

		moveEnemy(curEnemy, index, playerX, playerY, curEnemyData->moveSpeed);
	} else if (curEnemy->state == ENEMY_STATE_ATTACK_1) {
		// Spawn tail attack particles at intervals during attack animation
		if (curEnemy->animationFrame >= curEnemyData->attackActiveFrame) {
			if (!curEnemy->attackedPlayer) {
				curEnemy->attackedPlayer = true;
				curEnemy->targetX = playerX;
				curEnemy->targetY = playerY;
			}
			int attackCount = curEnemy->stateCount;
			int maxAttackCount = curEnemyData->abilityDataPrimary;
			if (curEnemy->stateTimer <= 0 && attackCount < maxAttackCount) {
				float dx = curEnemy->targetX - curEnemy->x;
				float dy = curEnemy->targetY - curEnemy->y;
				if (dx == 0 && dy == 0) {
					dx = 1;
					dy = 0;
				}
				float length = sqrtf(dx * dx + dy * dy);
				if (length > 0) {
					dx /= length;
					dy /= length;
				}
				float offsetX = dx * (curEnemy->stateCount + 1) * CELL_SIZE / 2;
				float offsetY = dy * (curEnemy->stateCount + 1) * CELL_SIZE / 2;
				spawnParticle(PARTICLE_TYPE_SERPENT_TAIL, curEnemy->x + offsetX, curEnemy->y + offsetY, PARTICLE_TYPE_NONE, true /* enemyParticle */);
				curEnemy->stateCount++;
				curEnemy->stateTimer = curEnemyData->abilityDataSecondary;
			}
		}

		if (attackAnimationReset) {
			curEnemy->attackedPlayer = false;
			curEnemy->stateTimer = 0;
			curEnemy->stateCount = 0;
			transitionToState(curEnemy, ENEMY_STATE_IDLE);
		}
	}
}

void turretEnemyUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	if (curEnemy->state == ENEMY_STATE_IDLE) {
		// Start attacking if player is in range
		if (curEnemy->attackCooldown <= 0 && playerInAttackDetectionRange(index)) {
			curEnemy->attackCooldown = curEnemyData->attackCooldown;

			float angle = curEnemy->stateCount * (PI * 2 / curEnemyData->abilityDataPrimary);
			float deltaX = cosf(angle);
			float deltaY = sinf(angle);
			float projectileSpeed = curEnemyData->abilityDataSecondary;
			float damage = curEnemyData->attackDamage;
			float projectileX = curEnemy->x + deltaX * PROJECTILE_OFFSET;
			float projectileY = curEnemy->y + deltaY * PROJECTILE_OFFSET;
			fireEnemyProjectile(projectileX, projectileY, deltaX, deltaY, projectileSpeed, damage);

			curEnemy->stateCount += 1;
			if (curEnemy->stateCount >= curEnemyData->abilityDataPrimary) {
				curEnemy->stateCount = 0;
			}
		}
	}
}

void elderBossUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	if (curEnemy->state == ENEMY_STATE_IDLE) {
		if (playerInAttackDetectionRange(index)) {
			transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
		} else if (curEnemy->stateTimer <= 0) {
			curEnemy->stateTimer = ELDER_BOSS_PREP_TIME;
			transitionToState(curEnemy, ENEMY_STATE_PREP);
			addAttackIntentIndicator(curEnemy->x, curEnemy->y, ELDER_BOSS_PREP_TIME);

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
				damagePlayerFromDirection(curEnemyData->attackDamage, curEnemy->x, curEnemy->y);
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

void wraithBossUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	if (curEnemy->attackCooldown <= 0 && playerInAttackDistance(index)) {
		curEnemy->attackCooldown = curEnemyData->attackCooldown;
		damagePlayerFromDirection(curEnemyData->attackDamage, curEnemy->x, curEnemy->y);
	}

	if (curEnemy->state == ENEMY_STATE_IDLE) {
		if (curEnemy->stateTimer <= 0) {
			transitionToState(curEnemy, ENEMY_STATE_PREP);
			curEnemy->stateTimer = WRAITH_BOSS_PREP_TIME;
			addAttackIntentIndicator(curEnemy->x, curEnemy->y, WRAITH_BOSS_PREP_TIME);
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
				Point bossRoomCenter = getRoomWorldCenter(DUNGEON_ROOM_COUNT - 1);
				curEnemy->targetX = bossRoomCenter.x;
				curEnemy->targetY = bossRoomCenter.y;
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

// === Circling charge constants ===
#define CIRCLING_RANGE   48.0f   // ~3 cells: distance at which enemy starts circling/pausing
#define ORBIT_RADIUS     48.0f   // orbit distance from player during circling
#define ORBIT_LEAD       0.15f   // radians ahead of current angle for orbit target (must keep target > 5 units from enemy)
#define STUN_DURATION    1.5f    // seconds stunned after charge
#define CIRCLE_TIME_MIN  1.2f    // min seconds in PREP before charge
#define CIRCLE_TIME_MAX  2.8f    // max seconds in PREP before charge

static void setupCharge(Enemy *curEnemy) {
    float wallX, wallY;
    castRayToWall(curEnemy->x, curEnemy->y, playerX, playerY, &wallX, &wallY);
    curEnemy->targetX = wallX;
    curEnemy->targetY = wallY;
    curEnemy->attackedPlayer = false;
}

void circlingChargeEnemyUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
    if (curEnemy->state == ENEMY_STATE_IDLE) {
        float distSq = distanceSquared(curEnemy->x, curEnemy->y, playerX, playerY);
        if (distSq <= CIRCLING_RANGE * CIRCLING_RANGE) {
            curEnemy->stateCount = (randInt(0, 1) == 0) ? 1 : -1;
            curEnemy->stateTimer = randFloat(CIRCLE_TIME_MIN, CIRCLE_TIME_MAX);
            transitionToState(curEnemy, ENEMY_STATE_PREP);
            addAttackIntentIndicator(curEnemy->x, curEnemy->y, curEnemy->stateTimer);
        } else {
            moveEnemy(curEnemy, index, playerX, playerY, curEnemyData->moveSpeed);
        }
    } else if (curEnemy->state == ENEMY_STATE_PREP) {
        float dx = curEnemy->x - playerX;
        float dy = curEnemy->y - playerY;
        float angle = atan2f(dy, dx) + ORBIT_LEAD * curEnemy->stateCount;
        float orbitX = playerX + cosf(angle) * ORBIT_RADIUS;
        float orbitY = playerY + sinf(angle) * ORBIT_RADIUS;
        moveEnemy(curEnemy, index, orbitX, orbitY, curEnemyData->moveSpeed);

        if (curEnemy->stateTimer <= 0) {
            setupCharge(curEnemy);
            transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
        }
    } else if (curEnemy->state == ENEMY_STATE_ATTACK_1) {
        if (playerInAttackDistance(index) && !curEnemy->attackedPlayer) {
            curEnemy->attackedPlayer = true;
            damagePlayerFromDirection(curEnemyData->attackDamage, curEnemy->x, curEnemy->y);
        }
        moveEnemy(curEnemy, index, curEnemy->targetX, curEnemy->targetY, curEnemyData->abilityDataPrimary);
        float dtx = curEnemy->targetX - curEnemy->x;
        float dty = curEnemy->targetY - curEnemy->y;
        if (dtx * dtx + dty * dty <= 64.0f) {
            curEnemy->stateTimer = STUN_DURATION;
            transitionToState(curEnemy, ENEMY_STATE_ATTACK_2);
        }
    } else if (curEnemy->state == ENEMY_STATE_ATTACK_2) {
        if (curEnemy->stateTimer <= 0) {
            transitionToState(curEnemy, ENEMY_STATE_IDLE);
        }
    }
}

void chargeEnemyUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
    if (curEnemy->state == ENEMY_STATE_IDLE) {
        float distSq = distanceSquared(curEnemy->x, curEnemy->y, playerX, playerY);
        if (distSq <= CIRCLING_RANGE * CIRCLING_RANGE) {
            curEnemy->stateCount = (randInt(0, 1) == 0) ? 1 : -1;
            curEnemy->stateTimer = randFloat(CIRCLE_TIME_MIN, CIRCLE_TIME_MAX);
            transitionToState(curEnemy, ENEMY_STATE_PREP);
            addAttackIntentIndicator(curEnemy->x, curEnemy->y, curEnemy->stateTimer);
        } else {
            moveEnemy(curEnemy, index, playerX, playerY, curEnemyData->moveSpeed);
        }
    } else if (curEnemy->state == ENEMY_STATE_PREP) {
        if (curEnemy->stateTimer <= 0) {
            setupCharge(curEnemy);
            transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
        }
    } else if (curEnemy->state == ENEMY_STATE_ATTACK_1) {
        if (playerInAttackDistance(index) && !curEnemy->attackedPlayer) {
            curEnemy->attackedPlayer = true;
            damagePlayerFromDirection(curEnemyData->attackDamage, curEnemy->x, curEnemy->y);
        }
        moveEnemy(curEnemy, index, curEnemy->targetX, curEnemy->targetY, curEnemyData->abilityDataPrimary);
        float dtx = curEnemy->targetX - curEnemy->x;
        float dty = curEnemy->targetY - curEnemy->y;
        if (dtx * dtx + dty * dty <= 64.0f) {
            curEnemy->stateTimer = STUN_DURATION;
            transitionToState(curEnemy, ENEMY_STATE_ATTACK_2);
        }
    } else if (curEnemy->state == ENEMY_STATE_ATTACK_2) {
        if (curEnemy->stateTimer <= 0) {
            transitionToState(curEnemy, ENEMY_STATE_IDLE);
        }
    }
}
