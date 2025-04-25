/*
 * File:   sys_timer.h
 * Author: msuzer
 *
 * Created on October 7, 2020, 10:14 PM
 */

#ifndef SYS_TIMER_H
#define	SYS_TIMER_H

#include <stdint.h>

    enum {
        SYS_TMR0 = 0,
        SYS_TMR1,
        SYS_TMR2,
        SYS_TMR3,
        SYS_TMR4,
        SYS_TMR5,
        SYS_TMR6,
        SYS_TMR7,
        SYS_TMR8,
        SYS_TMR9
    };

    uint32_t GetSysTick(void);
    void SYS_TIMER_Periodic_Tasks(void);
    void SYS_TIMER_Initialize(void);
    void SYS_TIMER_Main_Tasks(void);
    void SYS_TIMER_CancelTimer(uint8_t idx);
    bool SYS_TIMER_SetupTimer(uint8_t, uint16_t);
    void SYS_TIMER_SetOnTimerOverFlowEventHandler(void (* hnd)(uint8_t));

#endif	/* SYS_TIMER_H */

