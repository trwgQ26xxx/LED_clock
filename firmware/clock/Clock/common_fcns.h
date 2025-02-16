/*
 * common_fcns.h
 *
 *  Created on: Feb 13, 2025
 *      Author: trwgQ26xxx
 */

#ifndef COMMON_FCNS_H_
#define COMMON_FCNS_H_

#include <stdint.h>

#define CLEAR_TICK		{volatile uint32_t tmp = SysTick->CTRL; (void)tmp;}				/* Clear the COUNTFLAG */
#define CHECK_TICK		((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) != 0U)

#define DEC2BCD(in)		(uint8_t)(((in / 10) << 4) | (in % 10))
#define BCD2BIN(in)		(uint8_t)((((in & 0xF0) >> 4) * 10) + (in & 0x0F))

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

#endif /* COMMON_FCNS_H_ */
