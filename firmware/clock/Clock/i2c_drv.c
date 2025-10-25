/*
 * i2c_drv.c
 *
 *  Created on: Feb 20, 2025
 *      Author: trwgQ26xxx
 */

#include "i2c_drv.h"

#include "../Core/Inc/i2c.h"

#include "common_defs.h"
#include "common_fcns.h"

#define I2C_TIMEOUT				5		//ms

#define I2C_READ_ADDR_BIT		0x01

static void Clear_I2C(void);
static void Reset_I2C(void);
static void Wait_for_NACK_or_TXIS(void);
static void Wait_for_STOP(void);
static void Wait_for_NACK_or_TC(void);
static void Wait_for_NACK_or_RXNE(void);
static void Wait_for_RXNE(void);
static void Wait_for_TXE(void);

uint8_t I2C_Check_Addr(uint8_t device_addr)
{
	uint8_t device_present = FALSE;

	/* Clear flags */
	Clear_I2C();

	/* Try given address */
	LL_I2C_HandleTransfer(I2C1,
			device_addr,
			LL_I2C_ADDRSLAVE_7BIT,
			0,
			LL_I2C_MODE_AUTOEND,
			LL_I2C_GENERATE_START_WRITE);

	/* Wait for end of transfer */
	Wait_for_STOP();

	/* Is STOP flag set ? */
	if(LL_I2C_IsActiveFlag_STOP(I2C1))
	{
		/* It is, transfer ended OK */

		/* Is NACK flag set ? */
		if(LL_I2C_IsActiveFlag_NACK(I2C1))
		{
			/* Yes, no device of given address is not present on the bus */
			device_present = FALSE;
		}
		else
		{
			/* No, the device acknowledged */
			device_present = TRUE;
		}
	}
	else
	{
		/* Arbitration lost or SDA/SCL line stuck */
		device_present = FALSE;
	}

	/* Clear flags */
	Clear_I2C();

	return device_present;
}

uint8_t I2C_Write_Command(uint8_t device_addr, uint8_t command)
{
	volatile uint8_t transaction_OK = FALSE;

	/* Check if device address not equal to 0 */
	if(device_addr != 0)
	{
		/* Clear flags, Flush TXDR */
		Clear_I2C();

		/* Wait for TXDR to be empty */
		Wait_for_TXE();

		/* Verify that TXDR is empty */
		if(LL_I2C_IsActiveFlag_TXE(I2C1))
		{
			/* Write command */
			/* Write the first data in I2C_TXDR before the transmission starts, */
			/* due to HW bug "Transmission stalled after first byte transfer" */
			LL_I2C_TransmitData8(I2C1, command);

			/* Start transaction for 1 byte */
			/* A STOP would be automatically generated after the last received byte */
			LL_I2C_HandleTransfer(I2C1,
					device_addr,
					LL_I2C_ADDRSLAVE_7BIT,
					1,
					LL_I2C_MODE_AUTOEND,
					LL_I2C_GENERATE_START_WRITE);

			/* Wait for transaction to end */
			Wait_for_STOP();

			/* Check STOP flag and NACK flag */
			if(LL_I2C_IsActiveFlag_STOP(I2C1) && (!LL_I2C_IsActiveFlag_NACK(I2C1)))
			{
				/* If it is set, transaction was OK */
				transaction_OK = TRUE;
			}
		}
		else
		{
			/* TXDR unavailable */
			transaction_OK = FALSE;
		}

		/* End */
		Clear_I2C();
	}
	else
	{
		/* Wrong input arguments */
		transaction_OK = FALSE;
	}

	return transaction_OK;
}

uint8_t I2C_Read_Command(uint8_t device_addr, uint8_t *command)
{
	volatile uint8_t transaction_OK = FALSE;

	/* Check if device address not equal to 0 */
	if(device_addr != 0)
	{
		/* Clear flags, Flush TXDR */
		Clear_I2C();

		/* Wait for TXDR to be empty */
		Wait_for_TXE();

		/* Verify that TXDR is empty */
		if(LL_I2C_IsActiveFlag_TXE(I2C1))
		{
			/* Start transaction for 1 byte, write address for read */
			/* A NACK and a STOP would be automatically generated after the last received byte */
			LL_I2C_HandleTransfer(I2C1,
					device_addr | I2C_READ_ADDR_BIT,
					LL_I2C_ADDRSLAVE_7BIT,
					1,
					LL_I2C_MODE_AUTOEND,
					LL_I2C_GENERATE_START_READ);

			/* Wait for either NACK or RXNE flag */
			Wait_for_NACK_or_RXNE();

			/* Check if device acknowledged */
			if(LL_I2C_IsActiveFlag_RXNE(I2C1))
			{
				/* Yes */

				/* Get command */
				*command = LL_I2C_ReceiveData8(I2C1);

				/* Wait for transaction to end */
				Wait_for_STOP();

				/* Check STOP flag */
				if(LL_I2C_IsActiveFlag_STOP(I2C1))
				{
					/* If it is set, transaction was OK */
					transaction_OK = TRUE;
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
			/* TXDR unavailable */
			transaction_OK = FALSE;
		}

		/* End */
		Clear_I2C();
	}
	else
	{
		/* Wrong input arguments */
		transaction_OK = FALSE;
	}

	return transaction_OK;
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
	volatile uint8_t transaction_OK = FALSE;

	/* Check if data length > 0 and device address not equal to 0 */
	if((data_len > 0) && (device_addr != 0))
	{
		/* Clear flags, Flush TXDR */
		Clear_I2C();

		/* Wait for TXDR to be empty */
		Wait_for_TXE();

		/* Verify that TXDR is empty */
		if(LL_I2C_IsActiveFlag_TXE(I2C1))
		{
			/* Write register address */
			/* Write the first data in I2C_TXDR before the transmission starts, */
			/* due to HW bug "Transmission stalled after first byte transfer" */
			LL_I2C_TransmitData8(I2C1, reg_addr);

			/* Start transaction for 1 byte - register address write */
			LL_I2C_HandleTransfer(I2C1,
					device_addr,
					LL_I2C_ADDRSLAVE_7BIT,
					1,
					LL_I2C_MODE_SOFTEND,
					LL_I2C_GENERATE_START_WRITE);

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

					/* Wait for transaction to end */
					Wait_for_STOP();

					/* Check STOP flag */
					if(LL_I2C_IsActiveFlag_STOP(I2C1))
					{
						/* If it is set, transaction was OK */
						transaction_OK = TRUE;
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
			/* TXDR unavailable */
			transaction_OK = FALSE;
		}

		/* End */
		Clear_I2C();
	}
	else
	{
		/* Wrong input arguments */
		transaction_OK = FALSE;
	}

	return transaction_OK;
}

uint8_t I2C_Write_Data(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, uint8_t data_len)
{
	volatile uint8_t transaction_OK = FALSE;

	/* Check if data length > 0 and device address not equal to 0 */
	if((data_len > 0) && (device_addr != 0))
	{
		/* Clear flags, Flush TXDR */
		Clear_I2C();

		/* Wait for TXDR to be empty */
		Wait_for_TXE();

		/* Verify that TXDR is empty */
		if(LL_I2C_IsActiveFlag_TXE(I2C1))
		{
			/* Write register address */
			/* Write the first data in I2C_TXDR before the transmission starts, */
			/* due to HW bug "Transmission stalled after first byte transfer" */
			LL_I2C_TransmitData8(I2C1, reg_addr);

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

				/* Process each byte */
				for(uint32_t i = 0; i < data_len; i++)
				{
					/* Wait for TXDR to be empty */
					Wait_for_NACK_or_TXIS();

					/* Check if device acknowledged */
					/* TXIS flag is not set when a NACK was received */
					if(LL_I2C_IsActiveFlag_TXIS(I2C1))
					{
						/* Write data byte */
						LL_I2C_TransmitData8(I2C1, data[i]);
					}
					else
					{
						/* NACK received */
						transaction_OK = FALSE;

						break;
					}
				}

				/* Wait for transaction to end */
				Wait_for_STOP();

				/* Check STOP flag and NACK flag */
				if(LL_I2C_IsActiveFlag_STOP(I2C1) && (!LL_I2C_IsActiveFlag_NACK(I2C1)))
				{
					/* If it is set, transaction was OK */
					transaction_OK = TRUE;
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
			/* TXDR unavailable */
			transaction_OK = FALSE;
		}

		/* End */
		Clear_I2C();
	}
	else
	{
		/* Wrong input arguments */
		transaction_OK = FALSE;
	}

	return transaction_OK;
}

static void Clear_I2C(void)
{
	/* Clear flags */
	LL_I2C_ClearFlag_STOP(I2C1);
	LL_I2C_ClearFlag_NACK(I2C1);
	LL_I2C_ClearFlag_BERR(I2C1);
	LL_I2C_ClearFlag_ARLO(I2C1);
	LL_I2C_ClearFlag_OVR(I2C1);

	/* Flush TXDR */
	LL_I2C_ClearFlag_TXE(I2C1);
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
	volatile uint32_t timeout = 0;
	CLEAR_TICK;

	/* Wait for either NACK or TXIS flag */
	while(	!LL_I2C_IsActiveFlag_ARLO(I2C1) &&
			!LL_I2C_IsActiveFlag_TXIS(I2C1) &&
			!LL_I2C_IsActiveFlag_NACK(I2C1))
	{
		if(CHECK_TICK)
			timeout++;

		/* Timeout can occur when SDA or SCL is stuck @ low */
		if(timeout >= I2C_TIMEOUT)
		{
			/* All flags will be cleared afterwards */
			Reset_I2C();

			break;
		}
	}
}

static void Wait_for_STOP(void)
{
	uint32_t timeout = 0;
	CLEAR_TICK;

	/* Wait for either NACK, STOP or TXIS flag */
	while(!LL_I2C_IsActiveFlag_STOP(I2C1))
	{
		if(CHECK_TICK)
			timeout++;

		/* Timeout can occur when SDA or SCL is stuck @ low */
		if(timeout >= I2C_TIMEOUT)
		{
			/* All flags will be cleared afterwards */
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
			!LL_I2C_IsActiveFlag_TC(I2C1) &&
			!LL_I2C_IsActiveFlag_NACK(I2C1))
	{
		if(CHECK_TICK)
			timeout++;

		/* Timeout can occur when SDA or SCL is stuck @ low */
		if(timeout >= I2C_TIMEOUT)
		{
			/* All flags will be cleared afterwards */
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
			!LL_I2C_IsActiveFlag_RXNE(I2C1) &&
			!LL_I2C_IsActiveFlag_NACK(I2C1))
	{
		if(CHECK_TICK)
			timeout++;

		/* Timeout can occur when SDA or SCL is stuck @ low */
		if(timeout >= I2C_TIMEOUT)
		{
			/* All flags will be cleared afterwards */
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
			/* All flags will be cleared afterwards */
			Reset_I2C();

			break;
		}
	}
}

static void Wait_for_TXE(void)
{
	uint32_t timeout = 0;
	CLEAR_TICK;

	/* Wait for RXNE flag */
	while(!LL_I2C_IsActiveFlag_TXE(I2C1))
	{
		if(CHECK_TICK)
			timeout++;

		if(timeout >= I2C_TIMEOUT)
		{
			/* All flags will be cleared afterwards */
			Reset_I2C();

			break;
		}
	}
}

