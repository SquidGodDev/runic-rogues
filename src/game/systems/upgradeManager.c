#define TARGET_EXTENSION 1

#include "globals.h"
#include "game/systems/upgradeManager.h"

#define BASE_MAX_HEALTH 8
#define BASE_MOVE_SPEED 0.8f
#define BASE_PROJECTILE_DAMAGE 1
#define BASE_PROJECTILE_SPEED 4.0f
#define BASE_WEAPON_COOLDOWN 0.6f

typedef struct UpgradeData {
    float modifier;
    char* name_en;
    char* name_jp;
    char* desc_en;
    char* desc_jp;
} UpgradeData;

PlayerStats PLAYER_STATS;

static UpgradeData upgradeData[UPGRADE_TYPE_COUNT] = {
    [UPGRADE_TYPE_MAX_HEALTH] = {
        .modifier = 1,
        .name_en = "Health",
        .name_jp = "Health",
        .desc_en = "Increases max health",
        .desc_jp = "Increases max health"
    },
    [UPGRADE_TYPE_MOVE_SPEED] = {
        .modifier = 0.2f,
        .name_en = "Speed",
        .name_jp = "Speed",
        .desc_en = "Increases move speed",
        .desc_jp = "Increases move speed"
    },
    [UPGRADE_TYPE_ATTACK_COOLDOWN] = {
        .modifier = 0.2f,
        .name_en = "Attack Cooldown",
        .name_jp = "Attack Cooldown",
        .desc_en = "Decreases basic attack cooldown",
        .desc_jp = "Decreases basic attack cooldown"
    },
    [UPGRADE_TYPE_PROJECTILE_SPEED] = {
        .modifier = 0.5f,
        .name_en = "Projectile Speed",
        .name_jp = "Projectile Speed",
        .desc_en = "Increase basic attack projectile speed",
        .desc_jp = "Increase basic attack projectile speed"
    },
    [UPGRADE_TYPE_VAMPIRISM] = {
        .modifier = 0.1f,
        .name_en = "Vampirism",
        .name_jp = "Vampirism",
        .desc_en = "+10%% chance to heal when damaging an enemy",
        .desc_jp = "+10%% chance to heal when damaging an enemy"
    }
};

static UpgradeValue upgradeValues[UPGRADE_TYPE_COUNT];

void initUpgradeManager(void) {
    for (int i = 0; i < UPGRADE_TYPE_COUNT; i++) {
        upgradeValues[i] = (UpgradeValue) {
            .count = 0,
            .value = 0
        };
    }
}

float getUpgradeValue(UpgradeType upgradeType) {
    return upgradeValues[upgradeType].value;
}

void addUpgrade(UpgradeType upgradeType) {
    upgradeValues[upgradeType].count += 1;
}

void initUpgradeValues(void) {
    for (int i = 0; i < UPGRADE_TYPE_COUNT; i++) {
        upgradeValues[i].value = upgradeValues[i].count * upgradeData[i].modifier;
    }

    PLAYER_STATS = (PlayerStats) {
        .maxHealth = BASE_MAX_HEALTH + upgradeValues[UPGRADE_TYPE_MAX_HEALTH].value,
        .maxSpeed = TARGET_FPS * (BASE_MOVE_SPEED + upgradeValues[UPGRADE_TYPE_MOVE_SPEED].value),
        .projectileDamage = BASE_PROJECTILE_DAMAGE,
        .projectileSpeed = TARGET_FPS * (BASE_PROJECTILE_SPEED + upgradeValues[UPGRADE_TYPE_PROJECTILE_SPEED].value),
        .weaponCooldown = BASE_WEAPON_COOLDOWN * (1 - upgradeValues[UPGRADE_TYPE_ATTACK_COOLDOWN].value)
    };
}