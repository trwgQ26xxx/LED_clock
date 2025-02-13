/*
 * flash_drv.h
 *
 *  Created on: Feb 9, 2025
 *      Author: trwgQ26xxx
 */

#ifndef FLASH_DRV_H_
#define FLASH_DRV_H_

#include "common_defs.h"

struct settings_struct
{
	uint8_t intensity;

	uint8_t unused;

	uint16_t crc;
};

void Init_flash(void);

void Read_settings(volatile struct settings_struct *s);

void Write_settings(volatile struct settings_struct *s);

#endif /* FLASH_DRV_H_ */
