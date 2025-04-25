#include "sys_timer.h"
#include <stdlib.h>
#include "sys_config.h"

static uint32_t sys_tick = 0;
static uint32_t timer_objects[NUMBER_OF_TIMER_OBJECTS] = { 0 };

void (*TIMER_OnTimerOverFlowEventHandler)(uint8_t idx) = NULL;

void SYS_TIMER_SetOnTimerOverFlowEventHandler(void (*handler)(uint8_t)) {
  TIMER_OnTimerOverFlowEventHandler = handler;
}

uint32_t GetSysTick(void) {
  return sys_tick;
}

void SYS_TIMER_Periodic_Tasks(void) {
  ++sys_tick;  // It is going to take ~248 days to overflow with 5ms tick period, so no need worry about it.
}

void SYS_TIMER_CancelTimer(uint8_t idx) {
  if (idx < NUMBER_OF_TIMER_OBJECTS) {
    timer_objects[idx] = UINT32_MAX;
  }
}

bool SYS_TIMER_SetupTimer(uint8_t idx, uint16_t ticks) {
  if (idx < NUMBER_OF_TIMER_OBJECTS) {
    if (ticks > 0) {  // if ticks are zero, no need to setup the timer
      timer_objects[idx] = GetSysTick() + ticks;
      return true;
    }
  }

  return false;
}

void SYS_TIMER_Initialize(void) {
  uint8_t i;

  for (i = 0; i < NUMBER_OF_TIMER_OBJECTS; i++) {
    timer_objects[i] = UINT32_MAX;
  }
}

void SYS_TIMER_Main_Tasks(void) {
  uint8_t i;

  for (i = 0; i < NUMBER_OF_TIMER_OBJECTS; i++) {
    if (timer_objects[i] < GetSysTick()) {
      timer_objects[i] = UINT32_MAX;
      if (TIMER_OnTimerOverFlowEventHandler != NULL) {
        TIMER_OnTimerOverFlowEventHandler(i);
      }
    }
  }
}
