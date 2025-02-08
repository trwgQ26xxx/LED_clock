/*
 * rtc_drv.c
 *
 *  Created on: Feb 8, 2025
 *      Author: trwgQ26xxx
 */

#include "rtc_drv.h"

#include "../Core/Inc/i2c.h"

#define DS3231_WRITE_ADDR		0xD0
#define DS3231_READ_ADDR		0xD1

#define DS3231_TIME_DATA_LEN	7

#define DS3231_SECONDS_ADDR		0x00
#define DS3231_MINUTES_ADDR		0x01
#define DS3231_HOURS_ADDR		0x02
#define DS3231_DAY_ADDR			0x03
#define DS3231_DATE_ADDR		0x04
#define DS3231_MONTH_ADDR		0x05
#define DS3231_YEAR_ADDR		0x06

#define DS3231_TEMP_DATA_LEN	2
#define DS3231_TEMP_MSB_ADDR	0x11
#define DS3231_TEMP_LSB_ADDR	0x12

void Get_RTC_time(volatile struct rtc_data_struct *rtc_data);
void Get_RTC_temp(volatile struct rtc_data_struct *rtc_data);

void Init_RTC(void)
{
	/* TODO: Check power-fail flag */

	/* If set, reset time to default */
}

void Get_RTC_data(volatile struct rtc_data_struct *rtc_data)
{
	/* Get time */
	Get_RTC_time(rtc_data);

	/* Get temperature */
	Get_RTC_temp(rtc_data);
}

void Set_RTC_time(volatile struct rtc_data_struct *rtc_data)
{
	uint8_t temp_buffer[DS3231_TIME_DATA_LEN];

	temp_buffer[DS3231_SECONDS_ADDR]	= DEC2BCD(rtc_data->second) & 0x7F;
	temp_buffer[DS3231_MINUTES_ADDR]	= DEC2BCD(rtc_data->minute) & 0x7F;
	temp_buffer[DS3231_HOURS_ADDR] 		= DEC2BCD(rtc_data->hour) & 0x3F;		/* Select 24h mode */
	temp_buffer[DS3231_DAY_ADDR] 		= 0x01;									/* Set day to 1st */
	temp_buffer[DS3231_DATE_ADDR] 		= DEC2BCD(rtc_data->date) & 0x3F;
	temp_buffer[DS3231_MONTH_ADDR] 		= DEC2BCD(rtc_data->month) & 0x1F;
	temp_buffer[DS3231_YEAR_ADDR] 		= DEC2BCD(rtc_data->year) & 0xFF;

	LL_I2C_HandleTransfer(I2C1, DS3231_WRITE_ADDR, LL_I2C_ADDRSLAVE_7BIT, 1+DS3231_TIME_DATA_LEN, LL_I2C_MODE_SOFTEND, LL_I2C_GENERATE_START_WRITE);
	LL_I2C_TransmitData8(I2C1, DS3231_SECONDS_ADDR);

	while(LL_I2C_IsActiveFlag_TXE(I2C1) == RESET);

	for(uint8_t i = 0; i < DS3231_TIME_DATA_LEN; i++)
	{
		LL_I2C_TransmitData8(I2C1, temp_buffer[i]);

		while(LL_I2C_IsActiveFlag_TXE(I2C1) == RESET);
	}

	LL_I2C_GenerateStopCondition(I2C1);
}

void Get_RTC_time(volatile struct rtc_data_struct *rtc_data)
{
	uint8_t temp_buffer[DS3231_TIME_DATA_LEN];

	LL_I2C_HandleTransfer(I2C1, DS3231_WRITE_ADDR, LL_I2C_ADDRSLAVE_7BIT, 1, LL_I2C_MODE_SOFTEND, LL_I2C_GENERATE_START_WRITE);
	LL_I2C_TransmitData8(I2C1, DS3231_SECONDS_ADDR);

	while(LL_I2C_IsActiveFlag_TXE(I2C1) == RESET);

	LL_I2C_AcknowledgeNextData(I2C1, LL_I2C_ACK);
	LL_I2C_HandleTransfer(I2C1, DS3231_READ_ADDR, LL_I2C_ADDRSLAVE_7BIT, DS3231_TIME_DATA_LEN, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_READ);

	for(uint8_t i = 0; i < DS3231_TIME_DATA_LEN; i++)
	{
		if(i < (DS3231_TIME_DATA_LEN - 1))
			LL_I2C_AcknowledgeNextData(I2C1, LL_I2C_ACK);
		else
			LL_I2C_AcknowledgeNextData(I2C1, LL_I2C_NACK);

		while(LL_I2C_IsActiveFlag_RXNE(I2C1) == 0);

		temp_buffer[i] = LL_I2C_ReceiveData8(I2C1);
	}

	LL_I2C_GenerateStopCondition(I2C1);

	/* Convert time */
	rtc_data->second = BCD2BIN(temp_buffer[DS3231_SECONDS_ADDR] & 0x7F);
	rtc_data->minute = BCD2BIN(temp_buffer[DS3231_MINUTES_ADDR] & 0x7F);
	rtc_data->hour = BCD2BIN(temp_buffer[DS3231_HOURS_ADDR] & 0x3F);

	/* Convert date */
	rtc_data->day = BCD2BIN(temp_buffer[DS3231_DAY_ADDR] & 0x7);
	rtc_data->date = BCD2BIN(temp_buffer[DS3231_DATE_ADDR] & 0x3F);
	rtc_data->month = BCD2BIN(temp_buffer[DS3231_MONTH_ADDR] & 0x1F);
	rtc_data->year = BCD2BIN(temp_buffer[DS3231_YEAR_ADDR] & 0xFF);
}

void Get_RTC_temp(volatile struct rtc_data_struct *rtc_data)
{
	uint8_t temp_buffer[DS3231_TEMP_DATA_LEN];

	LL_I2C_HandleTransfer(I2C1, DS3231_WRITE_ADDR, LL_I2C_ADDRSLAVE_7BIT, 1, LL_I2C_MODE_SOFTEND, LL_I2C_GENERATE_START_WRITE);
	LL_I2C_TransmitData8(I2C1, DS3231_TEMP_MSB_ADDR);

	while(LL_I2C_IsActiveFlag_TXE(I2C1) == RESET);

	LL_I2C_AcknowledgeNextData(I2C1, LL_I2C_ACK);
	LL_I2C_HandleTransfer(I2C1, DS3231_READ_ADDR, LL_I2C_ADDRSLAVE_7BIT, DS3231_TEMP_DATA_LEN, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_READ);

	for(uint8_t i = 0; i < DS3231_TEMP_DATA_LEN; i++)
	{
		if(i < (DS3231_TEMP_DATA_LEN - 1))
			LL_I2C_AcknowledgeNextData(I2C1, LL_I2C_ACK);
		else
			LL_I2C_AcknowledgeNextData(I2C1, LL_I2C_NACK);

		while(LL_I2C_IsActiveFlag_RXNE(I2C1) == 0);

		temp_buffer[i] = LL_I2C_ReceiveData8(I2C1);
	}

	LL_I2C_GenerateStopCondition(I2C1);

	/* Convert temperature */
	rtc_data->temperature = temp_buffer[DS3231_TEMP_MSB_ADDR - DS3231_TEMP_MSB_ADDR];	/* Reg addr minus base addr (set during write */
}
