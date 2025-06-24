#pragma once
#include <Arduino.h>

// Tick resolution in milliseconds (must match hardware timer interval)
#define CLOCK_TICK_RESOLUTION       20
#define FR_To_Ticks(FREQUENCY)      (1000 / CLOCK_TICK_RESOLUTION / FREQUENCY)
#define MS_To_Ticks(TIME_IN_MS)     (TIME_IN_MS / CLOCK_TICK_RESOLUTION)
#define SEC_To_Ticks(TIME_IN_SEC)   (TIME_IN_SEC * (1000 / CLOCK_TICK_RESOLUTION))
#define MIN_To_Ticks(TIME_IN_MIN)   (TIME_IN_MIN * ((60 * 1000) / CLOCK_TICK_RESOLUTION))

// Define timers for state machine steps
enum TimerID {
    TMR_DEVICE_STATUS,
    TMR_MOVE_TO_MARK,
    TMR_MEASUREMENT,
    TMR_ABORT_WAIT,
    TMR_MOTOR_HOME,
    TMR_CHECK_CONNECTION,
    TMR_COUNT
};

// Timer slot struct
struct AsyncTimer {
    volatile bool active;
    volatile uint32_t ticks;
};

// Global timer array
extern volatile AsyncTimer timers[TMR_COUNT];

namespace Timer {
    void IRAM_ATTR tick();  // Call this from ISR (onSysTick)

    void start(TimerID id, uint32_t ms);       // Start a timer
    void stop(TimerID id);                     // Stop a timer
    void restart(TimerID id, uint32_t ms);     // Restart with new value
    bool expired(TimerID id);                  // Has the timer expired?
    bool isActive(TimerID id);                 // Is the timer still active?
}
