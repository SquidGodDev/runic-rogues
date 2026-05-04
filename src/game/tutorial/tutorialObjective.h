#pragma once

#include "stdbool.h"

void setTutorialObjective(const char *title, int current, int max);
void setTutorialObjectiveSimple(const char *title);
void clearTutorialObjective(void);
void drawTutorialObjective(void);
bool isTutorialObjectiveActive(void);
