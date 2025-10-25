/*
 * onewire_bridge_drv.h
 *
 *  Created on: Oct 24, 2025
 *      Author: trwgQ26xxx
 */

#ifndef ONEWIRE_BRIDGE_DRV_H_
#define ONEWIRE_BRIDGE_DRV_H_

#include <stdint.h>


uint8_t Init_OneWire_bridge(void);

uint8_t OneWire_reset(void);
uint8_t OneWire_write_byte(uint8_t data);
uint8_t OneWire_read_byte(uint8_t *data);

#endif /* ONEWIRE_BRIDGE_DRV_H_ */
