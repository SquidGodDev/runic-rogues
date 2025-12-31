#pragma once
#include "pd_api.h"

#define MAX_TIMERS 128

typedef void (*TimerCallback)(int repeatCount, void *data);

typedef struct Timer {
    bool active;
    float duration;
    int repeatCount;
    float remaining;
    TimerCallback callback;
    void *data;
} Timer;

void initTimers(void);
void clearTimers(void);
void updateTimers(float deltaTime);
void addTimer(float duration, int repeatCount, TimerCallback callback, void *data);