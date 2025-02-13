/*
 * display_drv.h
 *
 *  Created on: Feb 8, 2025
 *      Author: trwgQ26xxx
 */

#ifndef DISPLAY_DRV_H_
#define DISPLAY_DRV_H_

#include "common_definitons.h"

#define MIN_INTENSITY			0x0
#define MAX_INTENSITY			0xF

struct display_data_struct
{
	uint8_t hour;
	uint8_t minute;
	uint8_t second;

	uint8_t date;
	uint8_t month;
	uint8_t year;

	uint8_t hour_colon;

	int8_t int_temperature;

	uint8_t intensity;

	uint8_t special_mode;
};

enum DISPLAY_MODES
{
	DISPLAY_INT_TEMP = 0, DISPLAY_EXT_TEMP,
	DISPLAY_SET_HOUR, DISPLAY_SET_MINUTE, DISPLAY_SET_SECOND,
	DISPLAY_SET_DATE, DISPLAY_SET_MONTH, DISPLAY_SET_YEAR,
	DISPLAY_INTENSITY,
	DISPLAY_DEMO
};

void Init_display(uint8_t intensity);

void Update_display_config(volatile struct display_data_struct *data);

void Update_display_data(volatile struct display_data_struct *data);

#endif /* DISPLAY_DRV_H_ */
