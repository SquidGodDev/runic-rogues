#pragma once

#include "game/enemies/enemies.h"

// Generic enemy update functions
void proximityEnemyUpdate(Enemy *enemy, EnemyData *data, int index, bool attackAnimationReset);
void meleeEnemyUpdate(Enemy *enemy, EnemyData *data, int index, bool attackAnimationReset);
void hitAndRunEnemyUpdate(Enemy *enemy, EnemyData *data, int index, bool attackAnimationReset);
void comboMeleeEnemyUpdate(Enemy *enemy, EnemyData *data, int index, bool attackAnimationReset);
void slamEnemyUpdate(Enemy *enemy, EnemyData *data, int index, bool attackAnimationReset);
void projectileEnemyUpdate(Enemy *enemy, EnemyData *data, int index, bool attackAnimationReset);
void proximityProjectileEnemyUpdate(Enemy *enemy, EnemyData *data, int index, bool attackAnimationReset);
void lobberEnemyUpdate(Enemy *enemy, EnemyData *data, int index, bool attackAnimationReset);
void rushEnemyUpdate(Enemy *enemy, EnemyData *data, int index, bool attackAnimationReset);
void turretEnemyUpdate(Enemy *enemy, EnemyData *data, int index, bool attackAnimationReset);
void mummyUpdate(Enemy *enemy, EnemyData *data, int index, bool attackAnimationReset);
void bomberUpdate(Enemy *enemy, EnemyData *data, int index, bool attackAnimationReset);
void serpentUpdate(Enemy *enemy, EnemyData *data, int index, bool attackAnimationReset);
void airElementalUpdate(Enemy *enemy, EnemyData *data, int index, bool attackAnimationReset);
void rockGolemUpdate(Enemy *enemy, EnemyData *data, int index, bool attackAnimationReset);
void circlingChargeEnemyUpdate(Enemy *enemy, EnemyData *data, int index, bool attackAnimationReset);
void chargeEnemyUpdate(Enemy *enemy, EnemyData *data, int index, bool attackAnimationReset);
void dummyUpdate(Enemy *enemy, EnemyData *data, int index, bool attackAnimationReset);

// Boss update functions
void elderBossUpdate(Enemy *enemy, EnemyData *data, int index, bool attackAnimationReset);
void wraithBossUpdate(Enemy *enemy, EnemyData *data, int index, bool attackAnimationReset);
