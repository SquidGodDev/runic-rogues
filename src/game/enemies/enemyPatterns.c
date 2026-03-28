#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "game/map/map.h"
#include "game/player/player.h"
#include "game/entities/projectiles.h"
#include "game/rendering/particles.h"
#include "game/enemies/enemies.h"
#include "game/enemies/enemyInternal.h"

void proximityEnemyUpdate(Enemy *curEnemy, EnemyData *curEnemyData, int index, bool attackAnimationReset) {
	// Constantly attack nearby player
	if (playerInAttackDistance(index)) {
		if (curEnemy->attackCooldown <= 0) {
			curEnemy->attackCooldown = curEnemyData->attackCooldown;
			damagePlayer(curEnemyData->attackDamage);
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
		// Start attacking if player is in range
		if (playerInAttackDetectionRange(index)) {
			if (curEnemy->attackCooldown <= 0) {
				curEnemy->attackCooldown = curEnemyData->attackCooldown;
				transitionToState(curEnemy, ENEMY_STATE_ATTACK_1);
			}
		} else {
			curEnemy->attackCooldown = curEnemyData->attackCooldown;
		}

		// Move towards player
		moveEnemy(curEnemy, index, playerX, playerY, curEnemyData->moveSpeed);
	} else if (curEnemy->state == ENEMY_STATE_ATTACK_1) {
		// Damage player once attack animation passes active frame
		if (curEnemy->animationFrame >= curEnemyData->attackActiveFrame) {
			if (!curEnemy->attackedPlayer && playerInAttackDistance(index)) {
				curEnemy->attackedPlayer = true;
				damagePlayer(curEnemyData->attackDamage);
			}
		}

		// Transition back to idle state after attack animation ends
		if (attackAnimationReset) {
			curEnemy->attackedPlayer = false;
			curEnemy->attackCooldown = curEnemyData->attackCooldown;
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
			damagePlayer(curEnemyData->attackDamage);
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
