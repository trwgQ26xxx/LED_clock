/*
 * display_drv.c
 *
 *  Created on: Feb 8, 2025
 *      Author: trwgQ26xxx
 */

#include "display_drv.h"

#include "../Core/Inc/spi.h"

/* Number of displays in the chain */
#define NUM_OF_DISPLAYS			3

/* Number of digits in each of the display */
#define NUM_OF_DIGITS			8

/* MAX7219 registers */
#define DIGIT0_REG_ADDR			0x01
#define DIGIT_REG_ADDR_STEP		0x01
#define DIGIT1_REG_ADDR			0x02
#define DIGIT2_REG_ADDR			0x03
#define DIGIT3_REG_ADDR			0x04
#define DIGIT4_REG_ADDR			0x05
#define DIGIT5_REG_ADDR			0x06
#define DIGIT6_REG_ADDR			0x07
#define DIGIT7_REG_ADDR			0x08

#define DECODE_MODE_REG_ADDR	0x09
#define INTENSITY_REG_ADDR		0x0A
#define SCAN_LIMIT_REG_ADDR		0x0B
#define SHUTDOWN_REG_ADDR		0x0C
#define DISPLAY_TEST_REG_ADDR	0x0F

/* Time display chars */
#define TIME_COLON_ON			0x60
#define TIME_COLON_OFF			0x00

#define TIME_DP_ON				0x80
#define TIME_DP_OFF				0x00

#define TIME_E_SIGN				0x4F
#define TIME_r_SIGN				0x05
#define TIME_o_SIGN				0x1D

/* Date display chars */
#define DATE_DP_ON				0x20
#define DATE_DP_OFF				0x00

#define DATE_DEG_SIGN			0x0F
#define DATE_C_SIGN				0x99
#define DATE_I_SIGN				0x18
#define DATE_MINUS_SIGN			0x04

/* Common for all */
#define BLANK_DISP				0x00

/* pabcdefg configuration */
const uint8_t seg_table_hour[10]				= {0x7E, 0x30, 0x6D, 0x79, 0x33, 0x5B, 0x5F, 0x70, 0x7F, 0x7B};
/* dcpefgba configuration */
const uint8_t seg_table_date_temperature[10]	= {0xDB, 0x42, 0x97, 0xC7, 0x4E, 0xCD, 0xDD, 0x43, 0xDF, 0xCF};

inline static void Convert_display_data_to_segments(volatile struct display_data_struct *data, uint8_t *hour_buffer, uint8_t *date_buffer, uint8_t *temp_buffer);
inline static void Override_display_data_for_special_mode(volatile struct display_data_struct *data, uint8_t *hour_buffer, uint8_t *date_buffer, uint8_t *temp_buffer);
inline static void Blank_segments_buffer(uint8_t *digits_data);
inline static void Blank_DP_in_segments_buffer(uint8_t *digits_data, uint8_t dp);

inline static void Uint8_to_two_7segments_with_blanking(uint8_t val, uint8_t *first_digit, uint8_t *second_digit, const uint8_t *seg_table);
inline static void Uint8_to_two_7segments_without_blanking(uint8_t val, uint8_t *first_digit, uint8_t *second_digit, const uint8_t *seg_table);

inline static void Clear(void);
inline static void Set_config(uint8_t intensity);
inline static void Write_CMD_to_all_displays(uint8_t reg, uint8_t val);

inline static void SPI_Flush(void);
inline static void SPI_Send(uint8_t data);
inline static void LCD_Delay(void);


void Init_display(uint8_t intensity)
{
	/* Enable SPI */
	LL_SPI_Enable(SPI1);

	/* Clear displays */
	Clear();

	/* Set configuration */
	Set_config(intensity);
}

void Update_display_config(volatile struct display_data_struct *data)
{
	/* Update displays configuration */
	Set_config(data->intensity);
}

void Update_display_data(volatile struct display_data_struct *data)
{
	uint8_t hour_digits_data[NUM_OF_DIGITS];
	uint8_t date_digits_data[NUM_OF_DIGITS];
	uint8_t temp_digits_data[NUM_OF_DIGITS];

	/* Convert constant fields */
	Convert_display_data_to_segments(data, hour_digits_data, date_digits_data, temp_digits_data);

	/* Update display for special mode */
	Override_display_data_for_special_mode(data, hour_digits_data, date_digits_data, temp_digits_data);

	/* Update display */
	for(uint8_t i = 0; i < NUM_OF_DIGITS; i++)
	{
		LL_GPIO_ResetOutputPin(LED_CS_GPIO_Port, LED_CS_Pin);
		LCD_Delay();

		SPI_Send(i+1); SPI_Send(temp_digits_data[i]);		// Temperature display, last in chain
		SPI_Send(i+1); SPI_Send(date_digits_data[i]);		// Date display
		SPI_Send(i+1); SPI_Send(hour_digits_data[i]);		// Hour display, first in chain

		LCD_Delay();
		LL_GPIO_SetOutputPin(LED_CS_GPIO_Port, LED_CS_Pin);
		LCD_Delay();
	}
}

inline static void Convert_display_data_to_segments(volatile struct display_data_struct *data, uint8_t *hour_buffer, uint8_t *date_buffer, uint8_t *temp_buffer)
{
	uint8_t temp;

	/* Convert time */
	Uint8_to_two_7segments_with_blanking(data->hour,		&hour_buffer[0], &hour_buffer[1], seg_table_hour);
	Uint8_to_two_7segments_without_blanking(data->minute,	&hour_buffer[3], &hour_buffer[4], seg_table_hour);
	Uint8_to_two_7segments_without_blanking(data->second,	&hour_buffer[6], &hour_buffer[7], seg_table_hour);

	/* Manage colon */
	if(data->hour_colon == TRUE)
	{
		hour_buffer[2] = TIME_COLON_ON; hour_buffer[5] = TIME_COLON_ON;
	}
	else
	{
		hour_buffer[2] = TIME_COLON_OFF; hour_buffer[5] = TIME_COLON_OFF;
	}

	/* Convert date */
	Uint8_to_two_7segments_with_blanking(data->date,		&date_buffer[0], &date_buffer[1], seg_table_date_temperature);
	date_buffer[1] |= DATE_DP_ON;	/* Set decimal point at the date */

	Uint8_to_two_7segments_without_blanking(data->month,	&date_buffer[2], &date_buffer[3], seg_table_date_temperature);
	date_buffer[3] |= DATE_DP_ON;	/* Set decimal point at the month */

	date_buffer[4] = seg_table_date_temperature[2]; date_buffer[5] = seg_table_date_temperature[0]; /* Set year prefix to 20 */
	Uint8_to_two_7segments_without_blanking(data->year,		&date_buffer[6], &date_buffer[7], seg_table_date_temperature);

	/* Convert temperature */
	temp_buffer[0] = BLANK_DISP; /* Blank first display, as only the internal temperature is implemented for now */

	/* Manage negative value */
	if(data->int_temperature < 0)
	{
		temp = -data->int_temperature;
		temp_buffer[1] = DATE_MINUS_SIGN;
	}
	else
	{
		temp = data->int_temperature;
		temp_buffer[1] = BLANK_DISP;
	}

	/* Show temperature */
	Uint8_to_two_7segments_with_blanking(temp,		&temp_buffer[2], &temp_buffer[3], seg_table_date_temperature);

	/* Add *C sign */
	temp_buffer[4] = DATE_DEG_SIGN; temp_buffer[5] = DATE_C_SIGN;

	/* Blank not mounted displays */
	temp_buffer[6] = BLANK_DISP; temp_buffer[7] = BLANK_DISP;
}

inline static void Override_display_data_for_special_mode(volatile struct display_data_struct *data, uint8_t *hour_buffer, uint8_t *date_buffer, uint8_t *temp_buffer)
{
	switch(data->special_mode)
	{
	case DISPLAY_INT_TEMP:
	case DISPLAY_EXT_TEMP:

		/* No changes */

		break;

	case DISPLAY_SET_HOUR:

		/* Put dots under hour */
		hour_buffer[0] |= TIME_DP_ON; hour_buffer[1] |= TIME_DP_ON;

		/* Blank colons */
		hour_buffer[2] = TIME_COLON_OFF; hour_buffer[5] = TIME_COLON_OFF;

		/* Blank temperature display */
		Blank_segments_buffer(temp_buffer);

		break;

	case DISPLAY_SET_MINUTE:

		/* Put dots under minute */
		hour_buffer[3] |= TIME_DP_ON; hour_buffer[4] |= TIME_DP_ON;

		/* Blank colons */
		hour_buffer[2] = TIME_COLON_OFF; hour_buffer[5] = TIME_COLON_OFF;

		/* Blank temperature display */
		Blank_segments_buffer(temp_buffer);

		break;

	case DISPLAY_SET_SECOND:

		/* Put dots under second */
		hour_buffer[6] |= TIME_DP_ON; hour_buffer[7] |= TIME_DP_ON;

		/* Blank colons */
		hour_buffer[2] = TIME_COLON_OFF; hour_buffer[5] = TIME_COLON_OFF;

		/* Blank temperature display */
		Blank_segments_buffer(temp_buffer);

		break;

	case DISPLAY_SET_DATE:

		/* Blank decimal places in date */
		Blank_DP_in_segments_buffer(date_buffer, DATE_DP_ON);

		/* Put dots under date */
		date_buffer[0] |= DATE_DP_ON; date_buffer[1] |= DATE_DP_ON;

		/* Blank colons */
		hour_buffer[2] = TIME_COLON_OFF; hour_buffer[5] = TIME_COLON_OFF;

		/* Blank temperature display */
		Blank_segments_buffer(temp_buffer);

		break;

	case DISPLAY_SET_MONTH:

		/* Blank decimal places in date */
		Blank_DP_in_segments_buffer(date_buffer, DATE_DP_ON);

		/* Put dots under month */
		date_buffer[2] |= DATE_DP_ON; date_buffer[3] |= DATE_DP_ON;

		/* Blank colons */
		hour_buffer[2] = TIME_COLON_OFF; hour_buffer[5] = TIME_COLON_OFF;

		/* Blank temperature display */
		Blank_segments_buffer(temp_buffer);

		break;

	case DISPLAY_SET_YEAR:

		/* Blank decimal places in date */
		Blank_DP_in_segments_buffer(date_buffer, DATE_DP_ON);

		/* Put dots under year */
		date_buffer[4] |= DATE_DP_ON; date_buffer[5] |= DATE_DP_ON;
		date_buffer[6] |= DATE_DP_ON; date_buffer[7] |= DATE_DP_ON;

		/* Blank colons */
		hour_buffer[2] = TIME_COLON_OFF; hour_buffer[5] = TIME_COLON_OFF;

		/* Blank temperature display */
		Blank_segments_buffer(temp_buffer);

		break;

	case DISPLAY_INTENSITY:

		/* Blank temperature display */
		Blank_segments_buffer(temp_buffer);

		/* Show intensity */
		temp_buffer[1] = DATE_MINUS_SIGN; temp_buffer[4] = DATE_MINUS_SIGN;
		Uint8_to_two_7segments_with_blanking(data->intensity + 1, &temp_buffer[2], &temp_buffer[3], seg_table_date_temperature);

		break;

	case DISPLAY_DEMO:

		/* Override hour display */
		hour_buffer[0] = TIME_E_SIGN;
		hour_buffer[1] = TIME_r_SIGN;
		hour_buffer[3] = TIME_r_SIGN;
		hour_buffer[4] = TIME_o_SIGN;
		hour_buffer[6] = TIME_r_SIGN;
		hour_buffer[7] = TIME_r_SIGN;

		break;
	}
}

inline static void Blank_segments_buffer(uint8_t *digits_data)
{
	for(uint8_t i = 0; i < NUM_OF_DIGITS; i++)
	{
		digits_data[i] = BLANK_DISP;
	}
}

inline static void Blank_DP_in_segments_buffer(uint8_t *digits_data, uint8_t dp)
{
	for(uint8_t i = 0; i < NUM_OF_DIGITS; i++)
	{
		digits_data[i] &= ~dp;
	}
}

inline static void Uint8_to_two_7segments_with_blanking(uint8_t val, uint8_t *first_digit, uint8_t *second_digit, const uint8_t * seg_table)
{
	/* Limit value to 99 */
	if(val > 99)
	{
		val = 99;
	}

	/* Are two digits for display ? */
	if(val >= 10)
	{
		/* Yes */
		*first_digit	= seg_table[val / 10];
		*second_digit	= seg_table[val % 10];
	}
	else
	{
		/* No, blank first display */
		*first_digit	= BLANK_DISP;
		*second_digit	= seg_table[val];
	}
}

inline static void Uint8_to_two_7segments_without_blanking(uint8_t val, uint8_t *first_digit, uint8_t *second_digit, const uint8_t * seg_table)
{
	/* Limit value to 99 */
	if(val > 99)
	{
		val = 99;
	}

	/* Display always two digits */
	*first_digit	= seg_table[val / 10];
	*second_digit	= seg_table[val % 10];
}

inline static void Clear(void)
{
	for(uint8_t i = 0; i < NUM_OF_DIGITS; i++)
	{
		Write_CMD_to_all_displays(i+1,  BLANK_DISP);
	}
}

inline static void Set_config(uint8_t intensity)
{
	Write_CMD_to_all_displays(SHUTDOWN_REG_ADDR, 0x01);

	Write_CMD_to_all_displays(SCAN_LIMIT_REG_ADDR, 0x07);

	Write_CMD_to_all_displays(DISPLAY_TEST_REG_ADDR, 0x00);
	Write_CMD_to_all_displays(DECODE_MODE_REG_ADDR, 0x00);

	Write_CMD_to_all_displays(INTENSITY_REG_ADDR, (intensity & 0x0F));
}

inline static void Write_CMD_to_all_displays(uint8_t reg, uint8_t val)
{
	LL_GPIO_ResetOutputPin(LED_CS_GPIO_Port, LED_CS_Pin);
	LCD_Delay();

	for(uint8_t i = 0; i < NUM_OF_DISPLAYS; i++)
	{
		SPI_Send(reg); SPI_Send(val);
	}

	LCD_Delay();
	LL_GPIO_SetOutputPin(LED_CS_GPIO_Port, LED_CS_Pin);
	LCD_Delay();
}


inline static void SPI_Flush(void)
{
	/* Flush buffer */
	while(LL_SPI_IsActiveFlag_RXNE(SPI1))
	{
		LL_SPI_ReceiveData8(SPI1);
	}
}

inline static void SPI_Send(uint8_t data)
{
	/* Wait for not empty */
	while(!LL_SPI_IsActiveFlag_TXE(SPI1));

	/* Send byte */
	LL_SPI_TransmitData8(SPI1, data);

	/* Wait while busy */
	while(LL_SPI_IsActiveFlag_BSY(SPI1));

	/* Flush SPI */
	SPI_Flush();
}

inline static void LCD_Delay(void)
{
	for(uint8_t i = 0; i < 150; i++)
	{
		asm volatile("nop");
	}
}
