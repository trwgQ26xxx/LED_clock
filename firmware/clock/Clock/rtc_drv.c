/*
 * rtc_drv.c
 *
 *  Created on: Feb 8, 2025
 *      Author: trwgQ26xxx
 */

#include "rtc_drv.h"

#include "common_fcns.h"

#include "../Core/Inc/i2c.h"

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

#define I2C_READ_ADDR_BIT		0x01

void Get_RTC_time(volatile struct rtc_data_struct *rtc_data);
void Get_RTC_temp(volatile struct rtc_data_struct *rtc_data);

void I2C_Read_Register(uint8_t device_addr, uint8_t reg_addr, uint8_t *reg_data);
void I2C_Write_Register(uint8_t device_addr, uint8_t reg_addr, uint8_t reg_data);

void I2C_Read_Data(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, uint8_t data_len);
void I2C_Write_Data(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, uint8_t data_len);

void Init_RTC(void)
{
	uint8_t reg;

	/* Read status register */
	I2C_Read_Register(DS3231_ADDR, DS3231_STATUS_ADDR, &reg);

	/* Check power-fail flag */
	if(reg & DS3231_OSF_BIT)
	{
		/* Power-fail flag is set, reset time to default */

		/* Oscillator enabled, battery-backed square-wave disabled */
		/* do not force temperature conversion, INT output (not SQW), disable alarms */
		I2C_Write_Register(DS3231_ADDR, DS3231_CONTROL_ADDR, 0x04);

		/* Clear power-fail flag, disable 32kHz output, clear alarm flags */
		I2C_Write_Register(DS3231_ADDR, DS3231_STATUS_ADDR, 0x00);

		/* Clear aging offset */
		I2C_Write_Register(DS3231_ADDR, DS3231_AGING_ADDR, 0x00);

		uint8_t alarm1_buffer[DS3231_ALARM1_DATA_LEN], alarm2_buffer[DS3231_ALARM2_DATA_LEN];

		alarm1_buffer[DS3231_ALARM1_SEC - DS3231_ALARM1_SEC] 		= 0x00;
		alarm1_buffer[DS3231_ALARM1_MIN - DS3231_ALARM1_SEC] 		= 0x00;
		alarm1_buffer[DS3231_ALARM1_HOURS - DS3231_ALARM1_SEC] 		= 0x00;
		alarm1_buffer[DS3231_ALARM1_DAY_DATE - DS3231_ALARM1_SEC]	= 0x01;

		/* Disable alarm 1 */
		I2C_Write_Data(DS3231_ADDR, DS3231_ALARM1_SEC, alarm1_buffer, DS3231_ALARM1_DATA_LEN);

		alarm2_buffer[DS3231_ALARM2_MIN - DS3231_ALARM2_MIN] 		= 0x00;
		alarm2_buffer[DS3231_ALARM2_HOURS - DS3231_ALARM2_MIN] 		= 0x00;
		alarm2_buffer[DS3231_ALARM2_DAY_DATE - DS3231_ALARM2_MIN] 	= 0x01;

		/* Disable alarm 2 */
		I2C_Write_Data(DS3231_ADDR, DS3231_ALARM2_MIN, alarm2_buffer, DS3231_ALARM2_DATA_LEN);

		struct rtc_data_struct initial_time;

		initial_time.hour = 0;
		initial_time.minute = 0;
		initial_time.second = 0;

		initial_time.date = 1;
		initial_time.month = 1;
		initial_time.year = 0;

		/* Set default time */
		Set_RTC_time(&initial_time);
	}
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
	uint8_t buffer[DS3231_TIME_DATA_LEN];

	buffer[DS3231_SECONDS_ADDR]	= DEC2BCD(rtc_data->second) & 0x7F;
	buffer[DS3231_MINUTES_ADDR]	= DEC2BCD(rtc_data->minute) & 0x7F;
	buffer[DS3231_HOURS_ADDR] 	= DEC2BCD(rtc_data->hour) & 0x3F;		/* Select 24h mode */
	buffer[DS3231_DAY_ADDR] 	= 0x01;									/* Set day to 1st */
	buffer[DS3231_DATE_ADDR] 	= DEC2BCD(rtc_data->date) & 0x3F;
	buffer[DS3231_MONTH_ADDR] 	= DEC2BCD(rtc_data->month) & 0x1F;
	buffer[DS3231_YEAR_ADDR] 	= DEC2BCD(rtc_data->year) & 0xFF;

	I2C_Write_Data(DS3231_ADDR, DS3231_SECONDS_ADDR, buffer, DS3231_TIME_DATA_LEN);
}

void Get_RTC_time(volatile struct rtc_data_struct *rtc_data)
{
	uint8_t buffer[DS3231_TIME_DATA_LEN];

	/* Get data */
	I2C_Read_Data(DS3231_ADDR, DS3231_SECONDS_ADDR, buffer, DS3231_TIME_DATA_LEN);

	/* Convert time */
	rtc_data->second = BCD2BIN(buffer[DS3231_SECONDS_ADDR] & 0x7F);
	rtc_data->minute = BCD2BIN(buffer[DS3231_MINUTES_ADDR] & 0x7F);
	rtc_data->hour = BCD2BIN(buffer[DS3231_HOURS_ADDR] & 0x3F);

	/* Convert date */
	rtc_data->date = BCD2BIN(buffer[DS3231_DATE_ADDR] & 0x3F);
	rtc_data->month = BCD2BIN(buffer[DS3231_MONTH_ADDR] & 0x1F);
	rtc_data->year = BCD2BIN(buffer[DS3231_YEAR_ADDR] & 0xFF);
}

void Get_RTC_temp(volatile struct rtc_data_struct *rtc_data)
{
	uint8_t buffer[DS3231_TEMP_DATA_LEN];

	/* Get data */
	I2C_Read_Data(DS3231_ADDR, DS3231_TEMP_MSB_ADDR, buffer, DS3231_TEMP_DATA_LEN);

	/* Convert temperature */
	rtc_data->temperature = buffer[DS3231_TEMP_MSB_ADDR - DS3231_TEMP_MSB_ADDR];	/* Reg addr minus base addr (set during write */
}

void I2C_Read_Register(uint8_t device_addr, uint8_t reg_addr, uint8_t *reg_data)
{
	I2C_Read_Data(device_addr, reg_addr, reg_data, 1);
}

void I2C_Write_Register(uint8_t device_addr, uint8_t reg_addr, uint8_t reg_data)
{
	I2C_Write_Data(device_addr, reg_addr, &reg_data, 1);
}

void I2C_Read_Data(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, uint8_t data_len)
{
	LL_I2C_HandleTransfer(I2C1, device_addr, LL_I2C_ADDRSLAVE_7BIT, 1, LL_I2C_MODE_SOFTEND, LL_I2C_GENERATE_START_WRITE);

	LL_I2C_TransmitData8(I2C1, reg_addr);

	while(LL_I2C_IsActiveFlag_TXE(I2C1) == RESET);

	LL_I2C_AcknowledgeNextData(I2C1, LL_I2C_ACK);

	LL_I2C_HandleTransfer(I2C1, device_addr | I2C_READ_ADDR_BIT, LL_I2C_ADDRSLAVE_7BIT, data_len, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_READ);

	for(uint32_t i = 0; i < data_len; i++)
	{
		if(i < (data_len - 1))
			LL_I2C_AcknowledgeNextData(I2C1, LL_I2C_ACK);
		else
			LL_I2C_AcknowledgeNextData(I2C1, LL_I2C_NACK);

		while(LL_I2C_IsActiveFlag_RXNE(I2C1) == 0);

		data[i] = LL_I2C_ReceiveData8(I2C1);
	}

	LL_I2C_GenerateStopCondition(I2C1);
}

void I2C_Write_Data(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, uint8_t data_len)
{
	LL_I2C_HandleTransfer(I2C1, device_addr, LL_I2C_ADDRSLAVE_7BIT, data_len + 1, LL_I2C_MODE_SOFTEND, LL_I2C_GENERATE_START_WRITE);

	LL_I2C_TransmitData8(I2C1, reg_addr);

	while(LL_I2C_IsActiveFlag_TXE(I2C1) == RESET);

	for(uint32_t i = 0; i < data_len; i++)
	{
		LL_I2C_TransmitData8(I2C1, data[i]);

		while(LL_I2C_IsActiveFlag_TXE(I2C1) == RESET);
	}

	LL_I2C_GenerateStopCondition(I2C1);
}
