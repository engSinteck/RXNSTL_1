/*
 * falha.h
 *
 *  Created on: Sep 10, 2025
 *      Author: rinaldo.santos
 */
#ifndef INC_FALHA_H_
#define INC_FALHA_H_

#ifdef STM32H743xx
   #include "stm32h7xx_hal.h"
#else
	#include "stm32f7xx_hal.h"
#endif

#include "../Sinteck/src/defines.h"

#define QTD_MAX_ALARME	60

extern uint64_t falha;
extern uint8_t counter_screen;
extern uint32_t ContAlarme;

typedef struct {
	uint8_t enable;
	uint8_t alarme;
	uint8_t flag;
	uint8_t subtipo;
	uint8_t alm1;
	uint8_t alm2;
	uint8_t second;
	uint8_t minute;
	uint8_t hour;
	uint8_t day;
	uint8_t month;
	uint16_t year;
	char text[20];
} Cod_Alarme;

void status_falha(void);
void ClearAlarme(void);
void Alarme_Ins(uint8_t tipo, uint8_t subtipo, uint8_t flag);
void Alarme_Ins_Eltek(uint8_t tipo, uint8_t subtipo, uint8_t alm1, uint8_t alm2, uint8_t flag);
char* decode_falha(uint64_t fail);
char* decode_falha_2(uint64_t fail);
char* decode_falha_remote(uint8_t fail);
char* decode_falha_Eltek(uint8_t alm1, uint8_t alm2);
char* decode_falha_snmp(uint64_t fail);
void SetAlarmeTeste(void);
void setbitfalha(uint64_t *val, int bit, int value);

#endif /* INC_FALHA_H_ */

