#pragma once

typedef enum Language {
    LANGUAGE_EN,
    LANGUAGE_JP
} Language;

typedef struct Settings {
    Language language;
    bool drawMinimapWalls;
    bool spellButtonsReversed;
} Settings;

extern Settings SETTINGS;

void initSettings(void);