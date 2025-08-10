#pragma once

typedef enum ScreenShakeStrength {
    SCREEN_SHAKE_LIGHT,
    SCREEN_SHAKE_MEDIUM,
    SCREEN_SHAKE_STRONG
} ScreenShakeStrength;

void initUI(int isBossRoom);
void drawUI(float deltaTime);
void showRoomClearedPopup(void);
void setSpellOverlay(int index, int bitmapTableIndex, float frameTime, int drawX, int drawY);
void clearSpellOverlay(int index);
void setExitArrowPosition(int worldX, int worldY);
void screenShake(ScreenShakeStrength strength);