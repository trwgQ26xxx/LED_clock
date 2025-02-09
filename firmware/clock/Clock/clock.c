/*
 * clock.c
 *
 *  Created on: Feb 8, 2025
 *      Author: trwgQ26xxx
 */

#include "clock.h"

#include "common_definitons.h"

#include "display_drv.h"
#include "rtc_drv.h"
#include "kbd_drv.h"

enum CLOCK_MODES {NORMAL = 0, HOUR_SET, MINUTE_SET, SECOND_SET, DATE_SET, MONTH_SET, YEAR_SET, DEMO};

volatile uint8_t current_clock_mode = NORMAL;

volatile uint8_t current_intensity = (MAX_INTENSITY + MIN_INTENSITY) / 2;

volatile struct rtc_data_struct rtc_data;
volatile struct display_data_struct display_data;

volatile uint8_t keyboard_is_locked = FALSE;

volatile uint8_t read_rtc_flag = FALSE;
volatile uint8_t update_display_flag = FALSE;

inline static void Normal_mode(void);
inline static void Hour_set_mode(void);
inline static void Minute_set_mode(void);
inline static void Second_set_mode(void);
inline static void Date_set_mode(void);
inline static void Month_set_mode(void);
inline static void Year_set_mode(void);

void Lock_keyboard(void);
void Manage_keyboard_unlock(void);

inline void Inc_value(volatile uint8_t *val, uint8_t max);
inline void Dec_value(volatile uint8_t *val, uint8_t min);

inline void Inc_value_with_rewind(volatile uint8_t *val, uint8_t min, uint8_t max);
inline void Dec_value_with_rewind(volatile uint8_t *val, uint8_t min, uint8_t max);

void Set_read_RTC_flag(void)
{
	read_rtc_flag = TRUE;

	LL_TIM_ClearFlag_UPDATE(TIM16);

	NVIC_ClearPendingIRQ(TIM16_IRQn);
}

void Set_update_display_flag(void)
{
	update_display_flag = TRUE;

	LL_TIM_ClearFlag_UPDATE(TIM14);

	NVIC_ClearPendingIRQ(TIM14_IRQn);
}

void Init(void)
{
	/* Initialize display */
	Init_display(current_intensity);

	/* Initialize keyboard */
	ENABLE_KBD_TIMER;
	CLEAR_KBD_TIMER;

	/* Initialize RTC */
	Init_RTC();

	/* TODO: read intensity from flash */

	/* Enable Timer 16 to read RTC 4 times per second */
	LL_TIM_SetCounter(TIM16, 0);
	LL_TIM_EnableCounter(TIM16);
	LL_TIM_ClearFlag_UPDATE(TIM16);
	LL_TIM_EnableIT_UPDATE(TIM16);

	/* Enable Timer 14 to update display 10 times per second */
	LL_TIM_SetCounter(TIM14, 0);
	LL_TIM_EnableCounter(TIM14);
	LL_TIM_ClearFlag_UPDATE(TIM14);
	LL_TIM_EnableIT_UPDATE(TIM14);

	/* Clear display data structure */
	display_data.hour = 0;
	display_data.minute = 0;
	display_data.second = 0;

	display_data.hour_colon = TRUE;

	display_data.date = 1;
	display_data.month = 1;
	display_data.year = 0;

	display_data.int_temperature = 0;

	display_data.special_mode = DISPLAY_INT_TEMP;
}

void Run(void)
{
	/* Handle current mode */
	switch(current_clock_mode)
	{
		case NORMAL:
			Normal_mode();
			break;

		case HOUR_SET:
			Hour_set_mode();
			break;

		case MINUTE_SET:
			Minute_set_mode();
			break;

		case SECOND_SET:
			Second_set_mode();
			break;

		case DATE_SET:
			Date_set_mode();
			break;

		case MONTH_SET:
			Month_set_mode();
			break;

		case YEAR_SET:
			Year_set_mode();
			break;

		case DEMO:
			Normal_mode();
			break;

		default:
			Normal_mode();
			break;
	}

	/* Handle keyboard */
	Manage_keyboard_unlock();

	/* Handle display */
	if(update_display_flag == TRUE)
	{
		/* Clear flag */
		update_display_flag = FALSE;

		/* Perform update */
		Update_display(&display_data);
	}
}

inline static void Normal_mode(void)
{
	/* Is RTC flag set ? */
	if(read_rtc_flag == TRUE)
	{
		/* Clear flag */
		read_rtc_flag = FALSE;

		/* Get data from RTC */
		Get_RTC_data(&rtc_data);

		/* Copy data from RTC */
		display_data.second				= rtc_data.second;
		display_data.minute				= rtc_data.minute;
		display_data.hour				= rtc_data.hour;
		display_data.date				= rtc_data.date;
		display_data.month				= rtc_data.month;
		display_data.year				= rtc_data.year;
		display_data.int_temperature	= rtc_data.temperature;

		/* Manage colon */
		display_data.hour_colon = rtc_data.second % 2 == 1 ? FALSE : TRUE;
	}

	if(current_clock_mode == NORMAL)
	{
		/* Manage temperature display */
		/* TODO: Switch between ext and in every 2s */
		display_data.special_mode = DISPLAY_INT_TEMP;
	}
	else
	{
		display_data.special_mode = DISPLAY_DEMO;
	}

	/* Check if keyboard is unlocked */
	if(keyboard_is_locked == FALSE)
	{
		if(ENTER_KEY_IS_PRESSED)
		{
			current_clock_mode = HOUR_SET;

			Lock_keyboard();
		}
		else if(PLUS_KEY_IS_PRESSED)
		{
			/* Increment display intensity */
			Inc_value(&current_intensity, MAX_INTENSITY);

			Set_intensity(current_intensity);

			//Mark to save after 2s

			Lock_keyboard();
		}
		else if(MINUS_KEY_IS_PRESSED)
		{
			/* Decrement display intensity */
			Dec_value(&current_intensity, MIN_INTENSITY);

			Set_intensity(current_intensity);

			//Mark to save it after 2s

			Lock_keyboard();
		}
		else if(ESC_KEY_IS_PRESSED)
		{
			/* Toggle modes between normal and demo */
			if(current_clock_mode == NORMAL)
			{
				current_clock_mode = DEMO;
			}
			else
			{
				current_clock_mode = NORMAL;
			}

			Lock_keyboard();
		}
	}
}

inline static void Hour_set_mode(void)
{
	/* Display hour set mode */
	display_data.special_mode = DISPLAY_SET_HOUR;

	/* Check if keyboard is unlocked */
	if(keyboard_is_locked == FALSE)
	{
		/* Handle keys */
		if(ENTER_KEY_IS_PRESSED)
		{
			current_clock_mode = MINUTE_SET;

			Lock_keyboard();
		}
		else if(PLUS_KEY_IS_PRESSED)
		{
			/* Increment hour */
			Inc_value_with_rewind(&display_data.hour, 0, 23);

			Lock_keyboard();
		}
		else if(MINUS_KEY_IS_PRESSED)
		{
			/* Decrement hour */
			Dec_value_with_rewind(&display_data.hour, 0, 23);

			Lock_keyboard();
		}
		else if(ESC_KEY_IS_PRESSED)
		{
			current_clock_mode = NORMAL;

			Lock_keyboard();
		}
	}
}

inline static void Minute_set_mode(void)
{
	/* Display minute set mode */
	display_data.special_mode = DISPLAY_SET_MINUTE;

	/* Check if keyboard is unlocked */
	if(keyboard_is_locked == FALSE)
	{
		/* Handle keys */
		if(ENTER_KEY_IS_PRESSED)
		{
			current_clock_mode = SECOND_SET;

			Lock_keyboard();
		}
		else if(PLUS_KEY_IS_PRESSED)
		{
			/* Increment minute */
			Inc_value_with_rewind(&display_data.minute, 0, 59);

			Lock_keyboard();
		}
		else if(MINUS_KEY_IS_PRESSED)
		{
			/* Decrement minute */
			Dec_value_with_rewind(&display_data.minute, 0, 59);

			Lock_keyboard();
		}
		else if(ESC_KEY_IS_PRESSED)
		{
			current_clock_mode = NORMAL;

			Lock_keyboard();
		}
	}
}

inline static void Second_set_mode(void)
{
	/* Display second set mode */
	display_data.special_mode = DISPLAY_SET_SECOND;

	/* Check if keyboard is unlocked */
	if(keyboard_is_locked == FALSE)
	{
		/* Handle keys */
		if(ENTER_KEY_IS_PRESSED)
		{
			current_clock_mode = DATE_SET;

			Lock_keyboard();
		}
		else if(PLUS_KEY_IS_PRESSED || MINUS_KEY_IS_PRESSED)
		{
			/* Set seconds to 0 */
			display_data.second = 0;

			Lock_keyboard();
		}
		else if(ESC_KEY_IS_PRESSED)
		{
			current_clock_mode = NORMAL;

			Lock_keyboard();
		}
	}
}

inline static void Date_set_mode(void)
{
	/* Display date set mode */
	display_data.special_mode = DISPLAY_SET_DATE;

	/* Check if keyboard is unlocked */
	if(keyboard_is_locked == FALSE)
	{
		/* Handle keys */
		if(ENTER_KEY_IS_PRESSED)
		{
			current_clock_mode = MONTH_SET;

			Lock_keyboard();
		}
		else if(PLUS_KEY_IS_PRESSED)
		{
			/* Increment minute */
			Inc_value_with_rewind(&display_data.date, 1, 31);

			Lock_keyboard();
		}
		else if(MINUS_KEY_IS_PRESSED)
		{
			/* Decrement minute */
			Dec_value_with_rewind(&display_data.date, 1, 31);

			Lock_keyboard();
		}
		else if(ESC_KEY_IS_PRESSED)
		{
			current_clock_mode = NORMAL;

			Lock_keyboard();
		}
	}
}

inline static void Month_set_mode(void)
{
	/* Display month set mode */
	display_data.special_mode = DISPLAY_SET_MONTH;

	/* Check if keyboard is unlocked */
	if(keyboard_is_locked == FALSE)
	{
		/* Handle keys */
		if(ENTER_KEY_IS_PRESSED)
		{
			current_clock_mode = YEAR_SET;

			Lock_keyboard();
		}
		else if(PLUS_KEY_IS_PRESSED)
		{
			/* Increment minute */
			Inc_value_with_rewind(&display_data.month, 1, 12);

			Lock_keyboard();
		}
		else if(MINUS_KEY_IS_PRESSED)
		{
			/* Decrement minute */
			Dec_value_with_rewind(&display_data.month, 1, 12);

			Lock_keyboard();
		}
		else if(ESC_KEY_IS_PRESSED)
		{
			current_clock_mode = NORMAL;

			Lock_keyboard();
		}
	}
}

inline static void Year_set_mode(void)
{
	/* Display year set mode */
	display_data.special_mode = DISPLAY_SET_YEAR;

	/* Check if keyboard is unlocked */
	if(keyboard_is_locked == FALSE)
	{
		/* Handle keys */
		if(ENTER_KEY_IS_PRESSED)
		{
			/* Copy data for RTC */
			rtc_data.second	= display_data.second;
			rtc_data.minute	= display_data.minute;
			rtc_data.hour	= display_data.hour;
			rtc_data.date	= display_data.date;
			rtc_data.month	= display_data.month;
			rtc_data.year	= display_data.year;

			/* Store data in RTC */
			Set_RTC_time(&rtc_data);

			current_clock_mode = NORMAL;

			Lock_keyboard();
		}
		else if(PLUS_KEY_IS_PRESSED)
		{
			/* Increment minute */
			Inc_value_with_rewind(&display_data.year, 0, 99);

			Lock_keyboard();
		}
		else if(MINUS_KEY_IS_PRESSED)
		{
			/* Decrement minute */
			Dec_value_with_rewind(&display_data.year, 0, 99);

			Lock_keyboard();
		}
		else if(ESC_KEY_IS_PRESSED)
		{
			current_clock_mode = NORMAL;

			Lock_keyboard();
		}
	}
}

void Lock_keyboard(void)
{
	keyboard_is_locked = TRUE;

	CLEAR_KBD_TIMER;
}

void Manage_keyboard_unlock(void)
{
	/* Check if keyboard is locked */
	if(keyboard_is_locked == TRUE)
	{
		/* It is, is debounce time passed ? */
		if(GET_KBD_TIMER > KEYBOARD_DEBOUNCE_TIME)
		{
			/* Yes, check if all keys are released */
			if(ALL_KEYS_ARE_RELEASED)
			{
				/* Yes, unlock */
				keyboard_is_locked = FALSE;
			}
			else
			{
				/* No, clear timer */
				CLEAR_KBD_TIMER;
			}
		}
	}
}

inline void Inc_value(volatile uint8_t *val, uint8_t max)
{
	if((*val) < max)
		(*val)++;
}

inline void Dec_value(volatile uint8_t *val, uint8_t min)
{
	if((*val) > min)
		(*val)--;
}

inline void Inc_value_with_rewind(volatile uint8_t *val, uint8_t min, uint8_t max)
{
	if(*val == max)
		*val = min;
	else
		(*val)++;
}

inline void Dec_value_with_rewind(volatile uint8_t *val, uint8_t min, uint8_t max)
{
	if(*val == min)
		*val = max;
	else
		(*val)--;
}
