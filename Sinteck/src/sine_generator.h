/*
 * senoide.h
 *
 *  Created on: 1 de abr. de 2026
 *      Author: rinaldo.santos
 */

#ifndef SRC_SINE_GENERATOR_H_
#define SRC_SINE_GENERATOR_H_

#include "main.h"

// sine_generator.h
typedef struct {
    uint32_t	sample_rate;      	// Taxa de amostragem (192000 Hz)
    float 		frequency;          // Frequência desejada (1020 Hz)
    float 		amplitude;          // Amplitude (0.0 a 1.0)
    uint32_t 	num_samples;       	// Número de amostras na tabela
    uint32_t 	*sine_table;       	// Tabela da senoide (32 bits para 24 bits)
    uint32_t 	table_size;			// Tamanho da tabela
    float 		phase_increment;	// Incremento de fase
} SineGenerator_t;

// sine_calculations.h
typedef struct {
    uint32_t sample_rate;      		// 192000 Hz
    float target_freq;         		// 1020 Hz
    uint32_t table_size;       		// Tamanho da tabela
    uint32_t cycles_per_table; 		// Ciclos completos na tabela
    float freq_resolution;     		// Resolução de frequência
    float actual_frequency;    		// Frequência real gerada
} SineParameters_t;

// Prototipos
void SineGenerator_Init(SineGenerator_t *gen, uint32_t sample_rate, uint32_t table_size);
void SineGenerator_GenerateTable(SineGenerator_t *gen);
void SineGenerator_SetAmplitude(SineGenerator_t *gen, float amplitude);
void SineGenerator_SetFrequency(SineGenerator_t *gen, float frequency);
void CalculateOptimalParameters(SineParameters_t *params);
void GenerateOptimalSineTable(SineGenerator_t *gen, SineParameters_t *params);
void atualiza_buffer(SineGenerator_t *gen);

#endif /* SRC_SINE_GENERATOR_H_ */
