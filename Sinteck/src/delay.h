/*
 * delay.h
 *
 *  Created on: 26 de jun de 2019
 *      Author: rinaldo
 */

#ifndef INC_DELAY_H_
#define INC_DELAY_H_

#ifdef STM32H743xx
   #include "stm32h7xx_hal.h"
#else
	#include "stm32f7xx_hal.h"
#endif

#define AIRCR_VECTKEY_MASK    ((uint32_t)0x05FA0000)
#define DWT_DELAY_NEWBIE 0

void delayUS_DWT(uint32_t us);
void MicroDelay(uint32_t val);
void NVIC_GenerateSystemReset(void);
void teste_ri(void);
void DWT_Init(void);
void DWT_Delay(uint32_t us);

#endif /* INC_DELAY_H_ */
