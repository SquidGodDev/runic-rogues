#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals.h"
#include "game/systems/upgradeManager.h"
#include "game/settings/settings.h"
#include "game/rendering/textures.h"

#define BASE_MAX_HEALTH 8
#define BASE_MOVE_SPEED 0.8f
#define BASE_PROJECTILE_DAMAGE 1
#define BASE_PROJECTILE_SPEED 4.0f
#define BASE_WEAPON_COOLDOWN 0.6f

#define UPGRADE_OPTION_COUNT 3

typedef struct UpgradeData {
    float modifier;
    LCDBitmap *icon;
    char* name_en;
    char* name_jp;
    char* desc_en;
    char* desc_jp;
} UpgradeData;

PlayerStats PLAYER_STATS;

static UpgradeData upgradeData[UPGRADE_TYPE_COUNT];

static UpgradeValue upgradeValues[UPGRADE_TYPE_COUNT];

float getUpgradeValue(UpgradeType upgradeType) {
    return upgradeValues[upgradeType].value;
}

void addUpgrade(UpgradeType upgradeType) {
    upgradeValues[upgradeType].count += 1;
}

int upgradeOptions[UPGRADE_OPTION_COUNT];
int getUpgradeOption(lua_State* L) {
    int option = pd->lua->getArgInt(1);
    if (option == 0) {
        int count = 0;
        while (count < UPGRADE_OPTION_COUNT) {
            int num = rand() % UPGRADE_TYPE_COUNT;
            int duplicate = 0;
            for (int i = 0; i < count; i++) {
                if (upgradeOptions[i] == num) {
                    duplicate = 1;
                    break;
                }
            }

            if (!duplicate) {
                upgradeOptions[count++] = num;
            }
        }
    }

    // Return name, description, and icon
    UpgradeData curUpgradeData = upgradeData[upgradeOptions[option]];
    if (SETTINGS.language == LANGUAGE_EN) {
        pd->lua->pushString(curUpgradeData.name_en);
        pd->lua->pushString(curUpgradeData.desc_en);
    } else if (SETTINGS.language == LANGUAGE_JP) {
        pd->lua->pushString(curUpgradeData.name_jp);
        pd->lua->pushString(curUpgradeData.desc_jp);
    }
    pd->lua->pushBitmap(curUpgradeData.icon);

    return 3;
}

void selectUpgradeOption(int option) {
    addUpgrade(upgradeOptions[option]);
}

void initUpgradeManager(void) {
    pd->lua->addFunction(getUpgradeOption, "Upgrades.getUpgradeOption", NULL);

    upgradeData[UPGRADE_TYPE_MAX_HEALTH] = (UpgradeData) {
        .modifier = 1,
        .icon = BITMAPS[BITMAP_UPGRADE_MAX_HEALTH],
        .name_en = "Health",
        .name_jp = "体力",
        .desc_en = "Increases max health",
        .desc_jp = "最大体力の増加"
    };
    upgradeData[UPGRADE_TYPE_MOVE_SPEED] = (UpgradeData) {
        .modifier = 0.2f,
        .icon = BITMAPS[BITMAP_UPGRADE_MOVE_SPEED],
        .name_en = "Speed",
        .name_jp = "スピード",
        .desc_en = "Increases move speed",
        .desc_jp = "移動速度の増加"
    };
    upgradeData[UPGRADE_TYPE_ATTACK_COOLDOWN] = (UpgradeData) {
        .modifier = 0.2f,
        .icon = BITMAPS[BITMAP_UPGRADE_ATTACK_COOLDOWN],
        .name_en = "Attack Cooldown",
        .name_jp = "攻撃CD短縮",
        .desc_en = "Decreases basic attack cooldown",
        .desc_jp = "基本攻撃のクールダウンを短縮"
    };
    upgradeData[UPGRADE_TYPE_PROJECTILE_SPEED] = (UpgradeData) {
        .modifier = 0.5f,
        .icon = BITMAPS[BITMAP_UPGRADE_PROJECTILE_SPEED],
        .name_en = "Projectile Speed",
        .name_jp = "弾速強化",
        .desc_en = "Increases basic attack projectile speed",
        .desc_jp = "基本攻撃の弾速が上がる"
    };
    upgradeData[UPGRADE_TYPE_VAMPIRISM] = (UpgradeData) {
        .modifier = 0.1f,
        .icon = BITMAPS[BITMAP_UPGRADE_VAMPIRISM],
        .name_en = "Vampirism",
        .name_jp = "吸血鬼",
        .desc_en = "+10% chance to heal when damaging an enemy",
        .desc_jp = "敵にダメージを与えた際に回復確率が10%増加"
    };
}

void initUpgradeValues(int currentArea, int currentRoom) {
    if (currentArea == 0 && currentRoom == 0) {
        for (int i = 0; i < UPGRADE_TYPE_COUNT; i++) {
            upgradeValues[i] = (UpgradeValue) {
                .count = 0,
                .value = 0
            };
        }
    }

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