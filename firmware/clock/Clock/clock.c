/*
 * clock.c
 *
 *  Created on: Feb 8, 2025
 *      Author: trwgQ26xxx
 */

#include "clock.h"

#include "common_defs.h"
#include "common_fcns.h"

#include "display_drv.h"
#include "rtc_drv.h"
#include "kbd_drv.h"
#include "flash_drv.h"
#include "onewire_bridge_drv.h"
#include "ext_temp_sens_drv.h"

#include "../Core/Inc/iwdg.h"

enum CLOCK_MODES
{
	NORMAL = 0,
	HOUR_SET, MINUTE_SET, SECOND_SET,
	DATE_SET, MONTH_SET, YEAR_SET,
	INTENSITY_SET,
	DEMO
};

volatile uint8_t current_clock_mode = NORMAL;

volatile struct rtc_data_struct		rtc_data;
volatile struct display_data_struct	display_data;
volatile struct settings_struct		clock_settings;

volatile uint8_t	keyboard_is_locked = FALSE;

volatile uint8_t	store_settings_flag = FALSE;
volatile uint32_t	store_settings_delay_counter = 0;

volatile uint32_t	clock_set_inactivity_counter = 0;

volatile uint8_t	halt_rtc_read = FALSE;

volatile uint8_t	update_flag = FALSE;
volatile uint32_t	update_counter = 0;

volatile uint8_t	ext_temp_is_present		= FALSE;
volatile uint8_t	ext_temp_conv_triggered	= FALSE;

volatile uint32_t	int_ext_temp_cycling_counter = 0;
volatile uint8_t	int_ext_temp_cycling_flag = FALSE;

inline static void Manage_periodic_updates(void);

inline static void Normal_mode(void);
inline static void Hour_set_mode(void);
inline static void Minute_set_mode(void);
inline static void Second_set_mode(void);
inline static void Date_set_mode(void);
inline static void Month_set_mode(void);
inline static void Year_set_mode(void);

inline static void Lock_keyboard(void);
inline static void Manage_keyboard_unlock(void);

inline static void Set_flag_to_store_settings(void);
inline static void Manage_store_settings(void);

inline static void Clear_clock_set_inactivity_counter(void);
inline static void Manage_clock_set_inactivity(void);

inline static void Clear_int_ext_temp_cycling_counter(void);
inline static void Manage_int_ext_temp_cycling(void);

inline static void Go_to_normal_mode(void);

void Set_update_display_flag(void)
{
	update_flag = TRUE;

	LL_TIM_ClearFlag_UPDATE(TIM14);

	NVIC_ClearPendingIRQ(TIM14_IRQn);
}

void Init(void)
{
	/* Poke WDT */
	LL_IWDG_ReloadCounter(IWDG);

	/* Read settings from flash */
	Read_settings(&clock_settings);

	/* Initialize display */
	Init_display(clock_settings.intensity);

	/* Initialize keyboard */
	ENABLE_KBD_TIMER;
	CLEAR_KBD_TIMER;

	/* Initialize RTC */
	Init_RTC();

	/* Enable Timer 14 to perform periodic updates 32 times per second */
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
	display_data.ext_temperature = 0;

	display_data.special_mode = DISPLAY_INT_TEMP;

	display_data.intensity = clock_settings.intensity;

	/* Initialize 1-Wire bridge and external temperature sensor */
	if(Init_OneWire_bridge() == TRUE)
	{
		/* Update presence flag */
		ext_temp_is_present = Init_ext_temp_sens();
	}

	Clear_int_ext_temp_cycling_counter();

	Go_to_normal_mode();

	/* Disable keyboard to prevent selecting options, */
	/* if any key would be stuck at startup */
	Lock_keyboard();

	/* Poke WDT */
	LL_IWDG_ReloadCounter(IWDG);
}

void Run(void)
{
	/* Manage RTC, LED display updates */
	Manage_periodic_updates();

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

		case INTENSITY_SET:
			Normal_mode();
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

	/* Poke WDT */
	LL_IWDG_ReloadCounter(IWDG);
}

inline static void Manage_periodic_updates(void)
{
	int8_t ext_temp_data = 0;

	/* Check update flag */
	if(update_flag == TRUE)
	{
		/* Check RTC read match */
		if((update_counter % RTC_READ_MODULO) == RTC_READ_OFFSET)
		{
			/* Match. Check if RTC read is not halted */
			if(halt_rtc_read == FALSE)
			{
				/* Not halted */

				/* Get data from RTC */
				if(Get_RTC_data(&rtc_data) == TRUE)
				{
					/* Update only if read was successful */

					/* Copy data from RTC */
					display_data.second				= rtc_data.second;
					display_data.minute				= rtc_data.minute;
					display_data.hour				= rtc_data.hour;
					display_data.date				= rtc_data.date;
					display_data.month				= rtc_data.month;
					display_data.year				= rtc_data.year;
					display_data.int_temperature	= rtc_data.temperature;

					/* Manage colon */
					display_data.hour_colon = ((rtc_data.second % 2) == 1) ? FALSE : TRUE;
				}
			}
		}

		/* Check LED data update match */
		if((update_counter % LED_DATA_UPDATE_MODULO) == LED_DATA_UPDATE_OFFSET)
		{
			/* Perform LED data update */
			Update_display_data(&display_data);
		}

		/* Check LED configuration update match */
		if((update_counter % LED_CFG_UPDATE_MODULO) == LED_CFG_UPDATE_OFFSET)
		{
			/* Perform LED data update */
			Update_display_config(&display_data);
		}

		/* Check external temperature conversion trigger match */
		if(update_counter == EXT_TEMP_CONV_TRIGGER_CNT)
		{
			/* Check if external temperature sensor is present */
			if(ext_temp_is_present == TRUE)
			{
				/* Start external temperature conversion */
				ext_temp_conv_triggered = Ext_temp_start_conversion();
			}
		}

		/* Check external temperature read match */
		if(update_counter == EXT_TEMP_DATA_READ_CNT)
		{
			/* Check if external temperature sensor is present and conversion was triggered */
			if((ext_temp_is_present == TRUE) && (ext_temp_conv_triggered == TRUE))
			{
				/* Read external temperature */
				if(Ext_temp_read_temperature(&ext_temp_data) == TRUE)
				{
					/* Update display data */
					display_data.ext_temperature = ext_temp_data;
				}

				/* Clear conversion triggered flag */
				ext_temp_conv_triggered = FALSE;
			}
		}

		/* Store settings if necessary */
		Manage_store_settings();

		/* Exit clock set mode if no changes were made for 10s */
		Manage_clock_set_inactivity();

		/* Manage internal/external temperature cycling */
		Manage_int_ext_temp_cycling();

		/* Clear flag */
		update_flag = FALSE;

		/* Advance update counter */
		update_counter++;
		if(update_counter >= UPDATE_FREQUENCY)
		{
 			update_counter = 0;
		}
	}
}

inline static void Normal_mode(void)
{
	/* Read RTC */
	halt_rtc_read = FALSE;

	/* Update intensity */
	display_data.intensity = clock_settings.intensity;

	if(current_clock_mode == NORMAL)
	{
		/* Check if time to switch temperatures elapsed */
		if((ext_temp_is_present == TRUE) && (int_ext_temp_cycling_flag == TRUE))
		{
			/* Switch displayed temperature */
			if(display_data.special_mode == DISPLAY_EXT_TEMP)
			{
				display_data.special_mode = DISPLAY_INT_TEMP;
			}
			else
			{
				display_data.special_mode = DISPLAY_EXT_TEMP;
			}

			/* Clear flag */
			int_ext_temp_cycling_flag = FALSE;
		}
	}
	else if(current_clock_mode == INTENSITY_SET)
	{
		display_data.special_mode = DISPLAY_INTENSITY;
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
			/* Halt RTC */
			halt_rtc_read = TRUE;

			/* Go to first position set mode */
			current_clock_mode = HOUR_SET;

			/* Clear inactivity counter */
			Clear_clock_set_inactivity_counter();

			Lock_keyboard();
		}
		else if(PLUS_KEY_IS_PRESSED)
		{
			/* Increment display intensity */
			Inc_value(&clock_settings.intensity, MAX_INTENSITY);

			/* Show intensity */
			current_clock_mode = INTENSITY_SET;

			//Mark to save after 2s
			Set_flag_to_store_settings();

			Lock_keyboard();
		}
		else if(MINUS_KEY_IS_PRESSED)
		{
			/* Decrement display intensity */
			Dec_value(&clock_settings.intensity, MIN_INTENSITY);

			/* Show intensity */
			current_clock_mode = INTENSITY_SET;

			//Mark to save it after 2s
			Set_flag_to_store_settings();

			Lock_keyboard();
		}
		else if(ESC_KEY_IS_PRESSED)
		{
			/* Toggle modes between normal and demo */
			if(current_clock_mode != DEMO)
			{
				current_clock_mode = DEMO;
			}
			else
			{
				Go_to_normal_mode();
			}

			Lock_keyboard();
		}
	}
}

inline static void Hour_set_mode(void)
{
	/* Halt RTC */
	halt_rtc_read = TRUE;

	/* Display hour set mode */
	display_data.special_mode = DISPLAY_SET_HOUR;

	/* Check if keyboard is unlocked */
	if(keyboard_is_locked == FALSE)
	{
		/* Handle keys */
		if(ENTER_KEY_IS_PRESSED)
		{
			/* Go to next setting */
			current_clock_mode = MINUTE_SET;

			/* Clear inactivity counter */
			Clear_clock_set_inactivity_counter();

			Lock_keyboard();
		}
		else if(PLUS_KEY_IS_PRESSED)
		{
			/* Increment hour */
			Inc_value_with_rewind(&display_data.hour, 0, 23);

			/* Clear inactivity counter */
			Clear_clock_set_inactivity_counter();

			Lock_keyboard();
		}
		else if(MINUS_KEY_IS_PRESSED)
		{
			/* Decrement hour */
			Dec_value_with_rewind(&display_data.hour, 0, 23);

			/* Clear inactivity counter */
			Clear_clock_set_inactivity_counter();

			Lock_keyboard();
		}
		else if(ESC_KEY_IS_PRESSED)
		{
			Go_to_normal_mode();

			Lock_keyboard();
		}
	}
}

inline static void Minute_set_mode(void)
{
	/* Halt RTC */
	halt_rtc_read = TRUE;

	/* Display minute set mode */
	display_data.special_mode = DISPLAY_SET_MINUTE;

	/* Check if keyboard is unlocked */
	if(keyboard_is_locked == FALSE)
	{
		/* Handle keys */
		if(ENTER_KEY_IS_PRESSED)
		{
			/* Go to next setting */
			current_clock_mode = SECOND_SET;

			/* Clear inactivity counter */
			Clear_clock_set_inactivity_counter();

			Lock_keyboard();
		}
		else if(PLUS_KEY_IS_PRESSED)
		{
			/* Increment minute */
			Inc_value_with_rewind(&display_data.minute, 0, 59);

			/* Clear inactivity counter */
			Clear_clock_set_inactivity_counter();

			Lock_keyboard();
		}
		else if(MINUS_KEY_IS_PRESSED)
		{
			/* Decrement minute */
			Dec_value_with_rewind(&display_data.minute, 0, 59);

			/* Clear inactivity counter */
			Clear_clock_set_inactivity_counter();

			Lock_keyboard();
		}
		else if(ESC_KEY_IS_PRESSED)
		{
			Go_to_normal_mode();

			Lock_keyboard();
		}
	}
}

inline static void Second_set_mode(void)
{
	/* Halt RTC */
	halt_rtc_read = TRUE;

	/* Display second set mode */
	display_data.special_mode = DISPLAY_SET_SECOND;

	/* Check if keyboard is unlocked */
	if(keyboard_is_locked == FALSE)
	{
		/* Handle keys */
		if(ENTER_KEY_IS_PRESSED)
		{
			/* Go to next setting */
			current_clock_mode = DATE_SET;

			/* Clear inactivity counter */
			Clear_clock_set_inactivity_counter();

			Lock_keyboard();
		}
		else if(PLUS_KEY_IS_PRESSED || MINUS_KEY_IS_PRESSED)
		{
			/* Set seconds to 0 */
			display_data.second = 0;

			/* Clear inactivity counter */
			Clear_clock_set_inactivity_counter();

			Lock_keyboard();
		}
		else if(ESC_KEY_IS_PRESSED)
		{
			Go_to_normal_mode();

			Lock_keyboard();
		}
	}
}

inline static void Date_set_mode(void)
{
	/* Halt RTC */
	halt_rtc_read = TRUE;

	/* Display date set mode */
	display_data.special_mode = DISPLAY_SET_DATE;

	/* Check if keyboard is unlocked */
	if(keyboard_is_locked == FALSE)
	{
		/* Handle keys */
		if(ENTER_KEY_IS_PRESSED)
		{
			/* Go to next setting */
			current_clock_mode = MONTH_SET;

			/* Clear inactivity counter */
			Clear_clock_set_inactivity_counter();

			Lock_keyboard();
		}
		else if(PLUS_KEY_IS_PRESSED)
		{
			/* Increment minute */
			Inc_value_with_rewind(&display_data.date, 1, 31);

			/* Clear inactivity counter */
			Clear_clock_set_inactivity_counter();

			Lock_keyboard();
		}
		else if(MINUS_KEY_IS_PRESSED)
		{
			/* Decrement minute */
			Dec_value_with_rewind(&display_data.date, 1, 31);

			/* Clear inactivity counter */
			Clear_clock_set_inactivity_counter();

			Lock_keyboard();
		}
		else if(ESC_KEY_IS_PRESSED)
		{
			Go_to_normal_mode();

			Lock_keyboard();
		}
	}
}

inline static void Month_set_mode(void)
{
	/* Halt RTC */
	halt_rtc_read = TRUE;

	/* Display month set mode */
	display_data.special_mode = DISPLAY_SET_MONTH;

	/* Check if keyboard is unlocked */
	if(keyboard_is_locked == FALSE)
	{
		/* Handle keys */
		if(ENTER_KEY_IS_PRESSED)
		{
			/* Go to next setting */
			current_clock_mode = YEAR_SET;

			/* Clear inactivity counter */
			Clear_clock_set_inactivity_counter();

			Lock_keyboard();
		}
		else if(PLUS_KEY_IS_PRESSED)
		{
			/* Increment minute */
			Inc_value_with_rewind(&display_data.month, 1, 12);

			/* Clear inactivity counter */
			Clear_clock_set_inactivity_counter();

			Lock_keyboard();
		}
		else if(MINUS_KEY_IS_PRESSED)
		{
			/* Decrement minute */
			Dec_value_with_rewind(&display_data.month, 1, 12);

			/* Clear inactivity counter */
			Clear_clock_set_inactivity_counter();

			Lock_keyboard();
		}
		else if(ESC_KEY_IS_PRESSED)
		{
			Go_to_normal_mode();

			Lock_keyboard();
		}
	}
}

inline static void Year_set_mode(void)
{
	/* Halt RTC */
	halt_rtc_read = TRUE;

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

			Go_to_normal_mode();

			Lock_keyboard();
		}
		else if(PLUS_KEY_IS_PRESSED)
		{
			/* Increment minute */
			Inc_value_with_rewind(&display_data.year, 0, 99);

			/* Clear inactivity counter */
			Clear_clock_set_inactivity_counter();

			Lock_keyboard();
		}
		else if(MINUS_KEY_IS_PRESSED)
		{
			/* Decrement minute */
			Dec_value_with_rewind(&display_data.year, 0, 99);

			/* Clear inactivity counter */
			Clear_clock_set_inactivity_counter();

			Lock_keyboard();
		}
		else if(ESC_KEY_IS_PRESSED)
		{
			Go_to_normal_mode();

			Lock_keyboard();
		}
	}
}

inline static void Lock_keyboard(void)
{
	keyboard_is_locked = TRUE;

	CLEAR_KBD_TIMER;
}

inline static void Manage_keyboard_unlock(void)
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

inline static void Set_flag_to_store_settings(void)
{
	store_settings_flag = TRUE;
	store_settings_delay_counter = 0;
}

inline static void Manage_store_settings(void)
{
	if(store_settings_flag == TRUE)
	{
		store_settings_delay_counter++;
		if(store_settings_delay_counter >= STORE_SETTINGS_CNTR_MAX)
		{
			/* Clear flag */
			store_settings_flag = FALSE;

			/* Store settings */
			Write_settings(&clock_settings);

			/* Check if clock is still @ intensity, because */
			/* It can be in different mode (setting time) */
			if(current_clock_mode == INTENSITY_SET)
			{
				/* Return to normal */
				Go_to_normal_mode();
			}
		}
	}
}

inline static void Clear_clock_set_inactivity_counter(void)
{
	clock_set_inactivity_counter = 0;
}

inline static void Manage_clock_set_inactivity(void)
{
	/* Check if RTC read is halted */
	/* RTC read is halted when clock is being set */
	if(halt_rtc_read == TRUE)
	{
		clock_set_inactivity_counter++;
		if(clock_set_inactivity_counter >= CLOCK_SET_INACTIVITY_CNTR_MAX)
		{
			/* Go back in NORMAL mode */
			Go_to_normal_mode();
		}
	}
}

inline static void Clear_int_ext_temp_cycling_counter(void)
{
	int_ext_temp_cycling_counter = 0;
	int_ext_temp_cycling_flag = FALSE;
}

inline static void Manage_int_ext_temp_cycling(void)
{
	int_ext_temp_cycling_counter++;
	if(int_ext_temp_cycling_counter >= INT_EXT_TEMP_CYCLING_CNTR_MAX)
	{
		/* Reset counter */
		int_ext_temp_cycling_counter = 0;

		/* Set flag to change displayed temperature */
		int_ext_temp_cycling_flag = TRUE;
	}
}

inline static void Go_to_normal_mode(void)
{
	/* Reset internal/external temperature cycling counter */
	Clear_int_ext_temp_cycling_counter();

	/* Set displayed temperature to internal */
	display_data.special_mode = DISPLAY_INT_TEMP;

	/* Go back in NORMAL mode */
	current_clock_mode = NORMAL;
}
