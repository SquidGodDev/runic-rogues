#pragma once

typedef enum Language {
    LANGUAGE_EN,
    LANGUAGE_JP,
    LANGUAGE_COUNT
} Language;

typedef struct Settings {
    Language language;
    bool drawMinimapWalls;
    bool spellButtonsReversed;
    bool drawFPS;
} Settings;

extern Settings SETTINGS;

void initSettings(void);