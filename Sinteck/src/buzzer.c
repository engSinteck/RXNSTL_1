/*
 * buzzer.c
 *
 *  Created on: 1 de set de 2020
 *      Author: Rinaldo Dos Santos
 *      Sinteck Next
 */

#include "main.h"
#include "../Sinteck/src/buzzer.h"

buzzer_var Buzzer;

void Buzzer_init(void)
{
	Buzzer.cnt = 0;
	Buzzer.cnt2 = 0;
	Buzzer.delay = 0;
	Buzzer.tmr = 0;
	Buzzer.tmr_h = 0;
	Buzzer.tmr_l = 0;
	Buzzer.tmr2 = 0;
	Buzzer.tmr2_h = 0;
	Buzzer.tmr2_l = 0;
	Buzzer.state = 0;
}

void Buzzer_Test(uint32_t tempo, uint32_t contador, uint32_t delay)
{
	Buzzer.state = 1;
	Buzzer.tmr = tempo / 10;
	Buzzer.tmr_h = tempo / 10;
	Buzzer.tmr_l = tempo / 10;
	Buzzer.cnt = contador;
	Buzzer.delay = delay;
}

void Buzzer_Test2(uint32_t tempo, uint32_t contador, uint32_t delay, uint32_t tempo2, uint32_t contador2)
{
	Buzzer.state = 1;
	Buzzer.tmr = tempo / 10;
	Buzzer.tmr_h = tempo / 10;
	Buzzer.tmr_l = tempo / 10;
	Buzzer.cnt = contador;
	Buzzer.delay = delay;
	//
	Buzzer.tmr2 = tempo2 / 10;
	Buzzer.tmr2_h = tempo2 / 10;
	Buzzer.tmr2_l = tempo2 / 10;
	Buzzer.cnt2 = contador2;
}

void buzzer(uint8_t tipo)
{
#ifdef GIGA
	return;
#endif
	switch(tipo) {
		case 0:
			break;
		case 1:
			Buzzer_Test(500, 100, 0);
			break;
		case 2:
			Buzzer_Test(250, 100, 0);
			break;
		case 3:
			Buzzer_Test(125, 250, 0);
			break;
		case 4:
			Buzzer_Test(750, 250, 0);
			break;
		case 5:
			Buzzer_Test(1000, 250, 0);
			break;
		case 6:
			Buzzer_Test(125, 125, 0);
			break;
	}
}

void buzzer_Debug(uint8_t tipo)
{
#ifdef GIGA
	return;
#endif
	switch(tipo) {
		case 0:
			break;
		case 1:
			Buzzer_Test2(500, 100, 2, 1000, 250);
			break;
		case 2:
			Buzzer_Test2(250, 100, 2, 1000, 250);
			break;
		case 3:
			Buzzer_Test2(125, 250, 2, 1000, 250);
			break;
		case 4:
			Buzzer_Test2(750, 250, 2, 1000, 250);
			break;
		case 5:
			Buzzer_Test2(1000, 250, 2, 1000, 250);
			break;
	}
}

void Buzzer_Run2(void)
{
	// Buzzer ON
	if(Buzzer.state == 1 && Buzzer.cnt != 0 && Buzzer.tmr_h != 0) {
		HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
		Buzzer.state = 2;
	}
	else if(Buzzer.state == 2) {
		Buzzer.tmr_h--;
		if(Buzzer.tmr_h == 0) {
			Buzzer.state = 3;
		}
	}
	// Buzzer OFF
	else if(Buzzer.state == 3) {
		HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
		Buzzer.state = 4;
	}
	else if(Buzzer.state == 4) {
		Buzzer.tmr_l--;
		if(Buzzer.tmr_l == 0) {
			if(Buzzer.cnt >= 1) Buzzer.cnt--;
			if(Buzzer.cnt == 0)
				if(Buzzer.cnt2 == 0) {
					Buzzer.state = 0;
				}
				else {
					Buzzer.state = 5;
				}
			else {
				Buzzer.state = 1;
				Buzzer.tmr_h = Buzzer.tmr;
				Buzzer.tmr_l = Buzzer.tmr;
			}
		}
	}
	else if(Buzzer.state == 5) {
		Buzzer.delay--;
		if(Buzzer.delay == 0) Buzzer.state = 6;
	}
	else if(Buzzer.state == 6) {
		// Buzzer ON
		if(Buzzer.state == 6 && Buzzer.cnt2 != 0 && Buzzer.tmr2_h != 0) {
			HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
			Buzzer.state = 7;
		}
	}
	else if(Buzzer.state == 7) {
		Buzzer.tmr2_h--;
		if(Buzzer.tmr2_h == 0) {
			Buzzer.state = 8;
		}
	}
	// Buzzer OFF
	else if(Buzzer.state == 8) {
		HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
		Buzzer.state = 9;
	}
	else if(Buzzer.state == 9) {
		Buzzer.tmr2_l--;
		if(Buzzer.tmr2_l == 0) {
			if(Buzzer.cnt2 >= 1) Buzzer.cnt2--;
			if(Buzzer.cnt2 == 0)
				Buzzer.state = 0;
			else {
				Buzzer.state = 5;
				Buzzer.tmr2_h = Buzzer.tmr2;
				Buzzer.tmr2_l = Buzzer.tmr2;
			}
		}
	}
}

void Buzzer_Run(void)
{
	// Buzzer ON
	if(Buzzer.state == 1 && Buzzer.cnt != 0 && Buzzer.tmr_h != 0) {
		HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
		Buzzer.state = 2;
	}
	else if(Buzzer.state == 2) {
		Buzzer.tmr_h--;
		if(Buzzer.tmr_h == 0) {
			Buzzer.state = 3;
		}
	}
	// Buzzer OFF
	else if(Buzzer.state == 3) {
		HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
		Buzzer.state = 4;
	}
	else if(Buzzer.state == 4) {
		Buzzer.tmr_l--;
		if(Buzzer.tmr_l == 0) {
			if(Buzzer.cnt >= 1) Buzzer.cnt--;
			if(Buzzer.cnt == 0)
				Buzzer.state = 0;
			else {
				Buzzer.state = 1;
				Buzzer.tmr_h = Buzzer.tmr;
				Buzzer.tmr_l = Buzzer.tmr;
			}
		}
	}
}
