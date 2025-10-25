/*
 * ext_temp_sens_drv.c
 *
 *  Created on: Oct 24, 2025
 *      Author: trwgQ26xxx
 */


#include "common_defs.h"
#include "common_fcns.h"
#include "onewire_bridge_drv.h"


/* DS18B20 Commands */
#define DS18B20_SKIP_ROM			0xCC
#define DS18B20_CONVERT_T			0x44
#define DS18B20_WRITE_SCRATCHPAD	0x4E
#define DS18B20_READ_SCRATCHPAD		0xBE

/* DS18B20 Constants */
#define DS18B20_TH_BYTE				0x7F	// Temperature High Threshold = 127*C (set out of range to not trigger alarm)
#define DS18B20_TL_BYTE				0x80	// Temperature Low Threshold = -128*C (set out of range to not trigger alarm)
#define DS18B20_CONFIG_BYTE			0x7F	// Configuration Register (12-bit resolution, default value)

/* DS18B20 Scratchpad */
#define DS18B20_SCRATCHPAD_SIZE		9		// Scratchpad size in bytes
#define DS18B20_TEMP_LSB_INDEX		0		// Temperature LSB index in scratchpad
#define DS18B20_TEMP_MSB_INDEX		1		// Temperature MSB index in scratchpad
#define DS18B20_TH_INDEX			2		// Temperature High Threshold index in scratchpad
#define DS18B20_TL_INDEX			3		// Temperature Low Threshold index in scratchpad
#define DS18B20_CONFIG_INDEX		4		// Configuration Register index in scratchpad
#define DS18B20_RESERVED1_INDEX		5		// Reserved byte 1 index in scratchpad
#define DS18B20_RESERVED2_INDEX		6		// Reserved byte 2 index in scratchpad
#define DS18B20_RESERVED3_INDEX		7		// Reserved byte 3 index in scratchpad
#define DS18B20_CRC_INDEX			8		// CRC index in scratchpad

/* DS18B20 CRC */
#define DS18B20_CRC_INIT_VALUE		0x00
#define DS18B20_CRC_MSB_MASK		0x01
#define DS18B20_CRC_POLYNOMIAL		0x8C	// Polynomial x^8 + x^5 + x^4 + 1 (0x31 reversed)

#define DS18B20_CRC_SIZE			1											// Size of the CRC in bytes
#define DS18B20_CRC_CALC_SIZE		(DS18B20_SCRATCHPAD_SIZE - DS18B20_CRC_SIZE)// Number of bytes to use for CRC calculation



static uint8_t DS18B20_set_configuration(void);
static uint8_t DS18B20_read_scratchpad(uint8_t *scratch);
static uint8_t DS18B20_calculate_CRC(const uint8_t *data, uint8_t len);


uint8_t Init_ext_temp_sens(void)
{
	uint8_t init_OK = FALSE;

	/* Trigger 1-Wire Reset to check if an external temperature sensor is present */
	if(OneWire_reset() == TRUE)
	{
		init_OK = DS18B20_set_configuration();
	}

	return init_OK;
}

uint8_t Ext_temp_start_conversion(void)
{
	uint8_t conversion_triggered_OK = FALSE;

	if(OneWire_reset() == TRUE)
	{
		if(OneWire_write_byte(DS18B20_SKIP_ROM) == TRUE)
		{
			if(OneWire_write_byte(DS18B20_CONVERT_T) == TRUE)
			{
				conversion_triggered_OK = TRUE;
			}
		}
	}

	return conversion_triggered_OK;
}

uint8_t Ext_temp_read_temperature(int8_t *temperature)
{
	uint8_t read_OK = FALSE;

	uint8_t scratch[DS18B20_SCRATCHPAD_SIZE];

	/* Read scratchpad */
	if(DS18B20_read_scratchpad(scratch) == TRUE)
	{
		/* Calculate and check CRC */
		if(DS18B20_calculate_CRC(scratch, DS18B20_CRC_CALC_SIZE) == scratch[DS18B20_CRC_INDEX])
		{
			/* Combine LSB and MSB to get temperature in Celsius */
			int16_t raw_temp = (int16_t)((scratch[DS18B20_TEMP_MSB_INDEX] << 8) | scratch[DS18B20_TEMP_LSB_INDEX]);

			/* Convert raw temperature to integer Celsius (discard fractional part) */
			*temperature = (int8_t)(raw_temp >> 4);

			read_OK = TRUE;
		}
		else
		{
			/* CRC mismatch, data invalid */
			read_OK = FALSE;
		}
	}
	else
	{
		/* Failed to read scratchpad */
		read_OK = FALSE;
	}

	return read_OK;
}

static uint8_t DS18B20_set_configuration(void)
{
	uint8_t configured_OK = FALSE;

	if(OneWire_write_byte(DS18B20_SKIP_ROM) == TRUE)
	{
		if(OneWire_write_byte(DS18B20_WRITE_SCRATCHPAD) == TRUE)
		{
			if(OneWire_write_byte(DS18B20_TH_BYTE) == TRUE)
			{
				if(OneWire_write_byte(DS18B20_TL_BYTE) == TRUE)
				{
					if(OneWire_write_byte(DS18B20_CONFIG_BYTE) == TRUE)
					{
						configured_OK = TRUE;
					}
				}
			}
		}
	}

	return configured_OK;
}

static uint8_t DS18B20_read_scratchpad(uint8_t *scratch)
{
	uint8_t read_OK = TRUE;

	if(OneWire_reset() == TRUE)
	{
		if(OneWire_write_byte(DS18B20_SKIP_ROM) == TRUE)
		{
			if(OneWire_write_byte(DS18B20_READ_SCRATCHPAD) == TRUE)
			{
				for(uint8_t i = 0; i < DS18B20_SCRATCHPAD_SIZE; i++)
				{
					if(OneWire_read_byte(&scratch[i]) == TRUE)
					{
						/* Byte read successfully, continue */
						continue;
					}
					else
					{
						/* Failed to read byte */
						read_OK = FALSE;

						/* Quit */
						break;
					}
				}
			}
			else
			{
				/* Failed to read scratchpad */
				read_OK = FALSE;
			}
		}
		else
		{
			/* Failed to write SKIP ROM */
			read_OK = FALSE;
		}
	}
	else
	{
		/* 1-Wire reset failed */
		read_OK = FALSE;
	}

	return read_OK;
}

/* Maxim/Dallas 8-bit CRC calculation for DS18B20 */
static uint8_t DS18B20_calculate_CRC(const uint8_t *data, uint8_t len)
{
    uint8_t crc = DS18B20_CRC_INIT_VALUE;

    for(uint8_t i = 0; i < len; i++)
	{
        uint8_t inbyte = data[i];

        for(uint8_t j = 0; j < 8; j++)
		{
            if((crc ^ inbyte) & DS18B20_CRC_MSB_MASK)
            {
        		crc = (crc >> 1) ^ DS18B20_CRC_POLYNOMIAL;
            }
            else
            {
                crc >>= 1;
            }

            inbyte >>= 1;
        }
    }

    return crc;
}
