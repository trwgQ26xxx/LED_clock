/*
 * common_definitons.h
 *
 *  Created on: Feb 8, 2025
 *      Author: trwgQ26xxx
 */

#ifndef COMMON_DEFINITONS_H_
#define COMMON_DEFINITONS_H_

#include <stdint.h>

/* Common definitions */
#define TRUE	1
#define FALSE	0

#define DEC2BCD(in) (uint8_t)(((in / 10) << 4) | (in % 10))
#define BCD2BIN(in) (uint8_t)((((in & 0xF0) >> 4) * 10) + (in & 0x0F))

#endif /* COMMON_DEFINITONS_H_ */
