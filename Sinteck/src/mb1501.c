/*
 * mb1501.c
 *
 *  Created on: 27 de jun de 2019
 *      Author: Rinaldo Dos Santos
 *      Sinteck Next
 */

#include "main.h"
#include "../Sinteck/src/mb1501.h"
#include "../Sinteck/src/delay.h"

uint8_t debounce_pll = 0, flag_pll_power = 0;
uint8_t Status_pll_lock_pin = 0, pll_lock_status = 0;

void giraclk(void)
{
	MicroDelay(5);
	HAL_GPIO_WritePin(CLK_IO_GPIO_Port, CLK_IO_Pin, GPIO_PIN_SET);
	MicroDelay(5);
	HAL_GPIO_WritePin(CLK_IO_GPIO_Port, CLK_IO_Pin, GPIO_PIN_RESET);
	MicroDelay(5);
}

// XTAL = 20.000MHZ
// PLL OUT =
void mb1501ref(void)
{
	// Initial TX
	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(EN_MB1501_GPIO_Port, EN_MB1501_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(CLK_IO_GPIO_Port, CLK_IO_Pin, GPIO_PIN_RESET);
	MicroDelay(100);

	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_SET);		// CTRL = 1
	giraclk();
	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);		// MSB  = 0
	giraclk();
	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	giraclk();
	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_SET);
	giraclk();
	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_SET);
	giraclk();
	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_SET);
	giraclk();
	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_SET);
	giraclk();
	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_SET);
	giraclk();

	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	giraclk();
	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_SET);
	giraclk();
	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	giraclk();
	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	giraclk();
	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	giraclk();
	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	giraclk();
	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	giraclk();
	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_SET);
	giraclk();

	MicroDelay(20);
	HAL_GPIO_WritePin(EN_MB1501_GPIO_Port, EN_MB1501_Pin, GPIO_PIN_SET); 	// Termina TX
	MicroDelay(5);
	HAL_GPIO_WritePin(EN_MB1501_GPIO_Port, EN_MB1501_Pin, GPIO_PIN_RESET); 	// Termina TX
	HAL_GPIO_WritePin(CLK_IO_GPIO_Port, CLK_IO_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
}

uint8_t bit_test(uint32_t val, uint32_t nbit)
{
    return ((val & (1<<nbit)) != 0);
}

void mb1501(long int tempfreq)
{
	long int mbfreq, nfreq;
	char swa;

	mb1501ref();

	// Initial TX
	HAL_Delay(20);
	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(EN_MB1501_GPIO_Port, EN_MB1501_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(CLK_IO_GPIO_Port, CLK_IO_Pin, GPIO_PIN_RESET);
	MicroDelay(100);

	// transforma calculo
	mbfreq = tempfreq * 2;
	nfreq = mbfreq / 64;            // extrair o numero N
	swa = (mbfreq - (nfreq * 64));  // extrair o numero Swallow

	// transmitir N
	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);

	if(bit_test(nfreq, 10))
	  HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_SET);
	giraclk();

	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	if (bit_test(nfreq, 9))
	  HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_SET);
	giraclk();

	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	if (bit_test(nfreq, 8))
	  HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_SET);
	giraclk();

	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	if (bit_test(nfreq, 7))
	  HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_SET);
	giraclk();

	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	if (bit_test(nfreq, 6))
	  HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_SET);
	giraclk();

	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	if (bit_test(nfreq, 5))
	  HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_SET);
	giraclk();

	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	if (bit_test(nfreq, 4))
	  HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_SET);
	giraclk();

	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	if (bit_test(nfreq, 3))
	  HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_SET);
	giraclk();

	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	if (bit_test(nfreq, 2))
	  HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_SET);
	giraclk();

	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	if (bit_test(nfreq, 1))
	  HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_SET);
	giraclk();

	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	if (bit_test(nfreq, 0))
	  HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_SET);
	giraclk();

	// transmitir A
	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	if (bit_test(swa, 6))
	  HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_SET);
	giraclk();

	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	if ( bit_test(swa, 5) )
	  HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_SET);
	giraclk();

	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	if ( bit_test(swa, 4) )
	  HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_SET);
	giraclk();

	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	if ( bit_test(swa, 3) )
	  HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_SET);
	giraclk();

	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	if ( bit_test(swa, 2) )
	  HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_SET);
	giraclk();

	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	if ( bit_test(swa, 1) )
	  HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_SET);
	giraclk();

	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	if ( bit_test(swa, 0) )
	  HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_SET);
	giraclk();

	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	giraclk();
	delayUS_DWT(20);

	// TX
	HAL_GPIO_WritePin(EN_MB1501_GPIO_Port, EN_MB1501_Pin, GPIO_PIN_SET);
	MicroDelay(20);
	HAL_GPIO_WritePin(EN_MB1501_GPIO_Port, EN_MB1501_Pin, GPIO_PIN_RESET);

	HAL_GPIO_WritePin(CLK_IO_GPIO_Port, CLK_IO_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);

	flag_pll_power = 0;
}

void status_pll(void)
{
	// PLL LOCK
	if( Status_pll_lock_pin == 0 ) {
		debounce_pll = 0;
		HAL_GPIO_WritePin(LED_LOCK_GPIO_Port, LED_LOCK_Pin, LED_ON);

#ifdef DEMO
		HAL_GPIO_WritePin(LED_RFON_GPIO_Port, LED_RFON_Pin, LED_ON);			// LED RF ON
#endif

		if(pll_lock_status) {
			pll_lock_status = 0;
			//falha &= ~((uint64_t)1ULL << FAIL_PLLLOCK);						// Desliga Falha PLL
			flag_pll_power = 1;
		}
	}
	else {
		HAL_GPIO_WritePin(LED_LOCK_GPIO_Port, LED_LOCK_Pin, LED_OFF);			// LED OFF
		pll_lock_status = 255;
		debounce_pll++;
	}
}

