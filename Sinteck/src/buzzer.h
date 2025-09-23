/*
 * buzzer.h
 *
 *  Created on: 1 de set de 2020
 *      Author: Rinaldo Dos Santos
 *      Sinteck Next
 */

#ifndef SRC_BUZZER_H_
#define SRC_BUZZER_H_

void Buzzer_init(void);
void Buzzer_Test(uint32_t tempo, uint32_t contador, uint32_t delay);
void Buzzer_Test2(uint32_t tempo, uint32_t contador, uint32_t delay, uint32_t tempo2, uint32_t contador2);
void Buzzer_Run(void);
void buzzer(uint8_t tipo);
void Buzzer_Run2(void);
void buzzer_Debug(uint8_t tipo);

#endif /* SRC_BUZZER_H_ */
