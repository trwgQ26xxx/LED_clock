/*
 * i2c_drv.h
 *
 *  Created on: Feb 20, 2025
 *      Author: trwgQ26xxx
 */

#ifndef I2C_DRV_H_
#define I2C_DRV_H_

#include <stdint.h>

uint8_t I2C_Read_Register(uint8_t device_addr, uint8_t reg_addr, uint8_t *reg_data);
uint8_t I2C_Write_Register(uint8_t device_addr, uint8_t reg_addr, uint8_t reg_data);

uint8_t I2C_Read_Data(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, uint8_t data_len);
uint8_t I2C_Write_Data(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, uint8_t data_len);

uint8_t I2C_Check_Addr(uint8_t device_addr);

#endif /* I2C_DRV_H_ */
