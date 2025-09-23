/*
 * pwm.h
 *
 *  Created on: 17 de jan de 2019
 *      Author: rinal
 */

#ifndef INC_PWM_H_
#define INC_PWM_H_

#ifdef STM32H743xx
   #include "stm32h7xx_hal.h"
#else
	#include "stm32f7xx_hal.h"
#endif

void set_pwm(uint8_t channel, uint16_t value);

#endif /* INC_PWM_H_ */
