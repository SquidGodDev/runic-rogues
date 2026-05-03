#pragma once

#include "pd_api.h"
#include "globals/globals.h"
#include "game/systems/progressionManager.h"

void runStatsResetForNewRun(void);
void runStatsTick(float deltaTime);

void runStatsRecordDamageDealt(int amount);
void runStatsRecordDamageTaken(int amount);
void runStatsRecordDodge(void);

int  runStatsBeginAttack(void);
void runStatsEndSyncCast(void);
void runStatsPushAttackContext(int attackId);
void runStatsPopAttackContext(void);
int  runStatsGetCurrentAttackId(void);
void runStatsRecordHitForAttack(int attackId);
void runStatsCreditSyncHit(void);

void runStatsCaptureFinal(bool victory);

bool  runStatsGetVictory(void);
float runStatsGetTime(void);
int   runStatsGetDamageDealt(void);
int   runStatsGetDamageTaken(void);
int   runStatsGetAccuracyPercent(void);
int   runStatsGetDodges(void);
Area  runStatsGetFinalArea(void);
int   runStatsGetFinalRoom(void);
