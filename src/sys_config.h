/*
 * File:   sys_config.h
 * Author: msuzer
 *
 * Created on October 8, 2020, 12:43 AM
 */

#ifndef SYS_CONFIG_H
#define	SYS_CONFIG_H

#define NAME_OF_SSID                "Sitecom05D1AE"
#define PASSWORD_OF_SSID            "UE7KSDBUWHU4"

#define GASERA_DEVICE_IP            "192.168.0.100"
#define GASERA_DEVICE_PORT          8888
#define GASERA_MEASUREMENT_TIME     600

#define STEPPER_HOME_POSITION       0
#define STEPPER_MARK_POSITION       140000

#define STEPPER_DEFAULT_SPEED       400
#define STEPPER_FAST_SPEED          200

/*******************************************/

#define POWER_ON_TIMER_MS           2000
#define CLOCK_TICK_RESOLUTION       20
#define NUMBER_OF_TIMER_OBJECTS     2
#define FR_To_Ticks(FREQUENCY)      (1000 / CLOCK_TICK_RESOLUTION / FREQUENCY)
#define MS_To_Ticks(TIME_IN_MS)     (TIME_IN_MS / CLOCK_TICK_RESOLUTION)
#define SEC_To_Ticks(TIME_IN_SEC)   (TIME_IN_SEC * (1000 / CLOCK_TICK_RESOLUTION))
#define MIN_To_Ticks(TIME_IN_MIN)   (TIME_IN_MIN * ((60 * 1000) / CLOCK_TICK_RESOLUTION))

#define LCD_I2C_DEVICE_ADDRESS				0x27
#define ETHERNET_SHIELD_MAC         { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }

#endif	/* SYS_CONFIG_H */
