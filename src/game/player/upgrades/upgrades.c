#include "game/player/upgrades/upgrades.h"

const UpgradeData UPGRADE_DATA[UPGRADE_COUNT] = {
    // TODO
};

int playerUpgradeLevels[UPGRADE_COUNT];

void initUpgrades(void) {
    for (int i = 0; i < UPGRADE_COUNT; i++) {
        playerUpgradeLevels[i] = 0;
    }
}