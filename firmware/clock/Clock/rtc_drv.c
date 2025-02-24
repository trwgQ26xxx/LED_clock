/*
 * rtc_drv.c
 *
 *  Created on: Feb 8, 2025
 *      Author: trwgQ26xxx
 */

#include "rtc_drv.h"

#include "common_defs.h"
#include "common_fcns.h"

#include "i2c_drv.h"

#define DS3231_ADDR				0xD0

#define DS3231_TIME_DATA_LEN	7
#define DS3231_SECONDS_ADDR		0x00
#define DS3231_MINUTES_ADDR		0x01
#define DS3231_HOURS_ADDR		0x02
#define DS3231_DAY_ADDR			0x03
#define DS3231_DATE_ADDR		0x04
#define DS3231_MONTH_ADDR		0x05
#define DS3231_YEAR_ADDR		0x06

#define DS3231_ALARM1_DATA_LEN	4
#define DS3231_ALARM1_SEC		0x07
#define DS3231_ALARM1_MIN		0x08
#define DS3231_ALARM1_HOURS		0x09
#define DS3231_ALARM1_DAY_DATE	0x0A

#define DS3231_ALARM2_DATA_LEN	3
#define DS3231_ALARM2_MIN		0x0B
#define DS3231_ALARM2_HOURS		0x0C
#define DS3231_ALARM2_DAY_DATE	0x0D

#define DS3231_CONTROL_ADDR		0x0E
#define DS3231_STATUS_ADDR		0x0F
#define DS3231_AGING_ADDR		0x10

#define DS3231_TEMP_DATA_LEN	2
#define DS3231_TEMP_MSB_ADDR	0x11
#define DS3231_TEMP_LSB_ADDR	0x12

#define DS3231_OSF_BIT			0x80

uint8_t Get_RTC_time(volatile struct rtc_data_struct *rtc_data);
uint8_t Get_RTC_temp(volatile struct rtc_data_struct *rtc_data);

uint8_t Set_RTC_config(void);

uint8_t Init_RTC(void)
{
	uint8_t init_OK = TRUE;

	uint8_t reg;

	/* Try to read status register */
	if(I2C_Read_Register(DS3231_ADDR, DS3231_STATUS_ADDR, &reg) == TRUE)
	{
		/* DS3231 acknowledged */

		/* Check power-fail flag */
		if(reg & DS3231_OSF_BIT)
		{
			/* Power-fail flag is set, reset time to default */
			struct rtc_data_struct initial_time;

			initial_time.hour = 0;
			initial_time.minute = 0;
			initial_time.second = 0;

			initial_time.date = 1;
			initial_time.month = 1;
			initial_time.year = 25;

			/* Set default time */
			init_OK &= Set_RTC_time(&initial_time);
		}
	}
	else
	{
		/* Not acknowledged */
		init_OK = FALSE;
	}

	return init_OK;
}

uint8_t Get_RTC_data(volatile struct rtc_data_struct *rtc_data)
{
	uint8_t get_OK = TRUE;

	/* Get time */
	get_OK &= Get_RTC_time(rtc_data);

	/* Get temperature */
	get_OK &= Get_RTC_temp(rtc_data);

	return get_OK;
}

uint8_t Set_RTC_time(volatile struct rtc_data_struct *rtc_data)
{
	uint8_t set_OK = TRUE;

	uint8_t buffer[DS3231_TIME_DATA_LEN];

	/* Reset configuration to default */
	set_OK &= Set_RTC_config();

	buffer[DS3231_SECONDS_ADDR]	= DEC2BCD(rtc_data->second) & 0x7F;
	buffer[DS3231_MINUTES_ADDR]	= DEC2BCD(rtc_data->minute) & 0x7F;
	buffer[DS3231_HOURS_ADDR] 	= DEC2BCD(rtc_data->hour) & 0x3F;		/* Select 24h mode */
	buffer[DS3231_DAY_ADDR] 	= 0x01;									/* Set day to 1st */
	buffer[DS3231_DATE_ADDR] 	= DEC2BCD(rtc_data->date) & 0x3F;
	buffer[DS3231_MONTH_ADDR] 	= DEC2BCD(rtc_data->month) & 0x1F;
	buffer[DS3231_YEAR_ADDR] 	= DEC2BCD(rtc_data->year) & 0xFF;

	set_OK &= I2C_Write_Data(DS3231_ADDR, DS3231_SECONDS_ADDR, buffer, DS3231_TIME_DATA_LEN);

	return set_OK;
}

uint8_t Get_RTC_time(volatile struct rtc_data_struct *rtc_data)
{
	uint8_t get_OK = FALSE;

	uint8_t buffer[DS3231_TIME_DATA_LEN];

	/* Get data */
	if(I2C_Read_Data(DS3231_ADDR, DS3231_SECONDS_ADDR, buffer, DS3231_TIME_DATA_LEN) == TRUE)
	{
		/* Convert time */
		rtc_data->second = BCD2BIN(buffer[DS3231_SECONDS_ADDR] & 0x7F);
		rtc_data->minute = BCD2BIN(buffer[DS3231_MINUTES_ADDR] & 0x7F);
		rtc_data->hour = BCD2BIN(buffer[DS3231_HOURS_ADDR] & 0x3F);

		/* Convert date */
		rtc_data->date = BCD2BIN(buffer[DS3231_DATE_ADDR] & 0x3F);
		rtc_data->month = BCD2BIN(buffer[DS3231_MONTH_ADDR] & 0x1F);
		rtc_data->year = BCD2BIN(buffer[DS3231_YEAR_ADDR] & 0xFF);

		/* Collected OK */
		get_OK = TRUE;
	}

	return get_OK;
}

uint8_t Get_RTC_temp(volatile struct rtc_data_struct *rtc_data)
{
	uint8_t get_OK = FALSE;

	uint8_t buffer[DS3231_TEMP_DATA_LEN];

	/* Get data */
	if(I2C_Read_Data(DS3231_ADDR, DS3231_TEMP_MSB_ADDR, buffer, DS3231_TEMP_DATA_LEN) == TRUE)
	{
		/* Convert temperature */
		rtc_data->temperature = buffer[DS3231_TEMP_MSB_ADDR - DS3231_TEMP_MSB_ADDR];	/* Reg addr minus base addr (set during write */

		/* Collected OK */
		get_OK = TRUE;
	}

	return get_OK;
}

uint8_t Set_RTC_config(void)
{
	uint8_t set_OK = TRUE;

	/* Oscillator enabled, battery-backed square-wave disabled */
	/* do not force temperature conversion, INT output (not SQW), disable alarms */
	set_OK &= I2C_Write_Register(DS3231_ADDR, DS3231_CONTROL_ADDR, 0x04);

	/* Clear power-fail flag, disable 32kHz output, clear alarm flags */
	set_OK &= I2C_Write_Register(DS3231_ADDR, DS3231_STATUS_ADDR, 0x00);

	/* Clear aging offset */
	set_OK &= I2C_Write_Register(DS3231_ADDR, DS3231_AGING_ADDR, 0x00);

	uint8_t alarm1_buffer[DS3231_ALARM1_DATA_LEN], alarm2_buffer[DS3231_ALARM2_DATA_LEN];

	alarm1_buffer[DS3231_ALARM1_SEC - DS3231_ALARM1_SEC] 		= 0x00;
	alarm1_buffer[DS3231_ALARM1_MIN - DS3231_ALARM1_SEC] 		= 0x00;
	alarm1_buffer[DS3231_ALARM1_HOURS - DS3231_ALARM1_SEC] 		= 0x00;
	alarm1_buffer[DS3231_ALARM1_DAY_DATE - DS3231_ALARM1_SEC]	= 0x01;

	/* Disable alarm 1 */
	set_OK &= I2C_Write_Data(DS3231_ADDR, DS3231_ALARM1_SEC, alarm1_buffer, DS3231_ALARM1_DATA_LEN);

	alarm2_buffer[DS3231_ALARM2_MIN - DS3231_ALARM2_MIN] 		= 0x00;
	alarm2_buffer[DS3231_ALARM2_HOURS - DS3231_ALARM2_MIN] 		= 0x00;
	alarm2_buffer[DS3231_ALARM2_DAY_DATE - DS3231_ALARM2_MIN] 	= 0x01;

	/* Disable alarm 2 */
	set_OK &= I2C_Write_Data(DS3231_ADDR, DS3231_ALARM2_MIN, alarm2_buffer, DS3231_ALARM2_DATA_LEN);

	return set_OK;
}
