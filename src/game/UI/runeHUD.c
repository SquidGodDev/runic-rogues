#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "game/UI/runeHUD.h"
#include "game/systems/runeManager.h"
#include "game/rendering/textures.h"
#include "game/player/player.h"
#include "game/player/playerStats.h"
#include "game/player/spells/spells.h"

#define RUNE_HUD_START_X   3
#define RUNE_HUD_START_Y   14
#define RUNE_HUD_ICON_SIZE 9
#define RUNE_HUD_ICON_INSET 2
#define RUNE_HUD_STRIDE    13
#define RUNE_HUD_PER_ROW   5

static void slotPosition(int slotIndex, int *outX, int *outY) {
    int col = slotIndex % RUNE_HUD_PER_ROW;
    int row = slotIndex / RUNE_HUD_PER_ROW;
    *outX = RUNE_HUD_START_X + col * RUNE_HUD_STRIDE;
    *outY = RUNE_HUD_START_Y + row * RUNE_HUD_STRIDE;
}

static void drawSlotBorder(int slotX, int slotY) {
    pd->graphics->drawBitmap(BITMAPS[BITMAP_RUNE_HUD_SLOT], slotX, slotY, kBitmapUnflipped);
}

// A plain icon that is either present or not (e.g. Viking while active).
static void drawStaticSlot(int slotIndex, int iconIndex) {
    int slotX, slotY;
    slotPosition(slotIndex, &slotX, &slotY);
    drawSlotBorder(slotX, slotY);
    pd->graphics->drawBitmap(BITMAPS[iconIndex], slotX + RUNE_HUD_ICON_INSET, slotY + RUNE_HUD_ICON_INSET, kBitmapUnflipped);
}

// A trigger-timer icon that "fills up" from the bottom as it counts toward its
// next trigger. The base icon is drawn normally, then the bottom `progress`
// fraction is redrawn inverted so the fill rises as progress goes 0 -> 1.
static void drawFillSlot(int slotIndex, int iconIndex, float progress) {
    int slotX, slotY;
    slotPosition(slotIndex, &slotX, &slotY);
    drawSlotBorder(slotX, slotY);

    int iconX = slotX + RUNE_HUD_ICON_INSET;
    int iconY = slotY + RUNE_HUD_ICON_INSET;
    pd->graphics->drawBitmap(BITMAPS[iconIndex], iconX, iconY, kBitmapUnflipped);

    if (progress <= 0.0f) {
        return;
    }
    if (progress > 1.0f) {
        progress = 1.0f;
    }
    int fillHeight = (int)(progress * RUNE_HUD_ICON_SIZE + 0.5f);
    if (fillHeight <= 0) {
        return;
    }

    int clipY = iconY + (RUNE_HUD_ICON_SIZE - fillHeight);
    pd->graphics->setClipRect(iconX, clipY, RUNE_HUD_ICON_SIZE, fillHeight);
    pd->graphics->setDrawMode(kDrawModeInverted);
    pd->graphics->drawBitmap(BITMAPS[iconIndex], iconX, iconY, kBitmapUnflipped);
    pd->graphics->setDrawMode(kDrawModeCopy);
    pd->graphics->clearClipRect();
}

static bool vikingActive(void) {
    return playerHealth < getPlayerMaxHealth() * 0.5f;
}

// Runes whose HUD icon fills as an effect ramps up. Returns the fill fraction
// (0..1) via outProgress, or false if this rune has no permanent fill entry.
static bool runeHudFillProgress(RuneType type, float *outProgress) {
    if (getRuneTriggerType(type) == RUNE_TRIGGER_TIME) {
        *outProgress = getRuneTriggerProgress(type);
        return true;
    }
    if (type == RUNE_TYPE_CRESCENDO) {
        *outProgress = getCrescendoProgress();
        return true;
    }
    if (type == RUNE_TYPE_RAPIER) {
        *outProgress = getRapierProgress();
        return true;
    }
    if (type == RUNE_TYPE_BOMBER) {
        *outProgress = getRuneTriggerProgress(type);
        return true;
    }
    if (type == RUNE_TYPE_BOMB_ARROW) {
        *outProgress = getBombArrowProgress();
        return true;
    }
    if (type == RUNE_TYPE_BLACK_HOLE) {
        *outProgress = getBlackHoleProgress();
        return true;
    }
    if (type == RUNE_TYPE_ENCHANTED_AMULET) {
        *outProgress = getSpellRepeatProgress(BASIC_SPELL_INDEX);
        return true;
    }
    if (type == RUNE_TYPE_ARCANE_STAFF) {
        *outProgress = getSpellRepeatProgress(SPECIAL_SPELL_INDEX);
        return true;
    }
    return false;
}

void drawRuneHUD(void) {
    int obtainedCount = getObtainedRuneCount();
    int slotIndex = 0;

    pd->graphics->pushContext(NULL);

    // Permanent fill entries (trigger timers, ramping buffs), in obtain order.
    for (int i = 0; i < obtainedCount; i++) {
        RuneType type = getObtainedRune(i);
        float progress;
        if (!runeHudFillProgress(type, &progress)) {
            continue;
        }
        drawFillSlot(slotIndex, getRuneHudIconIndex(type), progress);
        slotIndex++;
    }

    // Conditional entries appended as they become active.
    for (int i = 0; i < obtainedCount; i++) {
        RuneType type = getObtainedRune(i);
        if (type == RUNE_TYPE_VIKING && vikingActive()) {
            drawStaticSlot(slotIndex, getRuneHudIconIndex(type));
            slotIndex++;
        } else if (type == RUNE_TYPE_FOCUS && !playerIsMoving()) {
            drawStaticSlot(slotIndex, getRuneHudIconIndex(type));
            slotIndex++;
        } else if (type == RUNE_TYPE_BULWARK && bulwarkIsAvailable()) {
            drawStaticSlot(slotIndex, getRuneHudIconIndex(type));
            slotIndex++;
        } else if (type == RUNE_TYPE_STONE_GOLEM && !playerIsMoving()) {
            drawStaticSlot(slotIndex, getRuneHudIconIndex(type));
            slotIndex++;
        } else if (type == RUNE_TYPE_ADRENALINE && adrenalineIsActive()) {
            drawStaticSlot(slotIndex, getRuneHudIconIndex(type));
            slotIndex++;
        }
    }

    pd->graphics->popContext();
}
