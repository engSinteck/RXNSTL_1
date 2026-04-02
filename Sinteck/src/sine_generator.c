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

#define SINE_TABLE_SIZE 192

// Tabela senoidal 1kHz - 192kHz sampling - 24 bits
// Valores em 32 bits alinhados à esquerda (formato I2S padrão STM32)
const uint32_t sine_table_24bit[SINE_TABLE_SIZE] = {
    0x00000000, 0x04308000, 0x08610000, 0x0C8D0000, 0x10B80000, 0x14DE0000,
    0x18FA0000, 0x1D160000, 0x21300000, 0x25440000, 0x29500000, 0x2D540000,
    0x31500000, 0x35420000, 0x392C0000, 0x3D0C0000, 0x40E00000, 0x44A80000,
    0x48680000, 0x4C1C0000, 0x4FC40000, 0x53600000, 0x56EC0000, 0x5A680000,
    0x5DD80000, 0x61380000, 0x64880000, 0x67C80000, 0x6AF80000, 0x6E180000,
    0x71240000, 0x741C0000, 0x77000000, 0x79D00000, 0x7C8C0000, 0x7F300000,
    0x81C00000, 0x84380000, 0x86980000, 0x88E00000, 0x8B100000, 0x8D280000,
    0x8F240000, 0x91080000, 0x92D00000, 0x94800000, 0x96140000, 0x978C0000,
    0x98E80000, 0x9A280000, 0x9B4C0000, 0x9C540000, 0x9D400000, 0x9E100000,
    0x9EC40000, 0x9F5C0000, 0x9FD80000, 0xA0380000, 0xA07C0000, 0xA0A40000,
    0xA0B00000, 0xA0A40000, 0xA07C0000, 0xA0380000, 0x9FD80000, 0x9F5C0000,
    0x9EC40000, 0x9E100000, 0x9D400000, 0x9C540000, 0x9B4C0000, 0x9A280000,
    0x98E80000, 0x978C0000, 0x96140000, 0x94800000, 0x92D00000, 0x91080000,
    0x8F240000, 0x8D280000, 0x8B100000, 0x88E00000, 0x86980000, 0x84380000,
    0x81C00000, 0x7F300000, 0x7C8C0000, 0x79D00000, 0x77000000, 0x741C0000,
    0x71240000, 0x6E180000, 0x6AF80000, 0x67C80000, 0x64880000, 0x61380000,
    0x5DD80000, 0x5A680000, 0x56EC0000, 0x53600000, 0x4FC40000, 0x4C1C0000,
    0x48680000, 0x44A80000, 0x40E00000, 0x3D0C0000, 0x392C0000, 0x35420000,
    0x31500000, 0x2D540000, 0x29500000, 0x25440000, 0x21300000, 0x1D160000,
    0x18FA0000, 0x14DE0000, 0x10B80000, 0x0C8D0000, 0x08610000, 0x04308000,
    0x00000000, 0xFBCF8000, 0xF79F0000, 0xF3730000, 0xEF480000, 0xEB220000,
    0xE7060000, 0xE2EA0000, 0xDED00000, 0xDABC0000, 0xD6B00000, 0xD2AC0000,
    0xCEB00000, 0xCABE0000, 0xC6D40000, 0xC2F40000, 0xBF200000, 0xBB580000,
    0xB7980000, 0xB3E40000, 0xB03C0000, 0xACA00000, 0xA9140000, 0xA5980000,
    0xA2280000, 0x9EC80000, 0x9B780000, 0x98380000, 0x95080000, 0x91E80000,
    0x8EDC0000, 0x8BE40000, 0x89000000, 0x86300000, 0x83740000, 0x80D00000,
    0x7E400000, 0x7BC80000, 0x79680000, 0x77200000, 0x74F00000, 0x72D80000,
    0x70DC0000, 0x6EF80000, 0x6D300000, 0x6B800000, 0x69EC0000, 0x68740000,
    0x67180000, 0x65D80000, 0x64B40000, 0x63AC0000, 0x62C00000, 0x61F00000,
    0x613C0000, 0x60A40000, 0x60280000, 0x5FC80000, 0x5F840000, 0x5F5C0000,
    0x5F500000, 0x5F5C0000, 0x5F840000, 0x5FC80000, 0x60280000, 0x60A40000,
    0x613C0000, 0x61F00000, 0x62C00000, 0x63AC0000, 0x64B40000, 0x65D80000
};


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
    HAL_I2S_Transmit(&hi2s2, (uint16_t*)sine_table_24bit, SINE_TABLE_SIZE * 2, 200);
}
