/*
 * eeprom.h
 *
 *  Created on: Aug 25, 2025
 *      Author: rinaldo.santos
 */

#ifndef SRC_EEPROM_H_
#define SRC_EEPROM_H_

#define I2C_TIMEOUT_MAX 10000

// HAL expects address to be shifted one bit to the left
#define MEM_DEVICE_ADDR				(uint16_t)(0x43 << 1)

#define EEPROM_ADDRESS              MEM_DEVICE_ADDR
#define EEPROM_MAXPKT             	32              	//(page size)
//#define EEPROM_WRITE              10              	//time to wait in ms
//#define EEPROM_TIMEOUT             5*EEPROM_WRITE  	//timeout while writing
#define EEPROM_SECTIONSIZE			64

// Endereço do dispositivo - A2=0, A1=0, A0=1 = 0b1010001 = 0x51
#define EEPROM_I2C_ADDRESS    0x51
#define EEPROM_PAGE_SIZE      64
#define EEPROM_TOTAL_SIZE     32768
#define EEPROM_WRITE_DELAY    5  // ms de delay para escrita

extern I2C_HandleTypeDef hi2c1;

HAL_StatusTypeDef Read_Eeprom(uint16_t addr, uint8_t *pdata, uint16_t size);
HAL_StatusTypeDef Write_Eeprom(uint16_t addr, uint8_t *pdata, uint16_t size);
void Carrega_Prog_Default(void);
void ReadIdEeprom(void);
void ReadConfig(void);
void UpdateValores(void);
void Clear_Eeprom(void);
void Read_Enviroment(void);
void Read_Profile(void);
void ReadLicSeq(void);
void EEPROM_Example(void);
void reset_password(void);
void Calculate_UPTime(void);
void Telemetry_save(void);

#endif /* SRC_EEPROM_H_ */
