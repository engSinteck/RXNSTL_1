/*
 * mb1501.h
 *
 *  Created on: 27 de jun de 2019
 *      Author: rinaldo
 */

#ifndef SRC_MB1501_H_
#define SRC_MB1501_H_

#ifdef STM32H743xx
   #include "stm32h7xx_hal.h"
#else
	#include "stm32f7xx_hal.h"
#endif

void giraclk(void);
void mb1501ref(void);
void mb1501(long int tempfreq);
void status_pll(void);
void status_Osc_10Mhz(void);
void Enable_OSC10MHZ_CFG(uint8_t value);

#endif /* SRC_MB1501_H_ */
