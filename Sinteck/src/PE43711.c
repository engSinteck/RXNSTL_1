/*
 * PE43711.c
 *
 *  Created on: Sep 23, 2025
 *      Author: rinaldo.santos
 */
#include "main.h"
#include <stdio.h>
#include <string.h>

#include "../Sinteck/src/delay.h"
#include "../Sinteck/src/PE43711.h"

extern void giraclk(void);

float attenuation = 0.0f;
uint8_t atn_out = 0;

uint8_t bit_test_atn(uint8_t val, uint8_t nbit)
{
    return ((val & (1<<nbit)) != 0);
}

void PE43711(float atn)
{
	attenuation = 4 * atn;
	atn_out = (uint8_t)attenuation;

	// Debug
	printf("attn: V1: %0.1f  V2: %d", attenuation, atn_out);

	// Initial TX
	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(EN_ATTN_RF_GPIO_Port, EN_ATTN_RF_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(CLK_IO_GPIO_Port, CLK_IO_Pin, GPIO_PIN_RESET);
	MicroDelay(100);

	// Data 7
	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	if (bit_test_atn(atn_out, 7))
	  HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_SET);
	giraclk();
	// Data 6
	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	if (bit_test_atn(atn_out, 6))
	  HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_SET);
	giraclk();
	// Data 5
	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	if (bit_test_atn(atn_out, 5))
	  HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_SET);
	giraclk();
	// Data 4
	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	if (bit_test_atn(atn_out, 4))
	  HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_SET);
	giraclk();
	// Data 3
	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	if (bit_test_atn(atn_out, 3))
	  HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_SET);
	giraclk();
	// Data 2
	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	if (bit_test_atn(atn_out, 2))
	  HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_SET);
	giraclk();
	// Data 1
	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	if (bit_test_atn(atn_out, 1))
	  HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_SET);
	giraclk();
	// Data 0
	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
	if (bit_test_atn(atn_out, 0))
	  HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_SET);
	giraclk();

	delayUS_DWT(20);

	HAL_GPIO_WritePin(EN_ATTN_RF_GPIO_Port, EN_ATTN_RF_Pin, GPIO_PIN_RESET); 	// Termina TX
	HAL_GPIO_WritePin(CLK_IO_GPIO_Port, CLK_IO_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(DATA_IO_GPIO_Port, DATA_IO_Pin, GPIO_PIN_RESET);
}
