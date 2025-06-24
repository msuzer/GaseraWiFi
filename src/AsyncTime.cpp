#include "AsyncTimer.h"

// Timer slot storage
volatile AsyncTimer timers[TMR_COUNT];

// Called every CLOCK_TICK_RESOLUTION ms from ISR
void IRAM_ATTR Timer::tick() {
    for (int i = 0; i < TMR_COUNT; ++i) {
        if (timers[i].active && timers[i].ticks > 0) {
            timers[i].ticks--;
        }
    }
}

void Timer::start(TimerID id, uint32_t ms) {
    timers[id].ticks = ms / CLOCK_TICK_RESOLUTION;
    timers[id].active = true;
}

void Timer::stop(TimerID id) {
    timers[id].active = false;
}

void Timer::restart(TimerID id, uint32_t ms) {
    start(id, ms);
}

bool Timer::expired(TimerID id) {
    return timers[id].active && timers[id].ticks == 0;
}

bool Timer::isActive(TimerID id) {
    return timers[id].active;
}
