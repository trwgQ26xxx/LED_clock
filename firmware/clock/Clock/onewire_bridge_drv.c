/*
 * onewire_bridge_drv.c
 *
 *  Created on: Oct 24, 2025
 *      Author: trwgQ26xxx
 */

#include "onewire_bridge_drv.h"
#include "common_defs.h"
#include "common_fcns.h"
#include "i2c_drv.h"

#define DS2482_ADDR					0x30

/* DS2482 Command Codes */
#define DS2482_CMD_DRST				0xF0	// Device Reset
#define DS2482_CMD_WCFG				0xD2	// Write Configuration Register
#define DS2482_CMD_SRP				0xE1	// Set Read Pointer
#define DS2482_CMD_1WRS				0xB4	// Execute 1-Wire Reset
#define DS2482_CMD_1WWB				0xA5	// Execute 1-Wire Write Byte
#define DS2482_CMD_1WRB				0x96	// Execute 1-Wire Read Byte
#define DS2482_CMD_1WSB				0x87	// Execute 1-Wire Search Byte

/* DS2482 Pointers to Registers */
#define DS2482_PTR_STATUS_REG		0xF0
#define DS2482_PTR_READ_DATA_REG	0xE1
#define DS2482_PTR_CONFIG_REG		0xC3

/* DS2482 Status Register Bits */
#define DS2482_STATUS_1WB			0x01	// 1-Wire Busy
#define DS2482_STATUS_PPD			0x02	// Presence Pulse Detect
#define DS2482_STATUS_SD			0x04	// Short Detected
#define DS2482_STATUS_LL			0x08	// Logic Level
#define DS2482_STATUS_RST			0x10	// Device Reset
#define DS2482_STATUS_SBR			0x20	// Single Bit Result
#define DS2482_STATUS_TSB			0x40	// Triplet Second Bit
#define DS2482_STATUS_DIR			0x80	// Branch Direction Taken

/* DS2482 Configuration */
#define DS2482_CONFIGURATION		0xE1	// Active pull-up enabled, Strong pull-up disabled, 1-Wire speed standard

inline static uint8_t DS2482_read_data_register(uint8_t *data_reg);
inline static void DS2482_Delay(void);


uint8_t Init_OneWire_bridge(void)
{
	uint8_t init_OK = FALSE;

	uint8_t config_reg = 0x00;

	/* Trigger DS2482 device reset */
    if(I2C_Write_Command(DS2482_ADDR, DS2482_CMD_DRST) == TRUE)
	{
		/* DS2482 acknowledged */

		/* Small delay to allow DS2482 to reset */
		DS2482_Delay();

		/* Write configuration register: Active pull-up, 1-Wire speed standard */
		if(I2C_Write_Register(DS2482_ADDR, DS2482_CMD_WCFG, DS2482_CONFIGURATION) == TRUE)
		{
			/* Configuration written successfully */
			/* Register pointer is set to configuration register */
			/* Read configuration register back */
			if(I2C_Read_Command(DS2482_ADDR, &config_reg) == TRUE)
			{
				/* Configuration read successfully */

				/* Check if configuration is correct (upper nibble of read is always 0) */
				if(config_reg == (DS2482_CONFIGURATION & 0x0F))
				{
					/* Configuration is correct */
					init_OK = TRUE;
				}
				else
				{
					/* Configuration is invalid */
					init_OK = FALSE;
				}
			}
			else
			{
				/* Failed to read configuration */
				init_OK = FALSE;
			}
		}
		else
		{
			/* Failed to write configuration */
			init_OK = FALSE;
		}
	}
	else
	{
		/* DS2482 Not acknowledged, device not present */
		init_OK = FALSE;
	}

	return init_OK;
}

uint8_t OneWire_reset(void)
{
	uint8_t any_device_present = FALSE;

	uint8_t further_polling_needed = TRUE;
	uint8_t status_reg = 0x00;

	/* Trigger DS2482 1 Wire Reset, register pointer would be set to status register */
    if(I2C_Write_Command(DS2482_ADDR, DS2482_CMD_1WRS) == TRUE)
	{
		/* Keep polling status register until 1-Wire Busy bit is cleared */
		do
		{
			/* Read status register */
			if(I2C_Read_Command(DS2482_ADDR, &status_reg) == TRUE)
			{
				/* Check if 1-Wire Busy bit is cleared */
				if(!(status_reg & DS2482_STATUS_1WB))
				{
					/* 1-Wire Busy bit cleared, further polling not needed */
					further_polling_needed = FALSE;

					/* Check if presence pulse was detected, and there is not a short on the bus */
					if((status_reg & DS2482_STATUS_PPD) && !(status_reg & DS2482_STATUS_SD))
					{
						/* Presence pulse detected, and there is not a short on the bus */
						/* Return at least one device is present */
						any_device_present = TRUE;
					}
					else
					{
						/* No presence pulse detected or there is a short on the bus */
						/* Return no devices present on the bus */
						any_device_present = FALSE;
					}
				}
				else
				{
					/* 1-Wire Busy bit still set, further polling needed */
					further_polling_needed = TRUE;
				}
			}
			else
			{
				/* Failed to read status register, quit */
				further_polling_needed = FALSE;
			}
		}
		while(further_polling_needed == TRUE);
	}
	else
	{
		/* Failed to trigger 1-Wire Reset */
		any_device_present = FALSE;
	}

	return any_device_present;
}

uint8_t OneWire_write_byte(uint8_t data)
{
	uint8_t written_OK = FALSE;

	uint8_t further_polling_needed = TRUE;
	uint8_t status_reg = 0x00;

	/* Trigger DS2482 1-Wire Write Byte */
	if(I2C_Write_Register(DS2482_ADDR, DS2482_CMD_1WWB, data) == TRUE)
	{
		/* Keep polling status register until 1-Wire Busy bit is cleared */
		do
		{
			/* Read status register */
			if(I2C_Read_Command(DS2482_ADDR, &status_reg) == TRUE)
			{
				/* Check if 1-Wire Busy bit is cleared */
				if(!(status_reg & DS2482_STATUS_1WB))
				{
					/* 1-Wire Busy bit cleared, further polling not needed */
					further_polling_needed = FALSE;

					/* Byte written successfully */
					written_OK = TRUE;
				}
				else
				{
					/* 1-Wire Busy bit still set, further polling needed */
					further_polling_needed = TRUE;
				}
			}
			else
			{
				/* Failed to read status register, quit */
				further_polling_needed = FALSE;
			}
		}
		while(further_polling_needed == TRUE);
	}
	else
	{
		/* Failed to trigger 1-Wire Write Byte */
		written_OK = FALSE;
	}

	return written_OK;
}

uint8_t OneWire_read_byte(uint8_t *data)
{
	uint8_t read_OK = FALSE;

	uint8_t further_polling_needed = TRUE;
	uint8_t status_reg = 0x00;

	/* Trigger DS2482 1-Wire Read Byte */
	if(I2C_Write_Command(DS2482_ADDR, DS2482_CMD_1WRB) == TRUE)
	{
		/* Keep polling status register until 1-Wire Busy bit is cleared */
		do
		{
			/* Read status register */
			if(I2C_Read_Command(DS2482_ADDR, &status_reg) == TRUE)
			{
				/* Check if 1-Wire Busy bit is cleared */
				if(!(status_reg & DS2482_STATUS_1WB))
				{
					/* 1-Wire Busy bit cleared, further polling not needed */
					further_polling_needed = FALSE;

					/* Read data register */
					read_OK = DS2482_read_data_register(data);
				}
				else
				{
					/* 1-Wire Busy bit still set, further polling needed */
					further_polling_needed = TRUE;
				}
			}
			else
			{
				/* Failed to read status register, quit */
				further_polling_needed = FALSE;
			}
		}
		while(further_polling_needed == TRUE);
	}
	else
	{
		/* Failed to trigger 1-Wire Read Byte */
		read_OK = FALSE;
	}

	return read_OK;
}

inline static uint8_t DS2482_read_data_register(uint8_t *data_reg)
{
	uint8_t read_OK = FALSE;

	/* Set pointer to data register */
	if(I2C_Write_Register(DS2482_ADDR, DS2482_CMD_SRP, DS2482_PTR_READ_DATA_REG) == TRUE)
	{
		/* Read data register */
		if(I2C_Read_Command(DS2482_ADDR, data_reg) == TRUE)
		{
			read_OK = TRUE;
		}
		else
		{
			/* Failed to read data register */
			read_OK = FALSE;
		}
	}
	else
	{
		/* Failed to set pointer to data register */
		read_OK = FALSE;
	}
    
	return read_OK;
}

inline static void DS2482_Delay(void)
{
	for(uint32_t i = 0; i < 1000; i++)
	{
		asm volatile("nop");
	}
}
