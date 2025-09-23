/*
 * pwm.c
 *
 *  Created on: 17 de jan de 2019
 *      Author: Rinaldo Dos Santos
 *      SInteck Next
 */
#include "pwm.h"
#include "tim.h"

void set_pwm(uint8_t channel, uint16_t value)
{
	switch(channel) {
		case PWM_FAN:
			if(value >=0 && value <= 4095) {
				__HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_2, value);	// PWM_FAN
			}
			break;
		case PWM_RF:
			if(value >=0 && value <= 4095) {
				__HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_1, value);	// PWM_RF
			}
			break;
		default:
			break;
	}
}
