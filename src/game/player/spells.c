#include "globals.h"
#include "helpers.h"

#include "game/entities/projectiles.h"
#include "game/UI/UI.h"
#include "game/player/player.h"
#include "game/player/spells.h"
#include "game/systems/audioManager.h"
#include "game/systems/upgradeManager.h"
#include "game/enemies/enemies.h"
#include "game/map/map.h"

// Spawn the projectile slightly to the right to match hand
#define FIRE_ANGLE_OFFSET PI / 8
#define FIRE_POSITION_OFFSET CELL_SIZE / 2

#define UTILITY_SPELL_COUNT 3

typedef struct SpellCastData {
    Spell spell;
    float castTime;
    float activationTime;
    int activationCount;
    float initialCastAngle;
} SpellCastData;

static Spell spellDeck[SPELL_DECK_MAX];
static SpellCastData spellCastData[SPELL_DECK_MAX];
float spellCooldowns[SPELL_DECK_MAX];
static int utilitySpellCount;
static float utilitySpellCharge;

SpellData spellData[SPELL_COUNT] = {
    [SPELL_NONE] = {
        .name = "None",
        .name_jp = "なし",
        .description = "Empty",
        .spellIconIndex = BITMAP_SPELL_EMPTY_ICON,
    },
    [SPELL_FIREBALL] = {
        .name = "Fireball",
        .name_jp = "ファイアボール",
        .description = "Shoot a projectile that deals medium damage to one enemy",
        .description_jp = "敵に中程度のダメージを与える弾を発射する",
        .pickupTextureIndex = TEXTURE_SPELL_FIREBALL,
        .spellTextureIndex = TEXTURE_PROJECTILE,
        .spellIconIndex = BITMAP_SPELL_FIREBALL_ICON,
        .type = SPELL_TYPE_INSTANT,
        .cooldown = 0.5f,
        .castTime = 0.4f,
        .damage = 1,
        .projectileData = (SpellProjectileData) {
            .speed = 75.0f
        }
    },
    [SPELL_TRIPLE_SHOT] = {
        .name = "Triple Shot",
        .name_jp = "トリプルショット",
        .description = "Shoot three projectiles, one after another",
        .description_jp = "3つの弾を連続で発射する",
        .pickupTextureIndex = TEXTURE_SPELL_TRIPLE_SHOT,
        .spellTextureIndex = TEXTURE_PROJECTILE,
        .spellIconIndex = BITMAP_SPELL_TRIPLE_SHOT_ICON,
        .type = SPELL_TYPE_REPEAT,
        .cooldown = 3.0f,
        .castTime = 1.0f,
        .damage = 1,
        .repeatData = (RepeatData){
            .repeatCount = 3,
            .activationTime = 0.3f
        },
        .projectileData = (SpellProjectileData){
            .speed = 150.0f,
        }
    },
    [SPELL_SHOTGUN] = {
        .name = "Shotgun",
        .name_jp = "ショットガン",
        .description = "Fire a spread of three projectiles in a cone",
        .description_jp = "扇形に3つの弾を発射する",
        .pickupTextureIndex = TEXTURE_SPELL_SHOTGUN,
        .spellTextureIndex = TEXTURE_PROJECTILE,
        .spellIconIndex = BITMAP_SPELL_SHOTGUN_ICON,
        .type = SPELL_TYPE_INSTANT,
        .cooldown = 3.0f,
        .castTime = 0.4f,
        .damage = 1,
        .projectileData = (SpellProjectileData){
            .speed = 100.0f
        }
    },
    [SPELL_LASER] = {
        .name = "Laser",
        .name_jp = "レーザー",
        .description = "Fire a continuous shot of projectiles in a straight line",
        .description_jp = "直線上に弾を連続で発射する",
        .pickupTextureIndex = TEXTURE_SPELL_LASER,
        .spellIconIndex = BITMAP_SPELL_LASER_ICON,
        .type = SPELL_TYPE_REPEAT,
        .cooldown = 7.0f,
        .castTime = 0.6f,
        .damage = 1,
        .playerSpeedModifier = -0.7f,
        .useInitialCastAngle = true,
        .repeatData = (RepeatData){
            .activationTime = 0.1f,
            .repeatCount = 5
        },
        .projectileData = (SpellProjectileData) {
            .hitCount = 3,
            .speed = 300.0f
        }
    },
    [SPELL_ROOT_SURGE] = {
        .name = "Root Surge",
        .name_jp = "ルートサージ",
        .description = "Summons two waves of roots that pierce up from the ground",
        .description_jp = "地面から突き出る2つの波の根を召喚する",
        .pickupTextureIndex = TEXTURE_SPELL_ROOT_SURGE,
        .spellIconIndex = BITMAP_SPELL_ROOT_SURGE_ICON,
        .type = SPELL_TYPE_REPEAT,
        .cooldown = 6.0f,
        .castTime = 1.2f,
        .damage = 4,
        .playerSpeedModifier = -0.3f,
        .useInitialCastAngle = true,
        .repeatData = (RepeatData){
            .activationTime = 0.2f,
            .repeatCount = 6
        }
    },
    [SPELL_WAVE] = {
        .name = "Wave",
        .name_jp = "ウェーブ",
        .description = "Unleashes a wave of projectiles",
        .description_jp = "弾の波を放つ",
        .pickupTextureIndex = TEXTURE_SPELL_WAVE,
        .spellTextureIndex = TEXTURE_PROJECTILE,
        .spellIconIndex = BITMAP_SPELL_WAVE_ICON,
        .type = SPELL_TYPE_INSTANT,
        .cooldown = 6.0f,
        .castTime = 0.4f,
        .damage = 1,
        .projectileData = (SpellProjectileData) {
            .speed = 100.0f
        }
    },
    [SPELL_TRIPLE_WAVE] = {
        .name = "Triple Wave",
        .name_jp = "トリプルウェーブ",
        .description = "Unleashes three waves of projectiles",
        .description_jp = "3つの弾の波を放つ",
        .pickupTextureIndex = TEXTURE_SPELL_TRIPLE_WAVE,
        .spellTextureIndex = TEXTURE_PROJECTILE,
        .spellIconIndex = BITMAP_SPELL_TRIPLE_WAVE_ICON,
        .type = SPELL_TYPE_REPEAT,
        .castTime = 1.0f,
        .cooldown = 6.0f,
        .damage = 1,
        .useInitialCastAngle = false,
        .repeatData = (RepeatData){
            .activationTime = 0.3f,
            .repeatCount = 3
        },
        .projectileData = (SpellProjectileData) {
            .speed = 150.0f
        }
    },
    [SPELL_CONE_OF_ICE] = {
        .name = "Cone of Ice",
        .name_jp = "アイスコーン",
        .description = "Shoot a cone of ice projectiles that slow enemies",
        .description_jp = "敵を遅くする氷の弾の円錐を発射する",
        .pickupTextureIndex = TEXTURE_SPELL_CONE_OF_ICE,
        .spellTextureIndex = TEXTURE_PROJECTILE,
        .spellIconIndex = BITMAP_SPELL_CONE_OF_ICE_ICON,
        .type = SPELL_TYPE_INSTANT,
        .castTime = 0.4f,
        .cooldown = 6.0f,
        .damage = 1,
        .useInitialCastAngle = false,
        .projectileData = (SpellProjectileData) {
            .hitCount = 5,
            .slowTime = 2.0f,
            .slowAmount = 0.8f,
            .speed = 100.0f
        }
    },
    [SPELL_ARC_LIGHTNING] = {
        .name = "Arc Lightning",
        .name_jp = "アークライトニング",
        .description = "Unleashes lightning from your hand that damages enemies directly in front of you",
        .description_jp = "前方の敵に直接ダメージを与える稲妻を手から放つ",
        .pickupTextureIndex = TEXTURE_SPELL_ARC_LIGHTNING,
        .spellIconIndex = BITMAP_SPELL_ARC_LIGHTNING_ICON,
        .type = SPELL_TYPE_REPEAT,
        .cooldown = 6.0f,
        .castTime = 1.5f,
        .damage = 3,
        .repeatData = (RepeatData){
            .activationTime = 0.3f,
            .repeatCount = 5
        }
    },
    [SPELL_EXPLOSIVE_SHOT] = {
        .name = "Explosive Shot",
        .name_jp = "エクスプローシブショット",
        .description = "Shoot a projectile that drops a bomb on impact, dealing damage to nearby enemies",
        .description_jp = "衝撃で爆弾を落とす弾を発射し、周囲の敵にダメージを与える",
        .pickupTextureIndex = TEXTURE_SPELL_EXPLOSIVE_SHOT,
        .spellTextureIndex = TEXTURE_PROJECTILE,
        .spellIconIndex = BITMAP_SPELL_EXPLOSIVE_SHOT_ICON,
        .type = SPELL_TYPE_INSTANT,
        .cooldown = 6.0f,
        .castTime = 0.4f,
        .damage = 2,
        .projectileData = (SpellProjectileData) {
            .speed = 70.0f,
            .hitParticle = PARTICLE_TYPE_BOMB
        }
    },
    [SPELL_BOMBER] = {
        .name = "Bomber",
        .name_jp = "ボンバー",
        .description = "Drop a bomb that explodes after a short delay, dealing damage to nearby enemies",
        .description_jp = "短時間の遅延後に爆発する爆弾を落とし、周囲の敵にダメージを与える",
        .pickupTextureIndex = TEXTURE_SPELL_BOMBER,
        .spellIconIndex = BITMAP_SPELL_BOMBER_ICON,
        .type = SPELL_TYPE_INSTANT,
        .cooldown = 6.0f,
        .castTime = 0.3f,
    }
};

void initDeck(void) {
    for (int i = 0; i < SPELL_DECK_MAX; i++) {
        spellDeck[i] = SPELL_NONE;
    }

    spellDeck[0] = SPELL_FIREBALL;
    spellDeck[1] = SPELL_LASER;
    spellDeck[2] = SPELL_NONE;
    spellDeck[3] = SPELL_NONE;
    spellDeck[4] = SPELL_NONE;
}

void initSpells(void) {
    for (int i = 0; i < SPELL_DECK_MAX; i++) {
        spellCastData[i] = (SpellCastData) {
            .spell = SPELL_NONE,
            .castTime = 0.0f,
            .activationTime = 0.0f,
            .activationCount = 0,
            .initialCastAngle = 0.0f
        };
    };

    for (int i = 0; i < SPELL_DECK_MAX; i++) {
        spellCooldowns[i] = 0.0f;
    }

    utilitySpellCount = UTILITY_SPELL_COUNT;
    utilitySpellCharge = 0.0f;
}

Spell *getSpellDeck(void) {
    return spellDeck;
}

bool spellInDeck(Spell spell) {
    for (int i = 0; i < SPELL_DECK_MAX; i++) {
        if (spellDeck[i] == SPELL_NONE) {
            continue;
        }
        if (spellDeck[i] == spell) {
            return true;
        }
    }
    return false;
}

void addSpellToDeck(Spell spell) {
    for (int i = 0; i < SPELL_DECK_MAX; i++) {
        if (spellDeck[i] == SPELL_NONE) {
            spellDeck[i] = spell;
            break;
        }
    }
}

int getSpellPickupTextureIndex(Spell spell) {
    return spellData[spell].pickupTextureIndex;
}

LCDBitmap* getSpellIcon(Spell spell) {
    return BITMAPS[spellData[spell].spellIconIndex];
}

Spell getDeckSpell(int index) {
    return spellDeck[index];
}

float getSpellCooldownPercentage(int index) {
    if (spellCooldowns[index] <= 0.0f) {
        return 0.0f;
    }

    SpellData data = spellData[spellDeck[index]];
    return spellCooldowns[index] / data.cooldown;
}

float getPlayerSpeedModifier(void) {
    float totalSpeedModifier = 1.0f;
    for (int i = 0; i < SPELL_DECK_MAX; i++) {
        SpellCastData *castData = &spellCastData[i];
        Spell spell = castData->spell;
        if (spell == SPELL_NONE) {
            continue;
        }
        SpellData *data = &spellData[spell];
        float speedModifier = data->playerSpeedModifier;
        if (speedModifier != 0) {
            totalSpeedModifier *= 1.0f + speedModifier;
        }
    }
    return totalSpeedModifier;
}

int getMaxUtilitySpells(void) {
    return UTILITY_SPELL_COUNT;
}

int getUtilitySpellCount(void) {
    return utilitySpellCount;
}

float getUtilitySpellCharge(void) {
    return utilitySpellCharge;
}

void castUtilitySpell(void) {
    if (utilitySpellCount > 0) {
        utilitySpellCount -= 1;
        playerDash();
    }
}

void fireSpellProjectile(Spell spell, float angle) {
    SpellData data = spellData[spell];
    float projectileX = playerX + cosf(angle + FIRE_ANGLE_OFFSET) * FIRE_POSITION_OFFSET;
    float projectileY = playerY + sinf(angle + FIRE_ANGLE_OFFSET) * FIRE_POSITION_OFFSET;
    firePlayerProjectile(projectileX, projectileY, cosf(angle), sinf(angle), data.damage, data.projectileData);
}

void activateSpellEffect(int index, float angle) {
    Spell spell = spellDeck[index];
    SpellCastData *castData = &spellCastData[index];
    if (spell == SPELL_FIREBALL) {
        fireSpellProjectile(spell, angle);
    } else if (spell == SPELL_TRIPLE_SHOT) {
        fireSpellProjectile(spell, angle);
    } else if (spell == SPELL_SHOTGUN) {
        float offsetAngle = PI / 16.0f;
        fireSpellProjectile(spell, angle);
        fireSpellProjectile(spell, angle + offsetAngle);
        fireSpellProjectile(spell, angle - offsetAngle);
    } else if (spell == SPELL_LASER) {
        fireSpellProjectile(spell, angle);
    } else if (spell == SPELL_ROOT_SURGE) {
        float offsetX = cosf(angle) * (castData->activationCount % 3 + 1) * CELL_SIZE;
        float offsetY = sinf(angle) * (castData->activationCount % 3 + 1) * CELL_SIZE;
        spawnParticle(PARTICLE_TYPE_PLAYER_ROOT_ATTACK, playerX + offsetX, playerY + offsetY, PARTICLE_TYPE_NONE, false /* enemyParticle */);
    } else if (spell == SPELL_WAVE) {
        float offsetAngle = PI / 16.0f;
        fireSpellProjectile(spell, angle);
        fireSpellProjectile(spell, angle + offsetAngle);
        fireSpellProjectile(spell, angle + offsetAngle * 2);
        fireSpellProjectile(spell, angle - offsetAngle);
        fireSpellProjectile(spell, angle - offsetAngle * 2);
    } else if (spell == SPELL_TRIPLE_WAVE) {
        float offsetAngle = PI / 16.0f;
        fireSpellProjectile(spell, angle);
        fireSpellProjectile(spell, angle + offsetAngle);
        fireSpellProjectile(spell, angle + offsetAngle * 2);
        fireSpellProjectile(spell, angle - offsetAngle);
        fireSpellProjectile(spell, angle - offsetAngle * 2);
    } else if (spell == SPELL_CONE_OF_ICE) {
        float offsetAngle = PI / 32.0f;
        fireSpellProjectile(spell, angle);
        fireSpellProjectile(spell, angle + offsetAngle);
        fireSpellProjectile(spell, angle - offsetAngle);
    } else if (spell == SPELL_ARC_LIGHTNING) {
        float lightningX = playerX + cosf(angle) * FIRE_POSITION_OFFSET;
        float lightningY = playerY + sinf(angle) * FIRE_POSITION_OFFSET;
        int damageRadius = CELL_SIZE;
        SpellData *data = &spellData[spell];
        int damage = data->damage;
        float knockbackTime = data->knockbackTime;
        float knockbackAmount = data->knockbackAmount;
        damageEnemiesInArea(lightningX, lightningY, damageRadius * damageRadius, damage, knockbackTime, knockbackAmount);
    } else if (spell == SPELL_EXPLOSIVE_SHOT) {
        fireSpellProjectile(spell, angle);
    } else if (spell == SPELL_BOMBER) {
        float offsetX = cosf(angle) * CELL_SIZE / 2;
        float offsetY = sinf(angle) * CELL_SIZE / 2;
        spawnParticle(PARTICLE_TYPE_BOMB, playerX + offsetX, playerY + offsetY, PARTICLE_TYPE_NONE, false /* enemyParticle */);
    }

    castData->activationCount++;
}

int castSpell(int index) {
    if (spellCooldowns[index] > 0.0f) {
        return 0;
    }

    Spell spell = spellDeck[index];
    if (spell == SPELL_NONE) {
        return 0;
    }

    SpellData *data = &spellData[spell];
    spellCooldowns[index] = data->cooldown;
    if (data->type == SPELL_TYPE_REPEAT) {
        spellCastData[index] = (SpellCastData) {
            .spell = spell,
            .castTime = data->castTime,
            .activationTime = data->repeatData.activationTime,
            .activationCount = 0,
            .initialCastAngle = playerAngle
        };
    } else {
        spellCastData[index].spell = SPELL_NONE;
    }

    playSound(SFX_SHOOT);
    if (data->type == SPELL_TYPE_INSTANT) {
        activateSpellEffect(index, playerAngle);
    } else if (data->type == SPELL_TYPE_REPEAT) {
        activateSpellEffect(index, playerAngle);
    }

    if (spell == SPELL_ARC_LIGHTNING) {
        setSpellOverlay(index, BITMAP_TABLE_ARC_LIGHTNING_OVERLAY, 0.2f /* frameTime */, 106 /* x */, 52 /* y */);
    }

    return 1;
}

void updateSpells(float deltaTime) {
    if (utilitySpellCount < UTILITY_SPELL_COUNT) {
        utilitySpellCharge += PLAYER_STATS.utilitySpellChargeRate * deltaTime;
        if (utilitySpellCharge > 1.0f) {
            utilitySpellCharge = 0.0f;
            utilitySpellCount++;
        }
    }

    for (int i = 0; i < SPELL_DECK_MAX; i++) {
        if (spellCooldowns[i] > 0.0f) {
            spellCooldowns[i] -= deltaTime;
            if (spellCooldowns[i] < 0.0f) {
                spellCooldowns[i] = 0.0f;
            }
        }

        SpellCastData *castData = &spellCastData[i];
        if (castData->spell == SPELL_NONE) {
            continue;
        }

        if (castData->castTime > 0.0f) {
            castData->castTime -= deltaTime;
            if (castData->castTime <= 0.0f) {
                castData->castTime = 0.0f;
                castData->spell = SPELL_NONE;
                clearSpellOverlay(i);
                continue;   
            }
        }

        SpellData *data = &spellData[castData->spell];
        if (data->type == SPELL_TYPE_REPEAT) {
            castData->activationTime -= deltaTime;
            if (castData->activationTime <= 0.0f && castData->activationCount < data->repeatData.repeatCount) {
                float angle = data->useInitialCastAngle ? castData->initialCastAngle : playerAngle;
                activateSpellEffect(i, angle);
                castData->activationTime = data->repeatData.activationTime;
            }
        }
    }    
}