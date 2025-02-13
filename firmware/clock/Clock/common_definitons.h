/*
 * common_definitons.h
 *
 *  Created on: Feb 8, 2025
 *      Author: trwgQ26xxx
 */

#ifndef COMMON_DEFINITONS_H_
#define COMMON_DEFINITONS_H_

#include <stdint.h>
#include "../Core/Inc/main.h"

/* Common definitions */
#define TRUE	1
#define FALSE	0

#define DEC2BCD(in) (uint8_t)(((in / 10) << 4) | (in % 10))
#define BCD2BIN(in) (uint8_t)((((in & 0xF0) >> 4) * 10) + (in & 0x0F))

#define RTC_READ_FREQUENCY				4	//Hz
#define LED_DATA_UPDATE_FREQUENCY		8	//Hz
#define LED_CFG_UPDATE_FREQUENCY		8	//Hz

#define RTC_READ_MODULO					(UPDATE_FREQUENCY / RTC_READ_FREQUENCY)
#define RTC_READ_OFFSET					0
#define LED_DATA_UPDATE_MODULO			(UPDATE_FREQUENCY / LED_DATA_UPDATE_FREQUENCY)
#define LED_DATA_UPDATE_OFFSET			1
#define LED_CFG_UPDATE_MODULO			(UPDATE_FREQUENCY / LED_CFG_UPDATE_FREQUENCY)
#define LED_CFG_UPDATE_OFFSET			2

#define STORE_SETTINGS_DELAY			2	//s
#define STORE_SETTINGS_CNTR_MAX			(STORE_SETTINGS_DELAY * UPDATE_FREQUENCY)

#define CLOCK_SET_INACTIVITY_TIMEOUT	10	//s
#define CLOCK_SET_INACTIVITY_CNTR_MAX	(CLOCK_SET_INACTIVITY_TIMEOUT * UPDATE_FREQUENCY)

#endif /* COMMON_DEFINITONS_H_ */
