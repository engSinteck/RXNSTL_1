/*
 * TPA6130A2.h
 *
 *  Created on: Feb 17, 2023
 *      Author: rinaldo.santos
 */

#ifndef TPA6130A2_H_
#define TPA6130A2_H_

#include "main.h"
#include "stdbool.h"

typedef struct {
	bool 	hp_en_l;
	bool 	hp_en_r;
	uint8_t mode;
	bool	mute_l;
	bool	mute_r;
	uint8_t	volume;
	bool	hiz_l;
	bool	hiz_r;
	uint8_t	version;
} tpa6130_var;

// HAL expects address to be shifted one bit to the left
#define TPA_DEVICE_ADDR						(uint16_t)(0x60 << 1)

#define TPA6130_CONTROL              		0x01
#define TPA6130_VOLUME_AND_MUTE      		0x02
#define TPA6130_OUTPUT_IMPEDANCE    		0x03
#define TPA6130_I2C_ADDRESS_VERSION  		0x04

/* Default register values after a reset */
#define TPA6130_CONTROL_DEFAULT             0x00
#define TPA6130_VOLUME_AND_MUTE_DEFAULT     0x0F
#define TPA6130_OUTPUT_IMPEDANCE_DEFAULT    0x00
#define TPA6130_I2C_ADDRESS_VERSION_DEFAULT 0x02

// Control register
#define HP_EN_L          					0x80
#define HP_EN_R          					0x40
#define STEREO_HP        					0x00
#define DUAL_MONO_HP     					0x10
#define BRIDGE_TIED_LOAD 					0x20
#define THERMAL          					0x02
#define SW_SHUTDOWN      					0x01

// Volume and mute register
#define MUTE_L           					0x80
#define MUTE_R           					0x40

// Output impedance register
#define HIZ_L            					0x80
#define HIZ_R            					0x40

// I2C address version register
#define VERSION          					0x02

#define TPA6130_MAX_VOLUME  				0x3F

HAL_StatusTypeDef TPA6130_Write(uint8_t *pdata, uint8_t size);
HAL_StatusTypeDef TPA6130_Read(uint8_t *pdata, uint8_t size);
void tpa6130_init(void);
void tpa6130_set_volume(uint8_t volume);

#endif /* TPA6130A2_H_ */
