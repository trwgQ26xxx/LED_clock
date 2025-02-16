/*
 * crc.h
 *
 *  Created on: Feb 16, 2025
 *      Author: trwgQ26xxx
 */

#ifndef CRC_H_
#define CRC_H_

#include <stdint.h>

#include "../Core/Inc/crc.h"

inline static uint32_t Calculate_CRC32(volatile uint8_t *data, uint32_t len)
{
	/* Set initial data */
	LL_CRC_SetInitialData(CRC, 0xFFFFFFFF);

	/* Reset CRC unit */
	LL_CRC_ResetCRCCalculationUnit(CRC);

	/* Set no reverse for input and output */
	LL_CRC_SetInputDataReverseMode(CRC, LL_CRC_INDATA_REVERSE_NONE);
	LL_CRC_SetOutputDataReverseMode(CRC, LL_CRC_OUTDATA_REVERSE_NONE);

	/* Feed data */
	for(uint32_t i = 0; i < len; i++)
	{
		LL_CRC_FeedData8(CRC, data[i]);
	}

	/* Get result */
	return LL_CRC_ReadData32(CRC);
}

#endif /* CRC_H_ */
