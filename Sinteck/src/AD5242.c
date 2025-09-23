/*
 * AD5242.c
 *
 *  Created on: Feb 17, 2023
 *      Author: rinaldo.santos
 */

#include "main.h"
#include "i2c.h"
#include "AD5242.h"

uint8_t cmd_pot[4] = {0};
uint8_t temp_O1 = 0;
uint8_t temp_O2 = 0;

HAL_StatusTypeDef AD5241_Write(uint8_t *pdata, uint8_t size)
{
	HAL_StatusTypeDef Result = HAL_OK;

	Result = HAL_I2C_IsDeviceReady(&hi2c1, AD5241_DEVICE_ADDR, 64, HAL_MAX_DELAY);
	Result = HAL_I2C_Master_Transmit(&hi2c1, AD5241_DEVICE_ADDR, pdata, size, HAL_MAX_DELAY);

	HAL_Delay(10);
    return Result;
}

HAL_StatusTypeDef AD5241_Read(uint8_t *pdata, uint8_t size)
{
	HAL_StatusTypeDef Result = HAL_OK;

	Result = HAL_I2C_IsDeviceReady(&hi2c1, AD5241_DEVICE_ADDR, 3, HAL_MAX_DELAY);
	Result = HAL_I2C_Master_Receive(&hi2c1, AD5241_DEVICE_ADDR, pdata, size, HAL_MAX_DELAY);

	return Result;
}

HAL_StatusTypeDef AD5242_Write(uint8_t *pdata, uint8_t size)
{
	HAL_StatusTypeDef Result = HAL_OK;

	Result = HAL_I2C_IsDeviceReady(&hi2c1, AD5242_DEVICE_ADDR, 64, HAL_MAX_DELAY);
	Result = HAL_I2C_Master_Transmit(&hi2c1, AD5242_DEVICE_ADDR, pdata, size, HAL_MAX_DELAY);

	HAL_Delay(10);
    return Result;
}

HAL_StatusTypeDef AD5242_Read(uint8_t *pdata, uint8_t size)
{
	HAL_StatusTypeDef Result = HAL_OK;

	Result = HAL_I2C_IsDeviceReady(&hi2c1, AD5242_DEVICE_ADDR, 3, HAL_MAX_DELAY);
	Result = HAL_I2C_Master_Receive(&hi2c1, AD5242_DEVICE_ADDR, pdata, size, HAL_MAX_DELAY);

	return Result;
}

void pot_write(uint8_t pot, uint8_t value, uint8_t o1, uint8_t o2)
{
	temp_O1 = (o1 == 0) ? 0 : AD524X_O1_HIGH;
	temp_O2 = (o2 == 0) ? 0 : AD524X_O2_HIGH;
	//
	cmd_pot[0] = (pot == AD524X_RDAC0) ? AD524X_RDAC0 : AD524X_RDAC1;
	//  apply the output lines
	cmd_pot[0] = cmd_pot[0] | temp_O1 | temp_O2;
	//
	cmd_pot[1] = value;
	AD5242_Write(cmd_pot, 2);
}

void pot_write_ext(uint8_t pot, uint8_t value)
{
	cmd_pot[0] = pot;
	cmd_pot[1] = value;
	AD5241_Write(cmd_pot, 2);
}

void Write_AD5242(uint8_t pot, uint8_t value, uint8_t o1,  uint8_t o2)
{
	pot_write(pot, value, o1, o2);
}

