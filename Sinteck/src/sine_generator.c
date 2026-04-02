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

uint32_t buffer_sine[2048*4] = {0};

// Inicializa o gerador de senoide
void SineGenerator_Init(SineGenerator_t *gen, uint32_t sample_rate, uint32_t table_size)
{
    gen->sample_rate = sample_rate;
    gen->table_size = table_size;
    gen->sine_table = (uint32_t*)malloc(table_size * sizeof(uint32_t));
    gen->amplitude = 1.0f;
}

// Calcula a tabela de senoide para 24 bits (MSB alinhado)
void SineGenerator_GenerateTable(SineGenerator_t *gen)
{
    int32_t max_amplitude;
    int32_t sample_24bit;

    // Para 24 bits, amplitude máxima é 2^23 - 1 = 8388607
    // Alinhado ao MSB em 32 bits: desloca 8 bits à esquerda
    max_amplitude = (int32_t)(8388607.0f * gen->amplitude);

    // Calcula o incremento de fase para a frequência desejada
    gen->phase_increment = (2.0f * M_PI * gen->frequency) / gen->sample_rate;

    // Gera a tabela com amostras de 24 bits alinhadas em 32 bits
    for(uint32_t i = 0; i < gen->table_size; i++) {
        float phase = i * gen->phase_increment;
        float sample = sinf(phase);

        // Converte para 24 bits com sinal (formato Q23)
        sample_24bit = (int32_t)(sample * max_amplitude);

        // Alinha ao MSB em 32 bits (desloca 8 bits para esquerda)
        // I2S periférico espera dados alinhados ao MSB para 24 bits
        gen->sine_table[i] = (uint32_t)(sample_24bit << 8);
    }
}

// Atualiza amplitude (para controle em tempo real)
void SineGenerator_SetAmplitude(SineGenerator_t *gen, float amplitude)
{
    gen->amplitude = amplitude;

    // Regenera tabela com nova amplitude
    int32_t max_amplitude = (int32_t)(8388607.0f * gen->amplitude);

    for(uint32_t i = 0; i < gen->table_size; i++) {
        float phase = i * gen->phase_increment;
        float sample = sinf(phase);
        int32_t sample_24bit = (int32_t)(sample * max_amplitude);
        gen->sine_table[i] = (uint32_t)(sample_24bit << 8);
    }
}

// Atualiza frequência (para controle em tempo real)
void SineGenerator_SetFrequency(SineGenerator_t *gen, float frequency)
{
    gen->frequency = frequency;
    gen->phase_increment = (2.0f * M_PI * gen->frequency) / gen->sample_rate;

    // Regenera tabela com nova frequência
    int32_t max_amplitude = (int32_t)(8388607.0f * gen->amplitude);

    for(uint32_t i = 0; i < gen->table_size; i++) {
        float phase = i * gen->phase_increment;
        float sample = sinf(phase);
        int32_t sample_24bit = (int32_t)(sample * max_amplitude);
        gen->sine_table[i] = (uint32_t)(sample_24bit << 8);
    }
}

// Calcula parâmetros otimizados para a senoide
void CalculateOptimalParameters(SineParameters_t *params)
{
    params->sample_rate = 192000;
    params->target_freq = 1020.0f;

    // Encontra tamanho de tabela que dá um número inteiro de ciclos
    // Isso evita descontinuidades na transição entre buffers
    for(params->table_size = 256; params->table_size <= 4096; params->table_size += 256) {
        float cycles = (params->target_freq * params->table_size) / params->sample_rate;
        float cycles_int;
        float frac = modff(cycles, &cycles_int);

        if(frac < 0.001f || frac > 0.999f) {
            params->cycles_per_table = (uint32_t)roundf(cycles);
            params->actual_frequency = (params->cycles_per_table * params->sample_rate) /
                                        (float)params->table_size;
            params->freq_resolution = params->sample_rate / (float)params->table_size;
            break;
        }
    }

    // Se não encontrou, usa o maior tamanho
    if(params->table_size > 4096) {
        params->table_size = 4096;
        params->cycles_per_table = (uint32_t)((params->target_freq * params->table_size) /
                                               params->sample_rate);
        params->actual_frequency = (params->cycles_per_table * params->sample_rate) /
                                    (float)params->table_size;
        params->freq_resolution = params->sample_rate / (float)params->table_size;
    }
}

// Gera tabela com número inteiro de ciclos
void GenerateOptimalSineTable(SineGenerator_t *gen, SineParameters_t *params)
{
    gen->sample_rate = params->sample_rate;
    gen->frequency = params->actual_frequency;  // Usa frequência real otimizada
    gen->table_size = params->table_size;

    // Realoca tabela se necessário
    if(gen->sine_table) {
        free(gen->sine_table);
    }
    gen->sine_table = (uint32_t*)malloc(params->table_size * sizeof(uint32_t));

    // Gera tabela com fase contínua
    int32_t max_amplitude = (int32_t)(8388607.0f * gen->amplitude);
    float phase_increment = (2.0f * M_PI * gen->frequency) / gen->sample_rate;

    for(uint32_t i = 0; i < params->table_size; i++) {
        float phase = i * phase_increment;
        float sample = sinf(phase);
        int32_t sample_24bit = (int32_t)(sample * max_amplitude);
        gen->sine_table[i] = (uint32_t)(sample_24bit << 8);
    }
}

void atualiza_buffer(SineGenerator_t *gen)
{
    memcpy(buffer_sine, gen->sine_table, 2048);

    // TX via I2S
    HAL_I2S_Transmit(&hi2s2, (uint16_t*)buffer_sine, 2048, 200);
}
