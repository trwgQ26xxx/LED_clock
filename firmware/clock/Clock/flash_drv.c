/*
 * flash_drv.c
 *
 *  Created on: Feb 9, 2025
 *      Author: trwgQ26xxx
 */

#include "flash_drv.h"

#include "common_fcns.h"
#include "display_drv.h"
#include "crc.h"

#include <string.h>
#include "../Drivers/CMSIS/Device/ST/STM32F0xx/Include/stm32f0xx.h"

volatile struct settings_struct flash_settings __attribute__((__section__(".flash_settings_section")));

#define FLASH_SETTINGS_ID		0x7EC9

#define FLASH_UNLOCK_TIMEOUT	2
#define FLASH_ERASE_TIMEOUT		50	/* Typical page erase time is 30ms */
#define FLASH_PROGRAM_TIMEOUT	50	/* Typical program time (single uint16_t) is 53.5us */

#define FLASH_UNLOCK_KEY1		0x45670123
#define FLASH_UNLOCK_KEY2		0xCDEF89AB

uint8_t Verify_settings(struct settings_struct *s);
void Restore_settings_to_default(struct settings_struct *s);

void Flash_read(volatile struct settings_struct *s);
void Flash_program(volatile struct settings_struct *s);
uint8_t Page_erase(void);

uint8_t Flash_unlock(void);
void Flash_lock(void);

void Read_settings(volatile struct settings_struct *s)
{
	struct settings_struct temp_settings;

	/* Check size */
	static_assert((sizeof(flash_settings) % sizeof(uint16_t)) == 0,
			"Flash settings structure has to be a multiple of uint16_t!");

	/* Read settings from flash */
	Flash_read(&temp_settings);

	/* Check settings */
	if(Verify_settings(&temp_settings) == FALSE)
	{
		/* Verification failed, restore to default values */
		Restore_settings_to_default(&temp_settings);

		/* Store settings */
		Write_settings(&temp_settings);
	}

	/* Copy settings */
	memcpy((void *)s, (void *)&temp_settings, sizeof(struct settings_struct));
}

void Write_settings(volatile struct settings_struct *s)
{
	/* Write ID */
	s->ID = FLASH_SETTINGS_ID;

	/* Clear unused field */
	s->unused = 0;

	/* Update CRC */
	s->crc = Calculate_CRC32((uint8_t *)s, sizeof(struct settings_struct) - sizeof(uint32_t));

	if(Flash_unlock() == TRUE)
	{
		if(Page_erase() == TRUE)
		{
			(void)Flash_program(s);
		}
	}

	Flash_lock();
}

uint8_t Verify_settings(struct settings_struct *s)
{
	uint8_t settings_OK = FALSE;

	/* Check ID */
	if(s->ID == FLASH_SETTINGS_ID)
	{
		/* ID OK */

		/* Check CRC */
		if(s->crc == Calculate_CRC32((uint8_t *)s, sizeof(struct settings_struct) - sizeof(uint32_t)))
		{
			/* CRC OK */

			/* Check data validity */

			/* Check intensity setting */
			if((s->intensity < MAX_INTENSITY) && (s->intensity > MIN_INTENSITY))
			{
				/* Settings OK */
				settings_OK = TRUE;
			}
		}
	}

	return settings_OK;
}

void Restore_settings_to_default(struct settings_struct *s)
{
	/* Assign default value */
	s->intensity = (MAX_INTENSITY + MIN_INTENSITY) / 2;
}

void Flash_read(volatile struct settings_struct *s)
{
	memcpy((void *)s, (void *)&flash_settings, sizeof(struct settings_struct));
}

void Flash_program(volatile struct settings_struct *s)
{
	volatile uint16_t *src_addr = (uint16_t *)s;
	volatile uint16_t *dst_addr = (uint16_t *)(&flash_settings);

	uint8_t programmed_OK = TRUE;
	uint32_t timeout = 0;

	/* Set the PG bit in the FLASH_CR register to enable programming */
	FLASH->CR |= FLASH_CR_PG;

	CLEAR_TICK;

	for(volatile uint32_t i = 0; i < (sizeof(flash_settings) / sizeof(uint16_t)); i++)
	{
		/* Perform the data write (half-word) at the desired address */
		*dst_addr = *src_addr;

		/* Wait until the BSY bit is reset in the FLASH_SR register */
		while((FLASH->SR & FLASH_SR_BSY) != 0)
		{
			if(CHECK_TICK)
			{
				timeout++;
			}

			if(timeout >= FLASH_PROGRAM_TIMEOUT)
			{
				programmed_OK = FALSE;
				break;
			}
		}

		/* Check the EOP flag in the FLASH_SR register */
		if((FLASH->SR & FLASH_SR_EOP) != 0)
		{
			/* Clear EOP flag */
			FLASH->SR = FLASH_SR_EOP;
		}
		else
		{
			/* Operation failed */
			programmed_OK = FALSE;
		}

		src_addr++;
		dst_addr++;

		if(programmed_OK == FALSE)
		{
			break;
		}
	}

	/* Reset the PG Bit to disable programming */
	FLASH->CR &= ~FLASH_CR_PG;
}

uint8_t Page_erase(void)
{
	uint8_t erased_OK = TRUE;
	uint32_t timeout = 0;

	/* Set the PER bit in the FLASH_CR register to enable page erasing */
	FLASH->CR |= FLASH_CR_PER;

	/* Program the FLASH_AR register to select a page to erase */
	FLASH->AR = (uint32_t)(&flash_settings);

	/* Set the STRT bit in the FLASH_CR register to start the erasing */
	FLASH->CR |= FLASH_CR_STRT;

	CLEAR_TICK;

	/* Wait until the BSY bit is reset in the FLASH_SR register */
	while((FLASH->SR & FLASH_SR_BSY) != 0)
	{
		if(CHECK_TICK)
		{
			timeout++;
		}

		if(timeout >= FLASH_ERASE_TIMEOUT)
		{
			erased_OK = FALSE;
			break;
		}
	}

	/* Check the EOP flag in the FLASH_SR register */
	if((FLASH->SR & FLASH_SR_EOP) != 0)
	{
		/* Clear EOP flag */
		FLASH->SR = FLASH_SR_EOP;
	}
	else
	{
		/* Operation failed */
		erased_OK = FALSE;
	}

	/* Reset the PER Bit to disable the page erase */
	FLASH->CR &= ~FLASH_CR_PER;

	return erased_OK;
}

uint8_t Flash_unlock(void)
{
	uint8_t unlocked_OK = TRUE;
	uint32_t timeout = 0;

	CLEAR_TICK;

	/* Wait till no operation is on going */
	while((FLASH->SR & FLASH_SR_BSY) != 0)
	{
		if(CHECK_TICK)
		{
			timeout++;
		}

		if(timeout >= FLASH_UNLOCK_TIMEOUT)
		{
			unlocked_OK = FALSE;
			break;
		}
	}

	if(unlocked_OK == TRUE)
	{
		/* Is the flash memory is locked? */
		if((FLASH->CR & FLASH_CR_LOCK) != 0)
		{
			/* Yes, perform unlock */
			FLASH->KEYR = FLASH_UNLOCK_KEY1;
			FLASH->KEYR = FLASH_UNLOCK_KEY2;
		}
	}

	return unlocked_OK;
}

void Flash_lock(void)
{
	/* Lock flash */
	FLASH->CR |= FLASH_CR_LOCK;
}
