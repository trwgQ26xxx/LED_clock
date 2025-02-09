/*
 * flash_drv.c
 *
 *  Created on: Feb 9, 2025
 *      Author: trwgQ26xxx
 */

#include "flash_drv.h"

#include "display_drv.h"

#include <string.h>
#include "../Drivers/CMSIS/Device/ST/STM32F0xx/Include/stm32f0xx.h"

volatile struct settings_struct flash_settings __attribute__((__section__(".flash_settings_section")));

#define FLASH_FKEY1 0x45670123
#define FLASH_FKEY2 0xCDEF89AB

void Flash_unlock(void)
{
	/* (1) Wait till no operation is on going */
	/* (2) Check that the flash memory is unlocked */
	/* (3) Perform unlock sequence */
	while ((FLASH->SR & FLASH_SR_BSY) != 0) /* (1) */
	{
		/* For robust implementation, add here time-out management */
	}

	if ((FLASH->CR & FLASH_CR_LOCK) != 0) /* (2) */
	{
		FLASH->KEYR = FLASH_FKEY1; /* (3) */
		FLASH->KEYR = FLASH_FKEY2;
	}
}

void Flash_lock(void)
{
	FLASH->CR |= FLASH_CR_LOCK;
}

void Flash_program(volatile struct settings_struct *s)
{
	volatile uint16_t *src_addr = (uint16_t *)s;
	volatile uint16_t *dst_addr = (uint16_t *)(&flash_settings);

	/* (1) Set the PG bit in the FLASH_CR register to enable programming */
	/* (2) Perform the data write (half-word) at the desired address */
	/* (3) Wait until the BSY bit is reset in the FLASH_SR register */
	/* (4) Check the EOP flag in the FLASH_SR register */
	/* (5) clear it by software by writing it at 1 */
	/* (6) Reset the PG Bit to disable programming */
	FLASH->CR |= FLASH_CR_PG; /* (1) */

	for(volatile uint32_t i = 0; i < (sizeof(flash_settings) / sizeof(uint16_t)); i++)
	{
		*dst_addr = *src_addr;/* (2) */

		while ((FLASH->SR & FLASH_SR_BSY) != 0) /* (3) */
		{
		/* For robust implementation, add here time-out management */
		}

		if ((FLASH->SR & FLASH_SR_EOP) != 0) /* (4) */
		{
			FLASH->SR = FLASH_SR_EOP; /* (5) */
		}
		else
		{
			/* Manage the error cases */
		}

		src_addr++;
		dst_addr++;
	}

	FLASH->CR &= ~FLASH_CR_PG; /* (6) */
}

void Page_erase(void)
{
	/* (1) Set the PER bit in the FLASH_CR register to enable page erasing */
	/* (2) Program the FLASH_AR register to select a page to erase */
	/* (3) Set the STRT bit in the FLASH_CR register to start the erasing */
	/* (4) Wait until the BSY bit is reset in the FLASH_SR register */
	/* (5) Check the EOP flag in the FLASH_SR register */
	/* (6) Clear EOP flag by software by writing EOP at 1 */
	/* (7) Reset the PER Bit to disable the page erase */
	FLASH->CR |= FLASH_CR_PER; /* (1) */
	FLASH->AR = (uint32_t)(&flash_settings); /* (2) */
	FLASH->CR |= FLASH_CR_STRT; /* (3) */

	while ((FLASH->SR & FLASH_SR_BSY) != 0) /* (4) */
	{
		/* For robust implementation, add here time-out management */
	}

	if ((FLASH->SR & FLASH_SR_EOP) != 0) /* (5) */
	{
		FLASH->SR = FLASH_SR_EOP; /* (6)*/
	}
	else
	{
		/* Manage the error cases */
	}

	FLASH->CR &= ~FLASH_CR_PER; /* (7) */
}

void Init_flash(void)
{
	struct settings_struct temp_settings;

	/* Check settings */
	static_assert((sizeof(flash_settings) % sizeof(uint16_t)) == 0,
			"Flash settings structure has to be a multiple of uint16_t!");

	Read_settings(&temp_settings);

	/* Check CRC */

	/* Check data validity */
	if((temp_settings.intensity > MAX_INTENSITY) || (temp_settings.intensity < MIN_INTENSITY))
	{
		/* Assign default value */
		temp_settings.intensity = (MAX_INTENSITY + MIN_INTENSITY) / 2;

		/* Store settings */
		Write_settings(&temp_settings);
	}
}

void Read_settings(volatile struct settings_struct *s)
{
	memcpy((void *)s, (void *)&flash_settings, sizeof(flash_settings));
}

void Write_settings(volatile struct settings_struct *s)
{
	/* Update CRC */

	Flash_unlock();

	Page_erase();

	Flash_program(s);

	Flash_lock();
}
