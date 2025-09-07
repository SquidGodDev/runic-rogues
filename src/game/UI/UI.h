#pragma once

typedef enum ScreenShakeStrength {
    SCREEN_SHAKE_LIGHT,
    SCREEN_SHAKE_MEDIUM,
    SCREEN_SHAKE_STRONG
} ScreenShakeStrength;

void initUI(void);
void drawUI(float deltaTime);
void showRoomClearedPopup(void);
bool adjustShopDescOffset(int delta);
void resetShopDescOffset(void);
void setSpellOverlay(int index, int bitmapTableIndex, float frameTime, int drawX, int drawY);
void clearSpellOverlay(int index);
void setExitArrowPosition(int worldX, int worldY);
void screenShake(ScreenShakeStrength strength);