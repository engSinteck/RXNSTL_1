/*
 * senoide.h
 *
 *  Created on: 1 de abr. de 2026
 *      Author: rinaldo.santos
 */

#ifndef SRC_SINE_GENERATOR_H_
#define SRC_SINE_GENERATOR_H_

#include "main.h"

// Prototipos
void generate_sine_table(void);
void fill_i2s_buffer(void);
void Send_I2S_buffer(void);

#endif /* SRC_SINE_GENERATOR_H_ */
