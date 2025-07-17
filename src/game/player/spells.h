#pragma once
#include "pd_api.h"

#include "game/rendering/particles.h"
#include "game/rendering/textures.h"

typedef enum Spell {
    SPELL_NONE,
    SPELL_FIREBALL,
    SPELL_TRIPLE_SHOT,
    SPELL_SHOTGUN,
    SPELL_LASER,
    SPELL_ROOT_SURGE,
    SPELL_WAVE,
    SPELL_TRIPLE_WAVE,
    SPELL_CONE_OF_ICE,
    SPELL_ARC_LIGHTNING,
    SPELL_EXPLOSIVE_SHOT,
    SPELL_BOMBER,
    SPELL_COUNT
} Spell;

typedef enum SpellType {
    SPELL_TYPE_INSTANT,
    SPELL_TYPE_REPEAT,
    SPELL_TYPE_CHANNEL,
    SPELL_TYPE_COUNT
} SpellType;

typedef struct RepeatData {
    int repeatCount;
    float activationTime;
} RepeatData;

typedef struct SpellProjectileData {
    int hitCount;
    float slowTime;
    float slowAmount;
    float speed;
    ParticleType hitParticle;
} SpellProjectileData;

typedef struct SpellData {
    char* name;
    char* name_jp;
    char* description;
    char* description_jp;
    int pickupTextureIndex;
    int spellTextureIndex;
    int spellIconIndex;
    SpellType type;
    float castTime;
    int damage;
    bool useInitialCastAngle;
    float playerSpeedModifier;
    RepeatData repeatData;
    SpellProjectileData projectileData;
} SpellData;

void initDeck(void);
void initSpells(void);
Spell *getSpellDeck(void);
bool spellInDeck(Spell spell);
void addSpellToDeck(Spell spell);
int getSpellPickupTextureIndex(Spell spell);
LCDBitmap* getSpellIcon(Spell spell);
Spell getRandomSpell(void);
Spell heldSpellsFull(void);
Spell getHeldSpell(int index);
int getMaxHeldSpells(void);
float getPlayerSpeedModifier(void);
float getManaPercentage(void);
int getMaxUtilitySpells(void);
int getUtilitySpellCount(void);
void rechargeUtilitySpell(void);
void castUtilitySpell(void);
void addHeldSpell(Spell spell);
int castHeldSpell(void);
void updateSpells(float deltaTime);

static SpellData spellData[SPELL_COUNT] = {
    [SPELL_NONE] = {
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
        .castTime = 0.4f,
        .damage = 4,
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
        .castTime = 0.3f,
    },
};