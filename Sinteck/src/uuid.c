/*
 * uuid.c
 *
 *  Created on: 25 de mar de 2020
 *      Author: Rinaldo Dos Santos
 *      Sinteck Next
 */

#include "main.h"
#include "../Sinteck/src/uuid.h"
#include "../Sinteck/src/eeprom.h"

extern Cfg_var cfg;

void uuid_generator(void)
{
}

void Get_UUID(void)
{
	cfg.uuid[0] = (uint8_t)((HAL_GetUIDw0() & 0xFF000000) >> 24);
	cfg.uuid[1] = (uint8_t)((HAL_GetUIDw0() & 0x00FF0000) >> 16);
	cfg.uuid[2] = (uint8_t)((HAL_GetUIDw0() & 0x0000FF00) >> 8);
	cfg.uuid[3] = (uint8_t)((HAL_GetUIDw0() & 0x000000FF));

	cfg.uuid[4] = (uint8_t)((HAL_GetDEVID() & 0xFF000000) >> 24);
	cfg.uuid[5] = (uint8_t)((HAL_GetDEVID() & 0x00FF0000) >> 16);
	cfg.uuid[6] = (uint8_t)((HAL_GetDEVID() & 0x0000FF00) >> 8);
	cfg.uuid[7] = (uint8_t)((HAL_GetDEVID() & 0x000000FF));

	cfg.uuid[8] = (uint8_t)((HAL_GetUIDw1() & 0xFF000000) >> 24);
	cfg.uuid[9] = (uint8_t)((HAL_GetUIDw1() & 0x00FF0000) >> 16);
	cfg.uuid[10] = (uint8_t)((HAL_GetUIDw1() & 0x0000FF00) >> 8);
	cfg.uuid[11] = (uint8_t)((HAL_GetUIDw1() & 0x000000FF));

	cfg.uuid[12] = (uint8_t)((HAL_GetUIDw2() & 0xFF000000) >> 24);
	cfg.uuid[13] = (uint8_t)((HAL_GetUIDw2() & 0x00FF0000) >> 16);
	cfg.uuid[14] = (uint8_t)((HAL_GetUIDw2() & 0x0000FF00) >> 8);
	cfg.uuid[15] = (uint8_t)((HAL_GetUIDw2() & 0x000000FF));
}
