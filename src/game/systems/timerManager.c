#include "game/systems/timerManager.h"

Timer timers[MAX_TIMERS];

void initTimers(void) {
    clearTimers();
}

void clearTimers(void) {
    for (int i = 0; i < MAX_TIMERS; i++) {
        timers[i].active = false;
    }
}

void updateTimers(float deltaTime) {
    for (int i = 0; i < MAX_TIMERS; i++) {
        Timer *timer = &timers[i];
        if (!timer->active) {
            continue;
        }

        timer->remaining -= deltaTime;
        if (timer->remaining <= 0.0f) {
            timer->callback(timer->repeatCount, timer->data);
            timer->repeatCount--;
            if (timer->repeatCount <= 0) {
                timer->active = false;
            } else {
                timer->remaining += timer->duration;
            }
        }
    }
}

void addTimer(float duration, int repeatCount, TimerCallback callback, void *data) {
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (!timers[i].active) {
            timers[i].active = true;
            timers[i].duration = duration;
            timers[i].repeatCount = repeatCount;
            timers[i].remaining = duration;
            timers[i].callback = callback;
            timers[i].data = data;
            break;
        }
    }
}