#pragma once

typedef enum ScreenShakeStrength {
    SCREEN_SHAKE_LIGHT,
    SCREEN_SHAKE_MEDIUM,
    SCREEN_SHAKE_STRONG
} ScreenShakeStrength;

void initUI(void);
void drawUI(float deltaTime);
void screenShake(ScreenShakeStrength strength);