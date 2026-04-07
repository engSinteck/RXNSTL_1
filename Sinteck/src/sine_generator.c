/*
 * senoide.c
 *
 *  Created on: 1 de abr. de 2026
 *      Author: rinaldo.santos
 */
#include "../Sinteck/src/sine_generator.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "i2s.h"

#define SAMPLE_RATE   48000
#define SINE_FREQ     1000
#define TABLE_SIZE    (SAMPLE_RATE / SINE_FREQ) // 48
#define DMA_BUFFER_SIZE (TABLE_SIZE * 2)

int16_t sine_table[TABLE_SIZE] __attribute__(( aligned(32)));
__attribute__((section(".RAM_D2"))) int32_t i2s_buffer[DMA_BUFFER_SIZE];

// Gera Senoide em 1KHz @ 48KHz - 16Bit
void generate_sine_table(void)
{
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        float angle = (2.0f * M_PI * i) / TABLE_SIZE;

        sine_table[i] = (int16_t)(32767.0f * sinf(angle));
    }
}

// Preenche Buffer I2S Buffer
void fill_i2s_buffer(void)
{
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        i2s_buffer[2*i]     = ((int32_t)sine_table[i]) << 16; // Left
        i2s_buffer[2*i +1]  = ((int32_t)sine_table[i]) << 16; // Right
    }
}

// Envia I2S - 48Khz 16Bits
void Send_I2S_buffer(void)
{
	SCB_CleanDCache_by_Addr((uint32_t*)i2s_buffer, sizeof(i2s_buffer));
	HAL_I2S_Transmit_DMA(&hi2s2, (uint16_t*)i2s_buffer, DMA_BUFFER_SIZE);
}
