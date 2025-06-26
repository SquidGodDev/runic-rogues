#pragma once

typedef struct PlayerStats {
	int maxHealth;
    int maxMana;
    float manaRegenRate;
	float maxSpeed;
	int projectileDamage;
	float projectileSpeed;
	float weaponCooldown;
} PlayerStats;

extern PlayerStats PLAYER_STATS;

typedef enum UpgradeType {
    UPGRADE_TYPE_MAX_HEALTH,
    UPGRADE_TYPE_MAX_MANA,
    UPGRADE_TYPE_MANA_REGEN,
    UPGRADE_TYPE_MOVE_SPEED,
    UPGRADE_TYPE_ATTACK_COOLDOWN,
    UPGRADE_TYPE_PROJECTILE_SPEED,
    UPGRADE_TYPE_VAMPIRISM,
    UPGRADE_TYPE_COUNT
} UpgradeType;

typedef struct UpgradeValue {
    int count;
    float value;
} UpgradeValue;

void initUpgradeManager(void);
float getUpgradeValue(UpgradeType upgradeType);
void addUpgrade(UpgradeType upgradeType);
void initUpgradeValues(int isGameStart);