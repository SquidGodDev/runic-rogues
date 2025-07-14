#pragma once

typedef enum ScreenShakeStrength {
    SCREEN_SHAKE_LIGHT,
    SCREEN_SHAKE_MEDIUM,
    SCREEN_SHAKE_STRONG
} ScreenShakeStrength;

void initUI(int isBossRoom);
void drawUI(float deltaTime);
void setSpellOverlay(int bitmapTableIndex, float frameTime, int drawX, int drawY);
void clearSpellOverlay(void);
void setExitArrowPosition(int worldX, int worldY);
void screenShake(ScreenShakeStrength strength);