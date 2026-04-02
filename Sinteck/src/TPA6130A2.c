/*
 * TPA6130A2.c
 *
 *  Created on: Feb 17, 2023
 *      Author: rinaldo.santos
 */

#include "main.h"
#include "i2c.h"
#include "TPA6130A2.h"

tpa6130_var headphone;

uint8_t cmd_tpa[4] = {0};

extern Cfg_var cfg;

HAL_StatusTypeDef TPA6130_Write(uint8_t *pdata, uint8_t size)
{
	HAL_StatusTypeDef Result = HAL_OK;

	Result = HAL_I2C_IsDeviceReady(&hi2c1, TPA_DEVICE_ADDR, 64, HAL_MAX_DELAY);
	Result = HAL_I2C_Master_Transmit(&hi2c1, TPA_DEVICE_ADDR, pdata, size, HAL_MAX_DELAY);

	HAL_Delay(10);
    return Result;
}

HAL_StatusTypeDef TPA6130_Read(uint8_t *pdata, uint8_t size)
{
	HAL_StatusTypeDef Result = HAL_OK;

	Result = HAL_I2C_IsDeviceReady(&hi2c1, TPA_DEVICE_ADDR, 3, HAL_MAX_DELAY);
	Result = HAL_I2C_Master_Receive(&hi2c1, TPA_DEVICE_ADDR, pdata, size, HAL_MAX_DELAY);

	return Result;
}

void tpa6130_init(void)
{
	headphone.hp_en_l = 1;
	headphone.hp_en_r = 1;
	headphone.hiz_l = 0;
	headphone.hiz_r = 0;
	headphone.mute_l = 0;
	headphone.mute_r = 0;
	headphone.mode = 0x00;		// 0x00 Stereo, 0x01 Dual Mono, 0x02 Bridge
	headphone.volume = 32;
	headphone.version = 0x02;

	// Reg 1
	cmd_tpa[0] = TPA6130_CONTROL;
	cmd_tpa[1] = 0;
	cmd_tpa[1] = HP_EN_L | HP_EN_R | STEREO_HP;
	TPA6130_Write(cmd_tpa, 2);
	// Reg 2
	cmd_tpa[0] = TPA6130_VOLUME_AND_MUTE;
	cmd_tpa[1] = 32;
	TPA6130_Write(cmd_tpa, 2);
	// Reg 3
	cmd_tpa[0] = TPA6130_OUTPUT_IMPEDANCE;
	cmd_tpa[1] = 0x00;
	TPA6130_Write(cmd_tpa, 2);
}

void tpa6130_set_volume(uint8_t volume)
{
	if( cfg.Vol_HeadPhone != volume) {
		cfg.Vol_HeadPhone = volume;
		headphone.volume = volume;

		cmd_tpa[0] = TPA6130_VOLUME_AND_MUTE;
		cmd_tpa[1] = volume;
		TPA6130_Write(cmd_tpa, 2);
	}
}
