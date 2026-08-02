#define TARGET_EXTENSION 1
#include "pd_api.h"

#include "globals/globals.h"
#include "globals/saveSystem.h"
#include "game/settings/settings.h"
#include "game/enemies/enemies.h"
#include "game/systems/difficultyManager.h"

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
    } else if (strcmp(key, "controlScheme") == 0) {
        int scheme = parseInt(value);
        if (scheme < 0 || scheme >= CONTROL_SCHEME_COUNT) {
            scheme = CONTROL_SCHEME_CRANK;
        }
        SETTINGS.controlScheme = (ControlScheme)scheme;
    } else if (strcmp(key, "drawMinimapWalls") == 0) {
        SETTINGS.drawMinimapWalls = parseInt(value) != 0;
    } else if (strcmp(key, "spellButtonsReversed") == 0) {
        SETTINGS.spellButtonsReversed = parseInt(value) != 0;
    } else if (strcmp(key, "toggleFireBasic") == 0) {
        SETTINGS.toggleFireBasic = parseInt(value) != 0;
    } else if (strcmp(key, "toggleFireSpecial") == 0) {
        SETTINGS.toggleFireSpecial = parseInt(value) != 0;
    } else if (strcmp(key, "drawFPS") == 0) {
        SETTINGS.drawFPS = parseInt(value) != 0;
    } else if (strcmp(key, "showTimer") == 0) {
        SETTINGS.showTimer = parseInt(value) != 0;
    } else if (strcmp(key, "targetFps") == 0) {
        int fps = parseInt(value);
        if (fps < 0 || fps >= FPS_SETTING_COUNT) {
            fps = FPS_SETTING_40;
        }
        SETTINGS.targetFps = fps;
    } else if (strcmp(key, "crankTurnSpeedInt") == 0) {
        int speed = parseInt(value);
        if (speed < SETTING_CRANK_TURN_SPEED_MIN) {
            speed = SETTING_CRANK_TURN_SPEED_MIN;
        } else if (speed > SETTING_CRANK_TURN_SPEED_MAX) {
            speed = SETTING_CRANK_TURN_SPEED_MAX;
        }
        SETTINGS.crankTurnSpeed = speed;
    } else if (strcmp(key, "dpadTurnSpeed") == 0) {
        SETTINGS.dpadTurnSpeed = parseBarLevel(value);
    } else if (strcmp(key, "dpadLeftRight") == 0) {
        int mode = parseInt(value);
        if (mode < 0 || mode >= DPAD_LR_MODE_COUNT) {
            mode = DPAD_LR_STRAFE;
        }
        SETTINGS.dpadLeftRight = mode;
    } else if (strcmp(key, "dpadStrafe") == 0) {
        // Legacy save: bool where 1 = strafe, 0 = turn.
        SETTINGS.dpadLeftRight = parseInt(value) != 0 ? DPAD_LR_STRAFE : DPAD_LR_TURN;
    } else if (strcmp(key, "absoluteCrank") == 0) {
        SETTINGS.absoluteCrank = parseInt(value) != 0;
    } else if (strcmp(key, "absoluteCrankOffset") == 0) {
        int offset = parseInt(value);
        if (offset < SETTING_CRANK_OFFSET_MIN) {
            offset = SETTING_CRANK_OFFSET_MIN;
        } else if (offset > SETTING_CRANK_OFFSET_MAX) {
            offset = SETTING_CRANK_OFFSET_MAX;
        }
        SETTINGS.absoluteCrankOffset = offset;
    } else if (strcmp(key, "crankRatio") == 0) {
        int ratio = parseInt(value);
        if (ratio < 0 || ratio >= CRANK_RATIO_COUNT) {
            ratio = CRANK_RATIO_1X;
        }
        SETTINGS.crankRatio = ratio;
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
    } else if (strcmp(key, "debugEnabled") == 0) {
        DEBUG.enabled = parseInt(value) != 0;
    } else if (strcmp(key, "debugSkipToRoom") == 0) {
        int room = parseInt(value);
        if (room < 0) {
            room = 0;
        } else if (room > DEBUG_SKIP_TO_ROOM_MAX) {
            room = DEBUG_SKIP_TO_ROOM_MAX;
        }
        DEBUG.skipToRoom = room;
    } else if (strcmp(key, "debugAreaOverride") == 0) {
        int area = parseInt(value);
        if (area < 0) {
            area = 0;
        } else if (area > DEBUG_AREA_OVERRIDE_MAX) {
            area = DEBUG_AREA_OVERRIDE_MAX;
        }
        DEBUG.areaOverride = area;
    } else if (strcmp(key, "debugInstantRoomClear") == 0) {
        DEBUG.instantRoomClear = parseInt(value) != 0;
    } else if (strcmp(key, "debugInstantKillBoss") == 0) {
        DEBUG.instantKillBoss = parseInt(value) != 0;
    } else if (strcmp(key, "debugOneHpBosses") == 0) {
        DEBUG.oneHpBosses = parseInt(value) != 0;
    } else if (strcmp(key, "debugStartInTestArea") == 0) {
        DEBUG.startInTestArea = parseInt(value) != 0;
    } else if (strcmp(key, "debugTestFirstRoomEnemy") == 0) {
        int enemy = parseInt(value);
        if (enemy < 0) {
            enemy = 0;
        } else if (enemy > ENEMY_TYPE_COUNT) {
            enemy = ENEMY_TYPE_COUNT;
        }
        DEBUG.testFirstRoomEnemy = enemy;
    } else if (strcmp(key, "debugSkipAreaMap") == 0) {
        DEBUG.skipAreaMap = parseInt(value) != 0;
    } else if (strcmp(key, "classDifficulty0") == 0) {
        setClassUnlockedDifficulty((ClassType)0, parseInt(value));
    } else if (strcmp(key, "classDifficulty1") == 0) {
        setClassUnlockedDifficulty((ClassType)1, parseInt(value));
    } else if (strcmp(key, "classDifficulty2") == 0) {
        setClassUnlockedDifficulty((ClassType)2, parseInt(value));
    } else if (strcmp(key, "classDifficulty3") == 0) {
        setClassUnlockedDifficulty((ClassType)3, parseInt(value));
    } else if (strcmp(key, "classLastDifficulty0") == 0) {
        setClassLastSelectedDifficulty((ClassType)0, parseInt(value));
    } else if (strcmp(key, "classLastDifficulty1") == 0) {
        setClassLastSelectedDifficulty((ClassType)1, parseInt(value));
    } else if (strcmp(key, "classLastDifficulty2") == 0) {
        setClassLastSelectedDifficulty((ClassType)2, parseInt(value));
    } else if (strcmp(key, "classLastDifficulty3") == 0) {
        setClassLastSelectedDifficulty((ClassType)3, parseInt(value));
    } else if (strcmp(key, "classUnlocked1") == 0) {
        setClassUnlocked((ClassType)1, parseInt(value) != 0);
    } else if (strcmp(key, "classUnlocked2") == 0) {
        setClassUnlocked((ClassType)2, parseInt(value) != 0);
    } else if (strcmp(key, "classUnlocked3") == 0) {
        setClassUnlocked((ClassType)3, parseInt(value) != 0);
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
        "controlScheme=%d\n"
        "drawMinimapWalls=%d\n"
        "spellButtonsReversed=%d\n"
        "toggleFireBasic=%d\n"
        "toggleFireSpecial=%d\n"
        "drawFPS=%d\n"
        "showTimer=%d\n"
        "targetFps=%d\n"
        "crankTurnSpeedInt=%d\n"
        "dpadTurnSpeed=%d\n"
        "dpadLeftRight=%d\n"
        "absoluteCrank=%d\n"
        "absoluteCrankOffset=%d\n"
        "crankRatio=%d\n"
        "invertCrank=%d\n"
        "invertDpad=%d\n"
        "musicVolume=%d\n"
        "sfxVolume=%d\n"
        "tutorialCompleted=%d\n"
        "debugEnabled=%d\n"
        "debugSkipToRoom=%d\n"
        "debugAreaOverride=%d\n"
        "debugInstantRoomClear=%d\n"
        "debugInstantKillBoss=%d\n"
        "debugOneHpBosses=%d\n"
        "debugStartInTestArea=%d\n"
        "debugTestFirstRoomEnemy=%d\n"
        "debugSkipAreaMap=%d\n"
        "classDifficulty0=%d\n"
        "classDifficulty1=%d\n"
        "classDifficulty2=%d\n"
        "classDifficulty3=%d\n"
        "classLastDifficulty0=%d\n"
        "classLastDifficulty1=%d\n"
        "classLastDifficulty2=%d\n"
        "classLastDifficulty3=%d\n"
        "classUnlocked1=%d\n"
        "classUnlocked2=%d\n"
        "classUnlocked3=%d\n",
        SAVE_VERSION,
        (int)SETTINGS.language,
        (int)SETTINGS.controlScheme,
        SETTINGS.drawMinimapWalls ? 1 : 0,
        SETTINGS.spellButtonsReversed ? 1 : 0,
        SETTINGS.toggleFireBasic ? 1 : 0,
        SETTINGS.toggleFireSpecial ? 1 : 0,
        SETTINGS.drawFPS ? 1 : 0,
        SETTINGS.showTimer ? 1 : 0,
        SETTINGS.targetFps,
        SETTINGS.crankTurnSpeed,
        SETTINGS.dpadTurnSpeed,
        SETTINGS.dpadLeftRight,
        SETTINGS.absoluteCrank ? 1 : 0,
        SETTINGS.absoluteCrankOffset,
        SETTINGS.crankRatio,
        SETTINGS.invertCrank ? 1 : 0,
        SETTINGS.invertDpad ? 1 : 0,
        SETTINGS.musicVolume,
        SETTINGS.sfxVolume,
        tutorialCompleted ? 1 : 0,
        DEBUG.enabled ? 1 : 0,
        DEBUG.skipToRoom,
        DEBUG.areaOverride,
        DEBUG.instantRoomClear ? 1 : 0,
        DEBUG.instantKillBoss ? 1 : 0,
        DEBUG.oneHpBosses ? 1 : 0,
        DEBUG.startInTestArea ? 1 : 0,
        DEBUG.testFirstRoomEnemy,
        DEBUG.skipAreaMap ? 1 : 0,
        getClassUnlockedDifficulty((ClassType)0),
        getClassUnlockedDifficulty((ClassType)1),
        getClassUnlockedDifficulty((ClassType)2),
        getClassUnlockedDifficulty((ClassType)3),
        getClassLastSelectedDifficulty((ClassType)0),
        getClassLastSelectedDifficulty((ClassType)1),
        getClassLastSelectedDifficulty((ClassType)2),
        getClassLastSelectedDifficulty((ClassType)3),
        getClassUnlockedStored((ClassType)1) ? 1 : 0,
        getClassUnlockedStored((ClassType)2) ? 1 : 0,
        getClassUnlockedStored((ClassType)3) ? 1 : 0
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
