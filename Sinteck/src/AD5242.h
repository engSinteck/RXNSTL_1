/*
 * AD5242.h
 *
 *  Created on: Feb 17, 2023
 *      Author: rinaldo.santos
 */

#ifndef AD5242_H_
#define AD5242_H_

#define AD524X_RDAC0        0x00
#define AD524X_RDAC1        0x80
#define AD524X_RESET        0x40
#define AD524X_SHUTDOWN     0x20
#define AD524X_O1_HIGH      0x10
#define AD524X_O2_HIGH      0x08

// HAL expects address to be shifted one bit to the left
#define AD5242_DEVICE_ADDR				(uint16_t)(0x2C << 1)
#define AD5241_DEVICE_ADDR				(uint16_t)(0x2D << 1)

HAL_StatusTypeDef AD5241_Write(uint8_t *pdata, uint8_t size);
HAL_StatusTypeDef AD5241_Read(uint8_t *pdata, uint8_t size);
HAL_StatusTypeDef AD5242_Write(uint8_t *pdata, uint8_t size);
HAL_StatusTypeDef AD5242_Read(uint8_t *pdata, uint8_t size);
void pot_write(uint8_t pot, uint8_t o1, uint8_t o2 ,uint8_t value);
void pot_write_ext(uint8_t pot, uint8_t value);
void Write_AD5242(uint8_t pot, uint8_t value, uint8_t o1,  uint8_t o2);

#endif /* AD5242_H_ */
