#include "game/systems/runStats.h"

#define RUN_STATS_ATTACK_RING 64

static float runTime;
static int   damageDealt;
static int   damageTaken;
static int   totalAttacks;
static int   hitAttacks;
static int   dodges;
static Area  finalArea;
static int   finalRoom;
static bool  victory;

static int monotonicAttackId;
static int activeAttackId;
static int inSyncCastAttackId = -1;

typedef struct {
    int  attackId;
    bool hit;
} AttackSlot;

static AttackSlot attackRing[RUN_STATS_ATTACK_RING];

void runStatsResetForNewRun(void) {
    runTime = 0.0f;
    damageDealt = 0;
    damageTaken = 0;
    totalAttacks = 0;
    hitAttacks = 0;
    dodges = 0;
    finalArea = AREA_TWILIGHT_COURT;
    finalRoom = 0;
    victory = false;

    monotonicAttackId = 0;
    activeAttackId = 0;
    inSyncCastAttackId = -1;
    for (int i = 0; i < RUN_STATS_ATTACK_RING; i++) {
        attackRing[i].attackId = 0;
        attackRing[i].hit = false;
    }
}

void runStatsTick(float deltaTime) {
    runTime += deltaTime;
}

void runStatsRecordDamageDealt(int amount) {
    if (amount <= 0) {
        return;
    }
    damageDealt += amount;
}

void runStatsRecordDamageTaken(int amount) {
    if (amount <= 0) {
        return;
    }
    damageTaken += amount;
}

void runStatsRecordDodge(void) {
    dodges++;
}

int runStatsBeginAttack(void) {
    int id = ++monotonicAttackId;
    int slot = id % RUN_STATS_ATTACK_RING;
    if (attackRing[slot].attackId > 0 && attackRing[slot].hit) {
        hitAttacks++;
    }
    attackRing[slot].attackId = id;
    attackRing[slot].hit = false;
    totalAttacks++;
    activeAttackId = id;
    inSyncCastAttackId = id;
    return id;
}

void runStatsEndSyncCast(void) {
    activeAttackId = 0;
    inSyncCastAttackId = -1;
}

void runStatsPushAttackContext(int attackId) {
    activeAttackId = attackId;
    inSyncCastAttackId = attackId;
}

void runStatsPopAttackContext(void) {
    activeAttackId = 0;
    inSyncCastAttackId = -1;
}

int runStatsGetCurrentAttackId(void) {
    return activeAttackId;
}

void runStatsRecordHitForAttack(int attackId) {
    if (attackId <= 0) {
        return;
    }
    int slot = attackId % RUN_STATS_ATTACK_RING;
    if (attackRing[slot].attackId == attackId) {
        attackRing[slot].hit = true;
    }
}

void runStatsCreditSyncHit(void) {
    if (inSyncCastAttackId > 0) {
        runStatsRecordHitForAttack(inSyncCastAttackId);
    }
}

void runStatsCaptureFinal(bool wasVictory) {
    finalArea = getCurrentArea();
    finalRoom = getCurrentRoom();
    victory = wasVictory;

    for (int i = 0; i < RUN_STATS_ATTACK_RING; i++) {
        if (attackRing[i].attackId > 0 && attackRing[i].hit) {
            hitAttacks++;
            attackRing[i].hit = false;
        }
    }
}

bool runStatsGetVictory(void) {
    return victory;
}

float runStatsGetTime(void) {
    return runTime;
}

char *runStatsMakeTimeString(void) {
    char *out = NULL;
    int totalSeconds = (int)runTime;
    if (totalSeconds < 0) {
        totalSeconds = 0;
    }
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;
    if (hours > 0) {
        pd->system->formatString(&out, "%d:%02d:%02d", hours, minutes, seconds);
    } else {
        pd->system->formatString(&out, "%d:%02d", minutes, seconds);
    }
    return out;
}

int runStatsGetDamageDealt(void) {
    return damageDealt;
}

int runStatsGetDamageTaken(void) {
    return damageTaken;
}

int runStatsGetAccuracyPercent(void) {
    if (totalAttacks <= 0) {
        return -1;
    }
    int clamped = hitAttacks > totalAttacks ? totalAttacks : hitAttacks;
    return (100 * clamped) / totalAttacks;
}

int runStatsGetDodges(void) {
    return dodges;
}

Area runStatsGetFinalArea(void) {
    return finalArea;
}

int runStatsGetFinalRoom(void) {
    return finalRoom;
}
