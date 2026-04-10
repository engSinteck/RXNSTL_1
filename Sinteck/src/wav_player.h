/*
 * wav_player.h
 *
 *  Created on: 10 de abr. de 2026
 *      Author: rinaldo.santos
 */

#ifndef SRC_WAV_PLAYER_H_
#define SRC_WAV_PLAYER_H_

#include "main.h"
#include "fatfs.h"
#include "cmsis_os.h"

// Funções públicas
void Audio_Player_Stop(void);
void Audio_Player_Init(void);
void Audio_Player_Start(const char* filename);
void fill_sine_buffer_task(int32_t* buffer, uint32_t num_words);

#endif
