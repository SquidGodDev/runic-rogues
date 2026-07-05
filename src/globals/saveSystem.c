#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/saveSystem.h"
#include "game/settings/settings.h"

#define SAVE_PATH "save.txt"
#define SAVE_VERSION 1
#define SAVE_BUF_SIZE 1024

static bool tutorialCompleted = false;

bool isTutorialCompleted(void) {
    return tutorialCompleted;
}

void setTutorialCompleted(bool value) {
    tutorialCompleted = value;
    saveGame();
}

static int parseInt(const char *s) {
    int value = 0;
    int sign = 1;
    if (*s == '-') {
        sign = -1;
        s++;
    }
    while (*s >= '0' && *s <= '9') {
        value = value * 10 + (*s - '0');
        s++;
    }
    return sign * value;
}

static int parseBarLevel(const char *s) {
    int level = parseInt(s);
    if (level < 0) {
        level = 0;
    } else if (level > SETTING_BAR_MAX_LEVEL) {
        level = SETTING_BAR_MAX_LEVEL;
    }
    return level;
}

static void applyKeyValue(const char *key, const char *value) {
    if (strcmp(key, "version") == 0) {
        return;
    }
    if (strcmp(key, "language") == 0) {
        SETTINGS.language = (Language)parseInt(value);
    } else if (strcmp(key, "drawMinimapWalls") == 0) {
        SETTINGS.drawMinimapWalls = parseInt(value) != 0;
    } else if (strcmp(key, "spellButtonsReversed") == 0) {
        SETTINGS.spellButtonsReversed = parseInt(value) != 0;
    } else if (strcmp(key, "drawFPS") == 0) {
        SETTINGS.drawFPS = parseInt(value) != 0;
    } else if (strcmp(key, "crankTurnSpeed") == 0) {
        SETTINGS.crankTurnSpeed = parseBarLevel(value);
    } else if (strcmp(key, "dpadTurnSpeed") == 0) {
        SETTINGS.dpadTurnSpeed = parseBarLevel(value);
    } else if (strcmp(key, "dpadStrafe") == 0) {
        SETTINGS.dpadStrafe = parseInt(value) != 0;
    } else if (strcmp(key, "absoluteCrank") == 0) {
        SETTINGS.absoluteCrank = parseInt(value) != 0;
    } else if (strcmp(key, "invertCrank") == 0) {
        SETTINGS.invertCrank = parseInt(value) != 0;
    } else if (strcmp(key, "invertDpad") == 0) {
        SETTINGS.invertDpad = parseInt(value) != 0;
    } else if (strcmp(key, "musicVolume") == 0) {
        SETTINGS.musicVolume = parseBarLevel(value);
    } else if (strcmp(key, "sfxVolume") == 0) {
        SETTINGS.sfxVolume = parseBarLevel(value);
    } else if (strcmp(key, "tutorialCompleted") == 0) {
        tutorialCompleted = parseInt(value) != 0;
    }
}

static void parseBuffer(char *buf, int len) {
    int lineStart = 0;
    for (int i = 0; i <= len; i++) {
        if (i == len || buf[i] == '\n' || buf[i] == '\r') {
            buf[i] = '\0';
            char *line = buf + lineStart;
            if (*line != '\0') {
                char *eq = strchr(line, '=');
                if (eq != NULL && eq != line) {
                    *eq = '\0';
                    applyKeyValue(line, eq + 1);
                }
            }
            lineStart = i + 1;
        }
    }
}

void saveGame(void) {
    SDFile *file = pd->file->open(SAVE_PATH, kFileWrite);
    if (file == NULL) {
        pd->system->logToConsole("save: open for write failed: %s", pd->file->geterr());
        return;
    }

    char *body = NULL;
    int len = pd->system->formatString(&body,
        "version=%d\n"
        "language=%d\n"
        "drawMinimapWalls=%d\n"
        "spellButtonsReversed=%d\n"
        "drawFPS=%d\n"
        "crankTurnSpeed=%d\n"
        "dpadTurnSpeed=%d\n"
        "dpadStrafe=%d\n"
        "absoluteCrank=%d\n"
        "invertCrank=%d\n"
        "invertDpad=%d\n"
        "musicVolume=%d\n"
        "sfxVolume=%d\n"
        "tutorialCompleted=%d\n",
        SAVE_VERSION,
        (int)SETTINGS.language,
        SETTINGS.drawMinimapWalls ? 1 : 0,
        SETTINGS.spellButtonsReversed ? 1 : 0,
        SETTINGS.drawFPS ? 1 : 0,
        SETTINGS.crankTurnSpeed,
        SETTINGS.dpadTurnSpeed,
        SETTINGS.dpadStrafe ? 1 : 0,
        SETTINGS.absoluteCrank ? 1 : 0,
        SETTINGS.invertCrank ? 1 : 0,
        SETTINGS.invertDpad ? 1 : 0,
        SETTINGS.musicVolume,
        SETTINGS.sfxVolume,
        tutorialCompleted ? 1 : 0
    );

    if (len > 0 && body != NULL) {
        if (pd->file->write(file, body, len) < 0) {
            pd->system->logToConsole("save: write failed: %s", pd->file->geterr());
        }
    }
    if (body != NULL) {
        pd->system->realloc(body, 0);
    }
    pd->file->close(file);
}

void initSaveSystem(void) {
    SDFile *file = pd->file->open(SAVE_PATH, kFileReadData);
    if (file == NULL) {
        pd->system->logToConsole("save: no save found, creating defaults");
        saveGame();
        return;
    }

    char buf[SAVE_BUF_SIZE];
    int totalRead = 0;
    while (totalRead < SAVE_BUF_SIZE - 1) {
        int n = pd->file->read(file, buf + totalRead, SAVE_BUF_SIZE - 1 - totalRead);
        if (n <= 0) {
            break;
        }
        totalRead += n;
    }
    pd->file->close(file);

    if (totalRead > 0) {
        parseBuffer(buf, totalRead);
    }

    saveGame();
}
