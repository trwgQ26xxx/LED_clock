/*
 * rtc_drv.h
 *
 *  Created on: Feb 8, 2025
 *      Author: trwgQ26xxx
 */

#ifndef RTC_DRV_H_
#define RTC_DRV_H_

#include "common_definitons.h"

struct rtc_data_struct
{
	uint8_t year;
	uint8_t month;
	uint8_t date;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;

	int8_t temperature;
};

void Init_RTC(void);

void Get_RTC_data(volatile struct rtc_data_struct *rtc_data);

void Set_RTC_time(volatile struct rtc_data_struct *rtc_data);

#endif /* RTC_DRV_H_ */
