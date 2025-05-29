#pragma once

typedef enum Language {
    LANGUAGE_EN,
    LANGUAGE_JP
} Language;

typedef struct Settings {
    Language language;
} Settings;

extern Settings SETTINGS;

void initSettings(void);