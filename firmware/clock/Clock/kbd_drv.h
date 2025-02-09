/*
 * kbd_drv.h
 *
 *  Created on: Feb 8, 2025
 *      Author: trwgQ26xxx
 */

#ifndef KBD_DRV_H_
#define KBD_DRV_H_

#include "../Core/Inc/tim.h"
#include "../Core/Inc/gpio.h"

/* Keyboard definitions */
#define ENTER_KEY_IS_PRESSED		(!(LL_GPIO_IsInputPinSet(ENTER_KEY_GPIO_Port, ENTER_KEY_Pin)))
#define PLUS_KEY_IS_PRESSED			(!(LL_GPIO_IsInputPinSet(PLUS_KEY_GPIO_Port, PLUS_KEY_Pin)))
#define MINUS_KEY_IS_PRESSED		(!(LL_GPIO_IsInputPinSet(MINUS_KEY_GPIO_Port, MINUS_KEY_Pin)))
#define ESC_KEY_IS_PRESSED			(!(LL_GPIO_IsInputPinSet(ESC_KEY_GPIO_Port, ESC_KEY_Pin)))

#define ENTER_KEY_IS_RELEASED		(LL_GPIO_IsInputPinSet(ENTER_KEY_GPIO_Port, ENTER_KEY_Pin))
#define PLUS_KEY_IS_RELEASED		(LL_GPIO_IsInputPinSet(PLUS_KEY_GPIO_Port, PLUS_KEY_Pin))
#define MINUS_KEY_IS_RELEASED		(LL_GPIO_IsInputPinSet(MINUS_KEY_GPIO_Port, MINUS_KEY_Pin))
#define ESC_KEY_IS_RELEASED			(LL_GPIO_IsInputPinSet(ESC_KEY_GPIO_Port, ESC_KEY_Pin))

#define ALL_KEYS_ARE_RELEASED		(ENTER_KEY_IS_RELEASED && PLUS_KEY_IS_RELEASED && MINUS_KEY_IS_RELEASED && ESC_KEY_IS_RELEASED)

#define ENABLE_KBD_TIMER			LL_TIM_EnableCounter(TIM17)
#define CLEAR_KBD_TIMER				LL_TIM_SetCounter(TIM17, 0)
#define GET_KBD_TIMER				LL_TIM_GetCounter(TIM17)

#endif /* KBD_DRV_H_ */
