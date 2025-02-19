/*
 * rtc_drv.c
 *
 *  Created on: Feb 8, 2025
 *      Author: trwgQ26xxx
 */

#include "rtc_drv.h"

#include "common_defs.h"
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

#define I2C_TIMEOUT				5		//ms

#define I2C_READ_ADDR_BIT		0x01

void Get_RTC_time(volatile struct rtc_data_struct *rtc_data);
void Get_RTC_temp(volatile struct rtc_data_struct *rtc_data);

uint8_t I2C_Read_Register(uint8_t device_addr, uint8_t reg_addr, uint8_t *reg_data);
uint8_t I2C_Write_Register(uint8_t device_addr, uint8_t reg_addr, uint8_t reg_data);

uint8_t I2C_Read_Data(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, uint8_t data_len);
uint8_t I2C_Write_Data(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, uint8_t data_len);

static void Clear_I2C_flags(void);
static void Wait_for_NACK_or_TXIS(void);
static void Wait_for_NACK_or_STOP_or_TXIS(void);
static void Wait_for_NACK_or_TC(void);
static void Wait_for_NACK_or_RXNE(void);
static void Wait_for_RXNE(void);


void Init_RTC(void)
{
	uint8_t reg;

	//volatile uint8_t a = 0;

	/*for(volatile uint16_t test_addr = 0x01; test_addr < 0x100; test_addr++)
	{
		a = I2C_Check_Addr(test_addr);
	}*/

	/* Try to read status register */
	if(I2C_Read_Register(DS3231_ADDR, DS3231_STATUS_ADDR, &reg) == TRUE)
	{
		/* DS3231 acknowledged */

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
	if(I2C_Read_Data(DS3231_ADDR, DS3231_SECONDS_ADDR, buffer, DS3231_TIME_DATA_LEN) == TRUE)
	{
		/* Collected OK */

		/* Convert time */
		rtc_data->second = BCD2BIN(buffer[DS3231_SECONDS_ADDR] & 0x7F);
		rtc_data->minute = BCD2BIN(buffer[DS3231_MINUTES_ADDR] & 0x7F);
		rtc_data->hour = BCD2BIN(buffer[DS3231_HOURS_ADDR] & 0x3F);

		/* Convert date */
		rtc_data->date = BCD2BIN(buffer[DS3231_DATE_ADDR] & 0x3F);
		rtc_data->month = BCD2BIN(buffer[DS3231_MONTH_ADDR] & 0x1F);
		rtc_data->year = BCD2BIN(buffer[DS3231_YEAR_ADDR] & 0xFF);
	}
}

void Get_RTC_temp(volatile struct rtc_data_struct *rtc_data)
{
	uint8_t buffer[DS3231_TEMP_DATA_LEN];

	/* Get data */
	if(I2C_Read_Data(DS3231_ADDR, DS3231_TEMP_MSB_ADDR, buffer, DS3231_TEMP_DATA_LEN) == TRUE)
	{
		/* Collected OK */

		/* Convert temperature */
		rtc_data->temperature = buffer[DS3231_TEMP_MSB_ADDR - DS3231_TEMP_MSB_ADDR];	/* Reg addr minus base addr (set during write */
	}
}

uint8_t I2C_Read_Register(uint8_t device_addr, uint8_t reg_addr, uint8_t *reg_data)
{
	return I2C_Read_Data(device_addr, reg_addr, reg_data, 1);
}

uint8_t I2C_Write_Register(uint8_t device_addr, uint8_t reg_addr, uint8_t reg_data)
{
	return I2C_Write_Data(device_addr, reg_addr, &reg_data, 1);
}

uint8_t I2C_Read_Data(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, uint8_t data_len)
{
	volatile uint8_t transaction_OK = TRUE;

	/* Clear flags */
	Clear_I2C_flags();

	/* Start transaction for 1 byte - register address write */
	LL_I2C_HandleTransfer(I2C1,
			device_addr,
			LL_I2C_ADDRSLAVE_7BIT,
			1,
			LL_I2C_MODE_SOFTEND,
			LL_I2C_GENERATE_START_WRITE);

	/* Wait for either NACK or TXIS flag */
	Wait_for_NACK_or_TXIS();

	/* Check if device acknowledged */
	/* TXIS flag is not set when a NACK was received */
	if(LL_I2C_IsActiveFlag_TXIS(I2C1))
	{
		/* Yes, continue */

		/* Write register address */
		LL_I2C_TransmitData8(I2C1, reg_addr);

		/* Wait for register address to be sent */
		Wait_for_NACK_or_TC();

		/* Check if device acknowledged */
		/* TC would be set if NACK was not received */
		if(LL_I2C_IsActiveFlag_TC(I2C1))
		{
			/* Yes */

			/* Invoke repeated start on the bus, write address for read */
			/* A NACK and a STOP would be automatically generated after the last received byte */
			LL_I2C_HandleTransfer(I2C1,
					device_addr | I2C_READ_ADDR_BIT,
					LL_I2C_ADDRSLAVE_7BIT,
					data_len,
					LL_I2C_MODE_AUTOEND,
					LL_I2C_GENERATE_START_READ);

			/* Wait for either NACK or RXNE flag */
			Wait_for_NACK_or_RXNE();

			/* Check if device acknowledged */
			if(LL_I2C_IsActiveFlag_RXNE(I2C1))
			{
				/* Yes */

				/* Process each byte */
				for(uint32_t i = 0; i < data_len; i++)
				{
					/* Wait for data */
					Wait_for_RXNE();

					/* Check if device acknowledged */
					if(LL_I2C_IsActiveFlag_RXNE(I2C1))
					{
						/* Get data */
						data[i] = LL_I2C_ReceiveData8(I2C1);
					}
					else
					{
						/* Not acknowledged */
						transaction_OK = FALSE;

						break;
					}
				}
			}
			else
			{
				/* Not acknowledged */
				transaction_OK = FALSE;
			}
		}
		else
		{
			/* Not acknowledged */
			transaction_OK = FALSE;
		}
	}
	else
	{
		/* Not acknowledged */
		transaction_OK = FALSE;
	}

	/* End */
	Clear_I2C_flags();

	return transaction_OK;
}

uint8_t I2C_Write_Data(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, uint8_t data_len)
{
	volatile uint8_t transaction_OK = FALSE;

	/* Clear flags */
	Clear_I2C_flags();

	/* Start transaction for N+1 byte - register address plus data bytes */
	/* A STOP would be automatically generated after the last received byte */
	LL_I2C_HandleTransfer(I2C1,
			device_addr,
			LL_I2C_ADDRSLAVE_7BIT,
			data_len + 1,
			LL_I2C_MODE_AUTOEND,
			LL_I2C_GENERATE_START_WRITE);

	/* Wait for either NACK or TXIS flag */
	Wait_for_NACK_or_TXIS();

	/* Check if device acknowledged */
	/* TXIS flag is not set when a NACK was received */
	if(LL_I2C_IsActiveFlag_TXIS(I2C1))
	{
		/* Yes, continue */

		/* Write register address */
		LL_I2C_TransmitData8(I2C1, reg_addr);

		/* Wait for either NACK or TXIS flag */
		Wait_for_NACK_or_TXIS();

		/* Check if device acknowledged */
		/* TXIS flag is not set when a NACK was received */
		if(LL_I2C_IsActiveFlag_TXIS(I2C1))
		{
			/* Yes, continue */

			/* Process each byte */
			for(uint32_t i = 0; i < data_len; i++)
			{
				/* Write data byte */
				LL_I2C_TransmitData8(I2C1, data[i]);

				/* Wait for either NACK, STOP or TXIS flag */
				Wait_for_NACK_or_STOP_or_TXIS();

				/* Check if NACK was received */
				if(LL_I2C_IsActiveFlag_NACK(I2C1))
				{
					/* NACK was received, fail! */
					transaction_OK = FALSE;

					break;
				}

				/* Check if transfer was completed */
				if(i == (data_len - 1))
				{
					if(LL_I2C_IsActiveFlag_STOP(I2C1))
					{
						/* Yes */
						transaction_OK = TRUE;

						break;
					}
					else
					{
						/* NACK was received, fail! */
						transaction_OK = FALSE;

						break;
					}
				}
			}
		}
		else
		{
			/* Not acknowledged */
			transaction_OK = FALSE;
		}

	}
	else
	{
		/* Not acknowledged */
		transaction_OK = FALSE;
	}

	/* End */
	Clear_I2C_flags();

	return transaction_OK;
}

static void Clear_I2C_flags(void)
{
	/* Clear flags */
	LL_I2C_ClearFlag_STOP(I2C1);
	LL_I2C_ClearFlag_NACK(I2C1);
	LL_I2C_ClearFlag_BERR(I2C1);
	LL_I2C_ClearFlag_ARLO(I2C1);
	LL_I2C_ClearFlag_OVR(I2C1);
}

static void Reset_I2C(void)
{
	/* 1. Write PE = 0 */
	LL_I2C_Disable(I2C1);

	/* 2. Check PE = 0 */
	while(LL_I2C_IsEnabled(I2C1))
	{
		LL_I2C_Disable(I2C1);
	}

	/* 3. Write PE = 1 */
	LL_I2C_Enable(I2C1);
}

static void Wait_for_NACK_or_TXIS(void)
{
	uint32_t timeout = 0;
	CLEAR_TICK;

	/* Wait for either NACK or TXIS flag */
	while(	!LL_I2C_IsActiveFlag_ARLO(I2C1) &&
			!LL_I2C_IsActiveFlag_BERR(I2C1) &&
			!LL_I2C_IsActiveFlag_TXIS(I2C1) &&
			!LL_I2C_IsActiveFlag_NACK(I2C1))
	{
		if(CHECK_TICK)
			timeout++;

		/* Timeout can occur when SDA or SCL is stuck @ low */
		if(timeout >= I2C_TIMEOUT)
		{
			Reset_I2C();

			break;
		}
	}
}

static void Wait_for_NACK_or_STOP_or_TXIS(void)
{
	uint32_t timeout = 0;
	CLEAR_TICK;

	/* Wait for either NACK, STOP or TXIS flag */
	while(	!LL_I2C_IsActiveFlag_TXIS(I2C1) &&
			!LL_I2C_IsActiveFlag_STOP(I2C1) &&
			!LL_I2C_IsActiveFlag_NACK(I2C1))
	{
		if(CHECK_TICK)
			timeout++;

		/* Timeout can occur when SDA or SCL is stuck @ low */
		if(timeout >= I2C_TIMEOUT)
		{
			Reset_I2C();

			break;
		}
	}
}

static void Wait_for_NACK_or_TC(void)
{
	uint32_t timeout = 0;
	CLEAR_TICK;

	/* Wait for either NACK or TC flag */
	while(	!LL_I2C_IsActiveFlag_ARLO(I2C1) &&
			!LL_I2C_IsActiveFlag_BERR(I2C1) &&
			!LL_I2C_IsActiveFlag_TC(I2C1) &&
			!LL_I2C_IsActiveFlag_NACK(I2C1))
	{
		if(CHECK_TICK)
			timeout++;

		/* Timeout can occur when SDA or SCL is stuck @ low */
		if(timeout >= I2C_TIMEOUT)
		{
			Reset_I2C();

			break;
		}
	}
}

static void Wait_for_NACK_or_RXNE(void)
{
	uint32_t timeout = 0;
	CLEAR_TICK;

	/* Wait for either NACK or RXNE flag */
	while(	!LL_I2C_IsActiveFlag_ARLO(I2C1) &&
			!LL_I2C_IsActiveFlag_BERR(I2C1) &&
			!LL_I2C_IsActiveFlag_RXNE(I2C1) &&
			!LL_I2C_IsActiveFlag_NACK(I2C1))
	{
		if(CHECK_TICK)
			timeout++;

		/* Timeout can occur when SDA or SCL is stuck @ low */
		if(timeout >= I2C_TIMEOUT)
		{
			Reset_I2C();

			break;
		}
	}
}

static void Wait_for_RXNE(void)
{
	uint32_t timeout = 0;
	CLEAR_TICK;

	/* Wait for RXNE flag */
	while(!LL_I2C_IsActiveFlag_RXNE(I2C1))
	{
		if(CHECK_TICK)
			timeout++;

		/* Timeout can occur when SDA or SCL is stuck @ low */
		if(timeout >= I2C_TIMEOUT)
		{
			Reset_I2C();

			break;
		}
	}
}

}
