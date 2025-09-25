/*
 * eeprom.c
 *
 *  Created on: Aug 25, 2025
 *      Author: rinaldo.santos
 */

#include "main.h"
#include "cmsis_os.h"
#include "semphr.h"

#include <stdio.h>
#include "string.h"
#include "../Sinteck/src/license.h"
#include "../Sinteck/src/eeprom.h"
#include "../Sinteck/src/defines.h"
#include "../Sinteck/src/PowerControl.h"
#include "../Sinteck/src/audio.h"
#include "../Sinteck/src/PE43711.h"
#include "../Sinteck/src/AD5242.h"

extern osMutexId_t MutexI2CHandle;

extern uint8_t flag_telemetry, flag_telemetry;

uint16_t endeeprom = 0;

Cfg_var cfg;
AdvancedSettings adv;
Profile_var Profile;
SYS_UPTime uptime;
rds_var rds;
extern license_var lic;
extern RTC_DateTypeDef gDate;
extern RTC_TimeTypeDef gTime;
extern SYS_DateTime TimeInicio;

HAL_StatusTypeDef Write_Eeprom(uint16_t addr, uint8_t *pdata, uint16_t size)
{
	HAL_StatusTypeDef Result = HAL_OK;

	xSemaphoreTake(MutexI2CHandle, portMAX_DELAY);

	// Habilita Escrita na EEPROM
    HAL_GPIO_WritePin(EEPROM_HOLD_GPIO_Port, EEPROM_HOLD_Pin, GPIO_PIN_RESET);

    Result = HAL_I2C_IsDeviceReady(&hi2c1, MEM_DEVICE_ADDR, 64, HAL_MAX_DELAY);
    if(Result != HAL_OK) goto errweeprom;

    Result = HAL_I2C_Mem_Write(&hi2c1, MEM_DEVICE_ADDR, addr, I2C_MEMADD_SIZE_16BIT, pdata, size, HAL_MAX_DELAY);
    if(Result != HAL_OK) goto errweeprom;

errweeprom:
    // Desabilita Escrita na EEPROM
    HAL_GPIO_WritePin(EEPROM_HOLD_GPIO_Port, EEPROM_HOLD_Pin, GPIO_PIN_SET);
    xSemaphoreGive(MutexI2CHandle);
    osDelay(12);

    return Result;
}

HAL_StatusTypeDef Read_Eeprom(uint16_t addr, uint8_t *pdata, uint16_t size)
{
	HAL_StatusTypeDef Result = HAL_OK;

	xSemaphoreTake(MutexI2CHandle, portMAX_DELAY);

	Result = HAL_I2C_IsDeviceReady(&hi2c1, MEM_DEVICE_ADDR, 64, HAL_MAX_DELAY);
	if(Result != HAL_OK) goto errreeprom;

	Result = HAL_I2C_Mem_Read(&hi2c1, MEM_DEVICE_ADDR, addr, I2C_MEMADD_SIZE_16BIT, pdata, size, HAL_MAX_DELAY);
	if(Result != HAL_OK) goto errreeprom;

errreeprom:
	xSemaphoreGive(MutexI2CHandle);
	osDelay(6);

    return Result;
}

HAL_StatusTypeDef EEPROM_Write(uint16_t memAddress, uint8_t *data, uint16_t size)
{
    HAL_StatusTypeDef status;
    uint16_t bytesWritten = 0;
    uint16_t bytesToWrite;

    while (bytesWritten < size)
    {
    	// Habilita Escrita na EEPROM
        HAL_GPIO_WritePin(EEPROM_HOLD_GPIO_Port, EEPROM_HOLD_Pin, GPIO_PIN_RESET);

    	// Calcula quantos bytes podem ser escritos na página atual
        bytesToWrite = EEPROM_PAGE_SIZE - (memAddress % EEPROM_PAGE_SIZE);
        if (bytesToWrite > (size - bytesWritten))
        {
            bytesToWrite = size - bytesWritten;
        }

        // Escreve os dados
        status = HAL_I2C_Mem_Write(&hi2c1,
                                  EEPROM_I2C_ADDRESS << 1,
                                  memAddress,
                                  I2C_MEMADD_SIZE_16BIT,
                                  &data[bytesWritten],
                                  bytesToWrite,
                                  100);

    	// Habilita Escrita na EEPROM
        HAL_GPIO_WritePin(EEPROM_HOLD_GPIO_Port, EEPROM_HOLD_Pin, GPIO_PIN_SET);

        if (status != HAL_OK)
        {
            return status;
        }

        // Aguarda a escrita ser completada
        osDelay(EEPROM_WRITE_DELAY);

        bytesWritten += bytesToWrite;
        memAddress += bytesToWrite;
    }

    return HAL_OK;
}

HAL_StatusTypeDef EEPROM_Read(uint16_t memAddress, uint8_t *data, uint16_t size)
{
    return HAL_I2C_Mem_Read(&hi2c1,
                           EEPROM_I2C_ADDRESS << 1,
                           memAddress,
                           I2C_MEMADD_SIZE_16BIT,
                           data,
                           size,
                           100);
}

HAL_StatusTypeDef EEPROM_SequentialRead(uint16_t memAddress, uint8_t *data, uint16_t size)
{
    HAL_StatusTypeDef status;

    // Envia endereço de memória
    status = HAL_I2C_Mem_Write(&hi2c1,
                              EEPROM_I2C_ADDRESS << 1,
                              memAddress,
                              I2C_MEMADD_SIZE_16BIT,
                              NULL,
                              0,
                              100);

    if (status != HAL_OK)
    {
        return status;
    }

    // Lê dados sequencialmente
    return HAL_I2C_Master_Receive(&hi2c1,
                                 EEPROM_I2C_ADDRESS << 1,
                                 data,
                                 size,
                                 100);
}

HAL_StatusTypeDef EEPROM_IsReady(void)
{
    return HAL_I2C_IsDeviceReady(&hi2c1,
                                EEPROM_I2C_ADDRESS << 1,
                                3,  // 3 tentativas
                                100); // timeout 100ms
}

void EEPROM_Example(void)
{
    uint8_t writeData[] = "Hello EEPROM!";
    uint8_t readData[20] = {0};
    HAL_StatusTypeDef status;

    // Verifica se EEPROM está pronta
    if (EEPROM_IsReady() == HAL_OK)
    {
        printf("EEPROM detectada!\n");
    }
    else
    {
        printf("EEPROM não respondendo!\n");
        return;
    }

    // Escreve dados no endereço 0x100
    status = EEPROM_Write(0x100, writeData, sizeof(writeData));
    if (status == HAL_OK)
    {
        printf("Dados escritos com sucesso!\n");
    }

    // Lê os dados
    status = EEPROM_Read(0x100, readData, sizeof(writeData));
    if (status == HAL_OK)
    {
        printf("Dados lidos: %s\n", readData);
    }

    // Limpa um bloco específico
    uint8_t clearData[32] = {0};
    EEPROM_Write(0x200, clearData, 32);
}

void Carrega_Prog_Default(void)
{
	cfg.EepromID = 0x55AA;
	cfg.prg_rev = PRG_REVISION;
	cfg.model_type = MODELO;
	cfg.servico = 0;

	// Profile
	memset(Profile.Station, 0, 50);
	memset(Profile.City, 0, 50);
	memset(Profile.State, 0, 50);
	memset(Profile.Country, 0, 50);

	// Token
	if(cfg.SerialNumber[0] == 0xFF || cfg.SerialNumber[1] == 0xFF || cfg.SerialNumber[2] == 0xFF || cfg.SerialNumber[3] == 0xFF ||
	   cfg.SerialNumber[4] == 0xFF || cfg.SerialNumber[5] == 0xFF || cfg.SerialNumber[6] == 0xFF || cfg.SerialNumber[7] == 0xFF ) {

		cfg.Token[0] = '0';
		cfg.Token[1] = '0';
		cfg.Token[2] = '0';
		cfg.Token[3] = '0';
		cfg.Token[4] = '0';
		cfg.Token[5] = '0';
		cfg.Token[6] = '0';
		cfg.Token[7] = '0';
		cfg.Token[8] = 0;
		cfg.Token[12] = 1;

	}
	else {
		cfg.Token[0] = cfg.SerialNumber[0];
		cfg.Token[1] = cfg.SerialNumber[1];
		cfg.Token[2] = cfg.SerialNumber[2];
		cfg.Token[3] = cfg.SerialNumber[3];
		cfg.Token[4] = cfg.SerialNumber[4];
		cfg.Token[5] = cfg.SerialNumber[5];
		cfg.Token[6] = cfg.SerialNumber[6];
		cfg.Token[7] = cfg.SerialNumber[7];
		cfg.Token[8] = 0;
		cfg.Token[12] = 1;
	}

	// RF
	cfg.modo = 0;
	cfg.BW = 0;
	cfg.Atten = 0.0f;
	RF_Disable();

	// Audio
	cfg.Processor = 1;
    cfg.Emphase = 0;
    cfg.AES192 = 0;
    cfg.AudioSource = 1;
	cfg.Vol_MPX1 = 48;
	cfg.Vol_MPX2 = 48;
	// Switch Ausencia Audio
	cfg.timer_audio_on = 1000 * 60 * 3;
	cfg.timer_audio_off = 1000 * 60 * 5;
	cfg.level_audio_on = 0;
	cfg.level_audio_off = 0;

	// frequencia
	cfg.Frequencia = 937500;

	// NetWork
	cfg.EnableSNMP = 0;
	cfg.PortWEB = 80;
	cfg.IP_ADDR[0] = 192;   cfg.IP_ADDR[1] = 168;   cfg.IP_ADDR[2] = 10;    cfg.IP_ADDR[3] = 17;
	cfg.MASK_ADDR[0] = 255; cfg.MASK_ADDR[1] = 255; cfg.MASK_ADDR[2] = 255; cfg.MASK_ADDR[3] = 0;
	cfg.GW_ADDR[0] = 192;   cfg.GW_ADDR[1] = 168;   cfg.GW_ADDR[2] = 10;    cfg.GW_ADDR[3] = 250;
	cfg.DNS_ADDR[0] = 192;  cfg.DNS_ADDR[1] = 168;  cfg.DNS_ADDR[2] = 10;   cfg.DNS_ADDR[3] = 250;

	// Timezone
	cfg.NTP = 0;
	cfg.Timezone = 9;

	//Password Admin/User
	cfg.PassAdmin[0] = 1; cfg.PassAdmin[1] = 2; cfg.PassAdmin[2] = 3; cfg.PassAdmin[3] = 4;
	cfg.PassUser[0]  = 1; cfg.PassUser[1]  = 2; cfg.PassUser[2]  = 3; cfg.PassUser[3]  = 4;

	// VSWR NULL
	cfg.ConfigHold = 0;

	// RDS
	//
	rds.enable = 0;			// Enable / Disable RDS
	rds.enable_station = 0;
	rds.enable_text = 0;
	rds.ms = 0;				// Music / Speech
	rds.pty = 0;			// Program Type Number
	rds.tp = 0;				// Traffic Program
	rds.type = 0;			// Tipo RDS(EU) or RBDS
	rds.ct = 0;				// Enable / Disable DateTime

	// Station Name 8 Pos
	rds.ps[0] = 0; rds.ps[1] = 0; rds.ps[2] = 0; rds.ps[3] = 0;
	rds.ps[4] = 0; rds.ps[5] = 0; rds.ps[6] = 0; rds.ps[7] = 0;
	rds.ps[8] = 0;   rds.ps[9] = 0;

	// Trafic PS
	rds.tps[0] = 0; rds.tps[1] = 0; rds.tps[2] = 0; rds.tps[3] = 0;
	rds.tps[4] = 0; rds.tps[5] = 0; rds.tps[6] = 0; rds.tps[7] = 0;
	rds.tps[8] = 0;   rds.tps[9] = 0;

	// Program Type Name 8 Caracteres
	rds.ptyn[0] = 0; rds.ptyn[1] = 0; rds.ptyn[2] = 0; rds.ptyn[3] = 0;
	rds.ptyn[4] = 0; rds.ptyn[5] = 0; rds.ptyn[6] = 0; rds.ptyn[7] = 0;
	rds.ptyn[8] = 0;   rds.ptyn[9] = 0;

	// Program Identification
	rds.pi[0] = '1'; rds.pi[1] = '2'; rds.pi[2] = '3'; rds.pi[3] = '4';

	// Alternative frequency
	rds.af[0] = 0; rds.af[1] = 0; rds.af[2] = 0; rds.af[3] = 0; rds.af[4] = 0; rds.af[5] = 0;

	// Text
	memset(rds.dps1, 0, 72);		// Dynamic Text 72 Caracteres
	memset(rds.rt1, 0, 72);			// Text 1  72 Caracteres

	// Advanced Settings
	// Offset
	adv.GainRSSI1  = 1.00f;
	adv.GainRSSI2  = 1.00f;
	adv.GainMPX    = 1.00f;
	adv.GainLeft   = 1.00f;
	adv.GainRight  = 1.00f;

	// Atenuação
	cfg.Atten = 0.0f;

	// Grava em EEPROM
	EEPROM_Write(ADDR_ID,       (uint8_t*)&cfg.EepromID,           2);
	EEPROM_Write(ADDR_REV,      (uint8_t*)&cfg.prg_rev,            sizeof(cfg.prg_rev));
	EEPROM_Write(ADDR_MODEL,    (uint8_t*)&cfg.model_type,         sizeof(cfg.model_type));
	EEPROM_Write(ADDR_SERV,     (uint8_t*)&cfg.servico,            1);
	// RF
	EEPROM_Write(ADDR_MODO,     (uint8_t*)&cfg.modo,               1);
	// AUDIO
	EEPROM_Write(ADDR_PROCESSOR, (uint8_t*)&cfg.Processor,         1);
	EEPROM_Write(ADDR_EMPHASE,   (uint8_t*)&cfg.Emphase,           1);
	EEPROM_Write(ADDR_AES192,    (uint8_t*)&cfg.AES192,            1);
	EEPROM_Write(ADDR_AUDIO,     (uint8_t*)&cfg.AudioSource,       1);
	EEPROM_Write(ADDR_VOLMPX1,   (uint8_t*)&cfg.Vol_MPX1,          1);
	EEPROM_Write(ADDR_VOLMPX2,   (uint8_t*)&cfg.Vol_MPX2,          1);
	EEPROM_Write(ADDR_TIMERON,   (uint8_t*)&cfg.timer_audio_on,    4);
	EEPROM_Write(ADDR_TIMEROFF,  (uint8_t*)&cfg.timer_audio_off,   4);
	EEPROM_Write(ADDR_LEVELON,   (uint8_t*)&cfg.level_audio_on,    1);
	EEPROM_Write(ADDR_LEVELOFF,  (uint8_t*)&cfg.level_audio_off,   1);
	// frequencia
	EEPROM_Write(ADDR_FREQ,      (uint8_t*)&cfg.Frequencia,        4);
	// NetWork
	EEPROM_Write(ADDR_SNMP,      (uint8_t*)&cfg.EnableSNMP,        1);
	EEPROM_Write(ADDR_PORTWEB,   (uint8_t*)&cfg.PortWEB,           2);
	EEPROM_Write(ADDR_IPADDR,    (uint8_t*)&cfg.IP_ADDR,           4);
	EEPROM_Write(ADDR_MASKADDR,  (uint8_t*)&cfg.MASK_ADDR,         4);
	EEPROM_Write(ADDR_GWADDR,    (uint8_t*)&cfg.GW_ADDR,           4);
	EEPROM_Write(ADDR_DNSADDR,   (uint8_t*)&cfg.DNS_ADDR,          4);
	// Timezone
	EEPROM_Write(ADDR_CONFIGNTP,  (uint8_t*)&cfg.NTP,              1);
	EEPROM_Write(ADDR_TIMEZONE,   (uint8_t*)&cfg.Timezone,         1);
	//Password Admin/User
	EEPROM_Write(ADDR_PASSADMIN,  (uint8_t*)&cfg.PassAdmin,        4);
	EEPROM_Write(ADDR_PASSUSER,   (uint8_t*)&cfg.PassUser,         4);
	// VSWR NULL
	EEPROM_Write(ADDR_CONFIGHOLD, (uint8_t*)&cfg.ConfigHold,       1);
	// RDS
	EEPROM_Write(ADDR_RDS_ENABLE, (uint8_t*)&rds.enable,           1);
	EEPROM_Write(ADDR_RDS_EN_STATION, (uint8_t*)&rds.enable_station,   1);
	EEPROM_Write(ADDR_RDS_EN_TEXT,    (uint8_t*)&rds.enable_text,      1);
	EEPROM_Write(ADDR_RDS_MS,     (uint8_t*)&rds.ms,               1);
	EEPROM_Write(ADDR_RDS_PTY,    (uint8_t*)&rds.pty,              1);
	EEPROM_Write(ADDR_RDS_TP,     (uint8_t*)&rds.tp,               1);
	EEPROM_Write(ADDR_RDS_TYPE,   (uint8_t*)&rds.type,             1);
	EEPROM_Write(ADDR_RDS_CT,     (uint8_t*)&rds.ct,               1);
	EEPROM_Write(ADDR_RDS_PI,     (uint8_t*)&rds.pi,               4);
	EEPROM_Write(ADDR_RDS_AF,     (uint8_t*)&rds.af,               4);
	EEPROM_Write(ADDR_RDS_PS,     (uint8_t*)&rds.ps,               10);
	EEPROM_Write(ADDR_RDS_TPS,    (uint8_t*)&rds.tps,              10);
	EEPROM_Write(ADDR_RDS_PTYN,   (uint8_t*)&rds.ptyn,             10);
	EEPROM_Write(ADDR_RDS_DYN,    (uint8_t*)&rds.dps1,             72);
	EEPROM_Write(ADDR_RDS_RT,     (uint8_t*)&rds.rt1,              72);

	// Advanced Settings
	EEPROM_Write(ADDR_ADV_RSSI1,  (uint8_t*)&adv.GainRSSI1, sizeof(adv.GainRSSI1));
	EEPROM_Write(ADDR_ADV_RSSI2,  (uint8_t*)&adv.GainRSSI2, sizeof(adv.GainRSSI2));
	EEPROM_Write(ADDR_ADV_MPX,  (uint8_t*)&adv.GainMPX, sizeof(adv.GainMPX));
	EEPROM_Write(ADDR_ADV_LEFT,  (uint8_t*)&adv.GainLeft, sizeof(adv.GainLeft));
	EEPROM_Write(ADDR_ADV_RIGHT, (uint8_t*)&adv.GainRight, sizeof(adv.GainRight));

	// Salva Profille
	EEPROM_Write(ADDR_PROF_STATION, (uint8_t*)&Profile.Station, 50);
	EEPROM_Write(ADDR_PROF_CITY,    (uint8_t*)&Profile.City,    50);
	EEPROM_Write(ADDR_PROF_STATE,   (uint8_t*)&Profile.State,   50);
	EEPROM_Write(ADDR_PROF_COUNTRY, (uint8_t*)&Profile.Country, 50);

	EEPROM_Write(ADDR_CFG_BW, (uint8_t*)&cfg.BW, 1);
	EEPROM_Write(ADDR_CFG_ATN, (uint8_t*)&cfg.Atten, 4);
}

void ReadIdEeprom(void)
{
	EEPROM_Read(ADDR_ID, (uint8_t*)&cfg.EepromID, 2);
	EEPROM_Read(ADDR_REV, (uint8_t*)&cfg.prg_rev, sizeof(cfg.prg_rev));
	EEPROM_Read(ADDR_MODEL, (uint8_t*)&cfg.model_type, sizeof(cfg.model_type));
	EEPROM_Read(ADDR_SERV, (uint8_t*)&cfg.servico, 1);
}

void ReadConfig(void)
{
	// Leitura em EEPROM
	EEPROM_Read(ADDR_ID, (uint8_t*)&cfg.EepromID, 2);
	EEPROM_Read(ADDR_REV, (uint8_t*)&cfg.prg_rev, sizeof(cfg.prg_rev));
	EEPROM_Read(ADDR_MODEL, (uint8_t*)&cfg.model_type, sizeof(cfg.model_type));
	// RF
	EEPROM_Read(ADDR_MODO,     (uint8_t*)&cfg.modo,               1);
	// AUDIO
	EEPROM_Read(ADDR_PROCESSOR, (uint8_t*)&cfg.Processor,         1);
	EEPROM_Read(ADDR_EMPHASE,   (uint8_t*)&cfg.Emphase,           1);
	EEPROM_Read(ADDR_AES192,    (uint8_t*)&cfg.AES192,            1);
	EEPROM_Read(ADDR_AUDIO,     (uint8_t*)&cfg.AudioSource,       1);
	EEPROM_Read(ADDR_VOLMPX1,   (uint8_t*)&cfg.Vol_MPX1,          1);
	EEPROM_Read(ADDR_VOLMPX2,   (uint8_t*)&cfg.Vol_MPX2,          1);
	EEPROM_Read(ADDR_TIMERON,   (uint8_t*)&cfg.timer_audio_on,    4);
	EEPROM_Read(ADDR_TIMEROFF,  (uint8_t*)&cfg.timer_audio_off,   4);
	EEPROM_Read(ADDR_LEVELON,   (uint8_t*)&cfg.level_audio_on,    1);
	EEPROM_Read(ADDR_LEVELOFF,  (uint8_t*)&cfg.level_audio_off,   1);
	// frequencia
	EEPROM_Read(ADDR_FREQ,      (uint8_t*)&cfg.Frequencia,        4);
	// NetWork
	EEPROM_Read(ADDR_SNMP,      (uint8_t*)&cfg.EnableSNMP,        1);
	EEPROM_Read(ADDR_PORTWEB,   (uint8_t*)&cfg.PortWEB,           2);
	EEPROM_Read(ADDR_IPADDR,    (uint8_t*)&cfg.IP_ADDR,           4);
	EEPROM_Read(ADDR_MASKADDR,  (uint8_t*)&cfg.MASK_ADDR,         4);
	EEPROM_Read(ADDR_GWADDR,    (uint8_t*)&cfg.GW_ADDR,           4);
	EEPROM_Read(ADDR_DNSADDR,   (uint8_t*)&cfg.DNS_ADDR,          4);
	// Timezone
	EEPROM_Read(ADDR_CONFIGNTP,  (uint8_t*)&cfg.NTP,              1);
	EEPROM_Read(ADDR_TIMEZONE,   (uint8_t*)&cfg.Timezone,         1);
	//Password Admin/User
	EEPROM_Read(ADDR_PASSADMIN,  (uint8_t*)&cfg.PassAdmin,        4);
	EEPROM_Read(ADDR_PASSUSER,   (uint8_t*)&cfg.PassUser,         4);
	// VSWR NULL
	EEPROM_Read(ADDR_CONFIGHOLD, (uint8_t*)&cfg.ConfigHold,       1);
	// RDS
	EEPROM_Read(ADDR_RDS_ENABLE, (uint8_t*)&rds.enable,           1);
	EEPROM_Read(ADDR_RDS_EN_STATION, (uint8_t*)&rds.enable_station,   1);
	EEPROM_Read(ADDR_RDS_EN_TEXT,    (uint8_t*)&rds.enable_text,      1);
	EEPROM_Read(ADDR_RDS_MS,     (uint8_t*)&rds.ms,               1);
	EEPROM_Read(ADDR_RDS_PTY,    (uint8_t*)&rds.pty,              1);
	EEPROM_Read(ADDR_RDS_TP,     (uint8_t*)&rds.tp,               1);
	EEPROM_Read(ADDR_RDS_TYPE,   (uint8_t*)&rds.type,             1);
	EEPROM_Read(ADDR_RDS_CT,     (uint8_t*)&rds.ct,               1);
	EEPROM_Read(ADDR_RDS_PI,     (uint8_t*)&rds.pi,               4);
	EEPROM_Read(ADDR_RDS_AF,     (uint8_t*)&rds.af,               4);
	EEPROM_Read(ADDR_RDS_PS,     (uint8_t*)&rds.ps,               10);
	EEPROM_Read(ADDR_RDS_TPS,    (uint8_t*)&rds.tps,              10);
	EEPROM_Read(ADDR_RDS_PTYN,   (uint8_t*)&rds.ptyn,             10);
	EEPROM_Read(ADDR_RDS_DYN,    (uint8_t*)&rds.dps1,             72);
	EEPROM_Read(ADDR_RDS_RT,     (uint8_t*)&rds.rt1,              72);

	// Advanced Settings
	EEPROM_Read(ADDR_ADV_RSSI1, (uint8_t*)&adv.GainRSSI1, sizeof(adv.GainRSSI1));
	EEPROM_Read(ADDR_ADV_RSSI2, (uint8_t*)&adv.GainRSSI2, sizeof(adv.GainRSSI2));
	EEPROM_Read(ADDR_ADV_MPX,   (uint8_t*)&adv.GainMPX, sizeof(adv.GainMPX));
	EEPROM_Read(ADDR_ADV_LEFT,  (uint8_t*)&adv.GainLeft, sizeof(adv.GainLeft));
	EEPROM_Read(ADDR_ADV_RIGHT, (uint8_t*)&adv.GainRight, sizeof(adv.GainRight));

	// Serial Number
	EEPROM_Read(ADDR_SN, (uint8_t*)&cfg.SerialNumber[0], 8);
	// Token
	if(cfg.SerialNumber[0] == 0xFF || cfg.SerialNumber[1] == 0xFF || cfg.SerialNumber[2] == 0xFF || cfg.SerialNumber[3] == 0xFF ||
	   cfg.SerialNumber[4] == 0xFF || cfg.SerialNumber[5] == 0xFF || cfg.SerialNumber[6] == 0xFF || cfg.SerialNumber[7] == 0xFF ) {

		cfg.Token[0] = '0';
		cfg.Token[1] = '0';
		cfg.Token[2] = '0';
		cfg.Token[3] = '0';
		cfg.Token[4] = '0';
		cfg.Token[5] = '0';
		cfg.Token[6] = '0';
		cfg.Token[7] = '0';
		cfg.Token[8] = 0;
		cfg.Token[12] = 1;
	}
	else {
		cfg.Token[0] = cfg.SerialNumber[0];
		cfg.Token[1] = cfg.SerialNumber[1];
		cfg.Token[2] = cfg.SerialNumber[2];
		cfg.Token[3] = cfg.SerialNumber[3];
		cfg.Token[4] = cfg.SerialNumber[4];
		cfg.Token[5] = cfg.SerialNumber[5];
		cfg.Token[6] = cfg.SerialNumber[6];
		cfg.Token[7] = cfg.SerialNumber[7];
		cfg.Token[8] = 0;
		cfg.Token[12] = 1;
	}

	// License
	EEPROM_Read(ADDR_LIC, (uint8_t*)&cfg.License[0], 8);

	EEPROM_Read(ADDR_LIC_SEQ, (uint8_t*)&lic.LicSeq, 4);
	if(lic.LicSeq > 99) lic.LicSeq = 0;
	EEPROM_Read(ADDR_TMR_LIC, (uint8_t*)&lic.licenseTimer, 4);
	if(lic.licenseTimer > 999) lic.licenseTimer = 999;
	if(lic.licenseTimer == 0 && (lic.LicSeq > 0 && lic.LicSeq <= 98) ) {
		//falha |= 1ULL << FAIL_LICENSE;
		//Alarme_Ins(FAIL_LICENSE, 0, 0);
	}

	EEPROM_Read(ADDR_CFG_BW, (uint8_t*)&cfg.BW, 1);
	EEPROM_Read(ADDR_CFG_ATN, (uint8_t*)&cfg.Atten, 4);

	 UpdateValores();
}

void UpdateValores(void)
{
	// Update Valores
	// Processor
	if(cfg.Processor) {
		HAL_GPIO_WritePin(DSP__MONO_GPIO_Port, DSP__MONO_Pin, GPIO_PIN_SET);
	}
	else {
		HAL_GPIO_WritePin(DSP__MONO_GPIO_Port, DSP__MONO_Pin, GPIO_PIN_RESET);
	}
	// EMPHASE
	if(cfg.Emphase) {
		HAL_GPIO_WritePin(SEL_75_50_GPIO_Port, SEL_75_50_Pin, GPIO_PIN_SET);
	}
	else {
		HAL_GPIO_WritePin(SEL_75_50_GPIO_Port, SEL_75_50_Pin, GPIO_PIN_RESET);
	}

	// AES192
	if(cfg.AES192) {
		HAL_GPIO_WritePin(AES192_GPIO_Port, AES192_Pin, GPIO_PIN_SET);
	}
	else {
		HAL_GPIO_WritePin(AES192_GPIO_Port, AES192_Pin, GPIO_PIN_RESET);
	}

	// Atauliza Chip
	PE43711(cfg.Atten);
	Write_AD5242(AD524X_RDAC0, cfg.Vol_MPX1, 0, 0);
	Write_AD5242(AD524X_RDAC1, cfg.Vol_MPX2, 0, 0);
}

void Read_Enviroment(void)
{
	// Serial Number
	EEPROM_Read(ADDR_SN, (uint8_t*)&cfg.SerialNumber[0], 8);

	// License
	EEPROM_Read(ADDR_LIC, (uint8_t*)&cfg.License[0], 8);
	EEPROM_Read(ADDR_LIC_SEQ, (uint8_t*)&lic.LicSeq, 4);
	EEPROM_Read(ADDR_TMR_LIC, (uint8_t*)&lic.licenseTimer, 4);
}

void Save_Profile(void)
{
	EEPROM_Write(ADDR_PROF_STATION, (uint8_t*)&Profile.Station, 50);
	EEPROM_Write(ADDR_PROF_CITY,    (uint8_t*)&Profile.City,    50);
	EEPROM_Write(ADDR_PROF_STATE,   (uint8_t*)&Profile.State,   50);
	EEPROM_Write(ADDR_PROF_COUNTRY, (uint8_t*)&Profile.Country, 50);
}

void Read_Profile(void)
{
	EEPROM_Read(ADDR_PROF_STATION, (uint8_t*)&Profile.Station, 50);
	EEPROM_Read(ADDR_PROF_CITY,    (uint8_t*)&Profile.City,    50);
	EEPROM_Read(ADDR_PROF_STATE,   (uint8_t*)&Profile.State,   50);
	EEPROM_Read(ADDR_PROF_COUNTRY, (uint8_t*)&Profile.Country, 50);
}

void ReadLicSeq(void)
{
	EEPROM_Read(ADDR_LIC_SEQ, (uint8_t*)&lic.LicSeq, 4);
}

HAL_StatusTypeDef EEPROM_Clear(void)
{
    uint8_t blankData[64] = {0};
    uint16_t address = 0;
    HAL_StatusTypeDef status;

    while (address < EEPROM_TOTAL_SIZE)
    {
        status = EEPROM_Write(address, blankData,
                             (EEPROM_TOTAL_SIZE - address) > 64 ? 64 : (EEPROM_TOTAL_SIZE - address));

        if (status != HAL_OK)
        {
            return status;
        }

        address += 64;
        HAL_Delay(EEPROM_WRITE_DELAY);
    }

    return HAL_OK;
}


void Telemetry_save(void)
{
	switch(flag_telemetry) {
		case 0:
			break;
		case 1:
			flag_telemetry = 0;
			break;
		case 2:
			EEPROM_Write(ADDR_PROCESSOR, (uint8_t*)&cfg.Processor,         1);
			EEPROM_Write(ADDR_EMPHASE,   (uint8_t*)&cfg.Emphase,           1);
			EEPROM_Write(ADDR_AES192,    (uint8_t*)&cfg.AES192,            1);
			EEPROM_Write(ADDR_AUDIO,     (uint8_t*)&cfg.AudioSource,       1);
        	flag_telemetry = 0;
			break;
		case 3:
			flag_telemetry = 0;
			break;
		case 4:
			// Timezone
			EEPROM_Write(ADDR_CONFIGNTP,  (uint8_t*)&cfg.NTP,              1);
			EEPROM_Write(ADDR_TIMEZONE,   (uint8_t*)&cfg.Timezone,         1);
			flag_telemetry = 0;
			break;
		case 5:
			// RDS EEPROM
			EEPROM_Write(ADDR_RDS_ENABLE, (uint8_t*)&rds.enable,           1);
			EEPROM_Write(ADDR_RDS_EN_STATION, (uint8_t*)&rds.enable_station,   1);
			EEPROM_Write(ADDR_RDS_EN_TEXT,    (uint8_t*)&rds.enable_text,      1);
			EEPROM_Write(ADDR_RDS_MS,     (uint8_t*)&rds.ms,               1);
			EEPROM_Write(ADDR_RDS_PTY,    (uint8_t*)&rds.pty,              1);
			EEPROM_Write(ADDR_RDS_TP,     (uint8_t*)&rds.tp,               1);
			EEPROM_Write(ADDR_RDS_TYPE,   (uint8_t*)&rds.type,             1);
			EEPROM_Write(ADDR_RDS_CT,     (uint8_t*)&rds.ct,               1);
			EEPROM_Write(ADDR_RDS_PI,     (uint8_t*)&rds.pi,               4);
			EEPROM_Write(ADDR_RDS_AF,     (uint8_t*)&rds.af,               4);
			EEPROM_Write(ADDR_RDS_PS,     (uint8_t*)&rds.ps,               10);
			EEPROM_Write(ADDR_RDS_TPS,    (uint8_t*)&rds.tps,              10);
			EEPROM_Write(ADDR_RDS_PTYN,   (uint8_t*)&rds.ptyn,             10);
			EEPROM_Write(ADDR_RDS_DYN,    (uint8_t*)&rds.dps1,             72);
			EEPROM_Write(ADDR_RDS_RT,     (uint8_t*)&rds.rt1,              72);
			flag_telemetry = 22;
			break;
		case 6:
			EEPROM_Write(ADDR_VOLMPX1,   (uint8_t*)&cfg.Vol_MPX1,          1);
			EEPROM_Write(ADDR_VOLMPX2,   (uint8_t*)&cfg.Vol_MPX2,          1);
			flag_telemetry = 0;
			break;
		case 7:
			EEPROM_Write(ADDR_FREQ, (uint8_t*)&cfg.Frequencia, sizeof(cfg.Frequencia));
			flag_telemetry = 0;
			break;
		case 8:
			flag_telemetry = 0;
			break;
		case 9:
			//Password Admin/User
			EEPROM_Write(ADDR_PASSADMIN,  (uint8_t*)&cfg.PassAdmin,        4);
			EEPROM_Write(ADDR_PASSUSER,   (uint8_t*)&cfg.PassUser,         4);
			flag_telemetry = 0;
			break;
		case 10:
			// VSWR NULL
			EEPROM_Write(ADDR_CONFIGHOLD, (uint8_t*)&cfg.ConfigHold,       1);
			flag_telemetry = 0;
			break;
		case 11:
			// Advanced Settings
			EEPROM_Write(ADDR_ADV_RSSI1,  (uint8_t*)&adv.GainRSSI1, sizeof(adv.GainRSSI1));
			EEPROM_Write(ADDR_ADV_RSSI2,  (uint8_t*)&adv.GainRSSI2, sizeof(adv.GainRSSI2));
			EEPROM_Write(ADDR_ADV_MPX,    (uint8_t*)&adv.GainMPX,   sizeof(adv.GainMPX));
			EEPROM_Write(ADDR_ADV_LEFT,   (uint8_t*)&adv.GainLeft,  sizeof(adv.GainLeft));
			EEPROM_Write(ADDR_ADV_RIGHT,  (uint8_t*)&adv.GainRight, sizeof(adv.GainRight));
			flag_telemetry = 0;
			break;
		case 12:
			// NetWork
			EEPROM_Write(ADDR_SNMP,      (uint8_t*)&cfg.EnableSNMP,        1);
			EEPROM_Write(ADDR_PORTWEB,   (uint8_t*)&cfg.PortWEB,           2);
			EEPROM_Write(ADDR_IPADDR,    (uint8_t*)&cfg.IP_ADDR,           4);
			EEPROM_Write(ADDR_MASKADDR,  (uint8_t*)&cfg.MASK_ADDR,         4);
			EEPROM_Write(ADDR_GWADDR,    (uint8_t*)&cfg.GW_ADDR,           4);
			EEPROM_Write(ADDR_DNSADDR,   (uint8_t*)&cfg.DNS_ADDR,          4);
			break;
		case 13:
			flag_telemetry = 0;
			break;
		case 14:
			flag_telemetry = 0;
			break;
		case 15:
			flag_telemetry = 0;
			break;
		case 16:
			flag_telemetry = 0;
			break;
		case 17:
			EEPROM_Write(ADDR_SN, (uint8_t*)&cfg.SerialNumber[0], 8);
			flag_telemetry = 0;
			break;
		case 18:
			flag_telemetry = 0;
			break;
		case 19:
			// grava Operacao/Servico
			EEPROM_Write(ADDR_SERV, (uint8_t*)&cfg.servico, 1);
			flag_telemetry = 0;
			break;
		case 20:
			flag_telemetry = 0;
			break;
		case 21:
			flag_telemetry = 0;
			break;
		case 22:
			flag_telemetry = 0;
			break;
		case 23:
			flag_telemetry = 0;
			break;
		case 24:
			flag_telemetry = 0;
			break;
		case 25:
			flag_telemetry = 0;
			break;
		case 26:
			flag_telemetry = 0;
			break;
		case 27:
			Save_Profile();
			flag_telemetry = 0;
			break;
		case 28:
			// License
			EEPROM_Write(ADDR_LIC, (uint8_t*)&cfg.License[0], 8);
			EEPROM_Write(ADDR_LIC_SEQ, (uint8_t*)&lic.LicSeq, 4);
			EEPROM_Write(ADDR_TMR_LIC, (uint8_t*)&lic.licenseTimer, 4);
			flag_telemetry = 0;
			break;
		case 29:
			flag_telemetry = 0;
			break;
		case 30:
			flag_telemetry = 0;
			break;
		case 31:
			EEPROM_Write(ADDR_SNMP,      (uint8_t*)&cfg.EnableSNMP,        1);
			flag_telemetry = 0;
			break;
		case 32:
			flag_telemetry = 0;
			break;
		case 33:
			EEPROM_Write(ADDR_TIMERON,   (uint8_t*)&cfg.timer_audio_on,    4);
			EEPROM_Write(ADDR_TIMEROFF,  (uint8_t*)&cfg.timer_audio_off,   4);
			EEPROM_Write(ADDR_LEVELON,   (uint8_t*)&cfg.level_audio_on,    1);
			EEPROM_Write(ADDR_LEVELOFF,  (uint8_t*)&cfg.level_audio_off,   1);
			flag_telemetry = 0;
			break;
		case 34:
			flag_telemetry = 0;
			break;
		case 35:
			flag_telemetry = 0;
			break;
		case 36:
			flag_telemetry = 0;
		    break;
		case 37:
			uint32_t retlic = CheckLicense();
			if(retlic != 0) {
				EEPROM_Write(ADDR_LIC, (uint8_t*)&cfg.License[0], 8);
				lic.LicSeq = lic.myseq;
				EEPROM_Write(ADDR_LIC_SEQ, (uint8_t*)&lic.LicSeq, 4);
				lic.licenseTimer = retlic;
				EEPROM_Write(ADDR_TMR_LIC, (uint8_t*)&lic.licenseTimer, 4);
			}
			flag_telemetry = 0;
			break;
		case 38:
			EEPROM_Write(ADDR_CFG_BW, (uint8_t*)&cfg.BW, 1);
			EEPROM_Write(ADDR_CFG_ATN, (uint8_t*)&cfg.Atten, 4);
			flag_telemetry = 0;
			break;
		case 253:
			reset_password();
			flag_telemetry = 0;
			break;
		default:
			flag_telemetry = 0;
			break;
	}
}

void reset_password(void)
{
	cfg.PassAdmin[0] = 1; cfg.PassAdmin[1] = 2; cfg.PassAdmin[2] = 3; cfg.PassAdmin[3] = 4;
	cfg.PassUser[0] = 1;  cfg.PassUser[1] = 2;  cfg.PassUser[2] = 3;  cfg.PassUser[3] = 4;

	//Password Admin/User
	EEPROM_Read(ADDR_PASSADMIN,  (uint8_t*)&cfg.PassAdmin,        4);
	EEPROM_Read(ADDR_PASSUSER,   (uint8_t*)&cfg.PassUser,         4);
}

void Calculate_UPTime(void)
{
	uint32_t t;

	t = uptime.dia;
	uptime.dia = (((2000+gDate.Year) - TimeInicio.year) * 365) + ((gDate.Month - TimeInicio.month) * 30) + (gDate.Date - TimeInicio.day);

	if(t == uptime.dia) {
		uptime.total = ( (gTime.Hours * 3600) + (gTime.Minutes * 60) + gTime.Seconds ) - TimeInicio.total;
	}
	else {
		TimeInicio.total = (gTime.Hours * 3600) + (gTime.Minutes * 60) + gTime.Seconds;
	}
}
