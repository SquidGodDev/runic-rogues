#pragma once

#include "game/UI/popups.h"

typedef enum ScreenShakeStrength {
    SCREEN_SHAKE_LIGHT,
    SCREEN_SHAKE_MEDIUM,
    SCREEN_SHAKE_STRONG
} ScreenShakeStrength;

void initUI(void);
void drawUI(float deltaTime, bool duringCinematic);
void showPopup(PopupType type, PopupPosition position);
void drawPopup(float deltaTime);
bool adjustShopDescOffset(int delta);
void resetShopDescOffset(void);
void setSpellOverlay(int index, int bitmapTableIndex, float frameTime, int drawX, int drawY);
void clearSpellOverlay(int index);
void setArrowPosition(int worldX, int worldY);
void clearArrow(void);
void addDamageIndicator(float sourceX, float sourceY);
void addAttackIntentIndicator(float sourceX, float sourceY, float duration);
void screenShake(ScreenShakeStrength strength);
void updateScreenShake(float deltaTime);
bool isPopupActive(void);