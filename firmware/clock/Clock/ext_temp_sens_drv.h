/*
 * ext_temp_sens_drv.h
 *
 *  Created on: Oct 24, 2025
 *      Author: trwgQ26xxx
 */

#ifndef EXT_TEMP_SENS_DRV_H_
#define EXT_TEMP_SENS_DRV_H_

#include <stdint.h>


uint8_t Init_ext_temp_sens(void);

uint8_t Ext_temp_start_conversion(void);
uint8_t Ext_temp_read_temperature(int8_t *temperature);

#endif /* EXT_TEMP_SENS_DRV_H_ */
