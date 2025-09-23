/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define TFT_DC_Pin GPIO_PIN_3
#define TFT_DC_GPIO_Port GPIOE
#define CLK_IO_Pin GPIO_PIN_4
#define CLK_IO_GPIO_Port GPIOE
#define DATA_IO_Pin GPIO_PIN_5
#define DATA_IO_GPIO_Port GPIOE
#define EEPROM_HOLD_Pin GPIO_PIN_6
#define EEPROM_HOLD_GPIO_Port GPIOE
#define DET_BAT_Pin GPIO_PIN_13
#define DET_BAT_GPIO_Port GPIOC
#define AD_57KHz_Pin GPIO_PIN_0
#define AD_57KHz_GPIO_Port GPIOC
#define AD_MPX_Pin GPIO_PIN_0
#define AD_MPX_GPIO_Port GPIOA
#define ADC_MONO_Pin GPIO_PIN_3
#define ADC_MONO_GPIO_Port GPIOA
#define AD_RSSI_S_Pin GPIO_PIN_4
#define AD_RSSI_S_GPIO_Port GPIOA
#define AD_RSSI_L_Pin GPIO_PIN_5
#define AD_RSSI_L_GPIO_Port GPIOA
#define AD_19KHz_Pin GPIO_PIN_6
#define AD_19KHz_GPIO_Port GPIOA
#define AD_LEFT_Pin GPIO_PIN_0
#define AD_LEFT_GPIO_Port GPIOB
#define AD_RIGHT_Pin GPIO_PIN_1
#define AD_RIGHT_GPIO_Port GPIOB
#define FMC_D4_Pin GPIO_PIN_7
#define FMC_D4_GPIO_Port GPIOE
#define KEY_ESC_Pin GPIO_PIN_11
#define KEY_ESC_GPIO_Port GPIOE
#define KEY_DOWN_Pin GPIO_PIN_12
#define KEY_DOWN_GPIO_Port GPIOE
#define KEY_UP_Pin GPIO_PIN_13
#define KEY_UP_GPIO_Port GPIOE
#define KEY_ENTER_Pin GPIO_PIN_14
#define KEY_ENTER_GPIO_Port GPIOE
#define BUZZER_Pin GPIO_PIN_15
#define BUZZER_GPIO_Port GPIOE
#define LED_FAIL_Pin GPIO_PIN_14
#define LED_FAIL_GPIO_Port GPIOB
#define LED_RSSI_Pin GPIO_PIN_15
#define LED_RSSI_GPIO_Port GPIOB
#define RDCL_Pin GPIO_PIN_8
#define RDCL_GPIO_Port GPIOD
#define RDDA_Pin GPIO_PIN_9
#define RDDA_GPIO_Port GPIOD
#define GPIO_USB_Pin GPIO_PIN_10
#define GPIO_USB_GPIO_Port GPIOD
#define LED_LOCK_Pin GPIO_PIN_6
#define LED_LOCK_GPIO_Port GPIOC
#define PLL_LOCK_Pin GPIO_PIN_7
#define PLL_LOCK_GPIO_Port GPIOC
#define EN_MB1501_Pin GPIO_PIN_8
#define EN_MB1501_GPIO_Port GPIOC
#define DSP_2_Pin GPIO_PIN_10
#define DSP_2_GPIO_Port GPIOA
#define DSP_1_Pin GPIO_PIN_15
#define DSP_1_GPIO_Port GPIOA
#define EN_ATTN_RF_Pin GPIO_PIN_10
#define EN_ATTN_RF_GPIO_Port GPIOC
#define BW_SEL_Pin GPIO_PIN_11
#define BW_SEL_GPIO_Port GPIOC
#define AUDIO_MUTE_Pin GPIO_PIN_12
#define AUDIO_MUTE_GPIO_Port GPIOC
#define AES192_Pin GPIO_PIN_2
#define AES192_GPIO_Port GPIOD
#define TFT_RD_Pin GPIO_PIN_4
#define TFT_RD_GPIO_Port GPIOD
#define TFT_EN_Pin GPIO_PIN_5
#define TFT_EN_GPIO_Port GPIOD
#define SEL_75_50_Pin GPIO_PIN_6
#define SEL_75_50_GPIO_Port GPIOD
#define TFT_CS_Pin GPIO_PIN_7
#define TFT_CS_GPIO_Port GPIOD
#define ST_MO_Pin GPIO_PIN_3
#define ST_MO_GPIO_Port GPIOB
#define DSP__MONO_Pin GPIO_PIN_5
#define DSP__MONO_GPIO_Port GPIOB
#define DSP_AUDIO_Pin GPIO_PIN_7
#define DSP_AUDIO_GPIO_Port GPIOB
#define FM_DEM_Pin GPIO_PIN_8
#define FM_DEM_GPIO_Port GPIOB
#define RELAY_1_Pin GPIO_PIN_0
#define RELAY_1_GPIO_Port GPIOE
#define RELAY_2_Pin GPIO_PIN_1
#define RELAY_2_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */
#define ADC_VREF		((float)2.50)
#define ADC_SENSE  		(float)(ADC_VREF/4095.0)

#define MAX_PWM     	4095		// Resolucao PWM 12 Bits

typedef enum reset_cause_e
{
    RESET_CAUSE_UNKNOWN = 0,
    RESET_CAUSE_LOW_POWER_RESET,
    RESET_CAUSE_WINDOW_WATCHDOG_RESET,
    RESET_CAUSE_INDEPENDENT_WATCHDOG_RESET,
    RESET_CAUSE_SOFTWARE_RESET,
    RESET_CAUSE_POWER_ON_POWER_DOWN_RESET,
    RESET_CAUSE_EXTERNAL_RESET_PIN_RESET,
    RESET_CAUSE_BROWNOUT_RESET,
} reset_cause_t;

typedef struct {
	uint32_t cnt;
	uint32_t cnt2;
	uint32_t tmr;
	uint32_t tmr_h;
	uint32_t tmr_l;
	uint32_t tmr2;
	uint32_t tmr2_h;
	uint32_t tmr2_l;
	uint32_t delay;
	uint32_t state;
} buzzer_var;

typedef struct {
	char ps[10];			// Station Name 8 Caracteres
	char tps[10];			// Trafic PS
	char ptyn[10];			// Program Type Name 8 Caracteres
	char pi[4];				// Program Identification
	char af[5];				// Alternative frequency
	char dps1[72];			// Dynamic Text 64 Caracteres
	char rt1[72];			// Text 1  64 Caracteres
	uint8_t ms;				// Music / Speech
	uint8_t pty;			// Program Type Number
	uint8_t tp;				// Traffic Program
	uint8_t type;			// Tipo RDS(EU) or RBDS
	uint8_t enable;			// Enable / Disable RDS
	uint8_t ct;				// Enable / Disable DateTime
	char version[20];
	uint8_t enable_station;
	uint8_t enable_text;
} rds_var;

typedef struct {
	// RF
	uint8_t		modo;
	// Audio
	uint8_t		MonoStereo;
	uint8_t		Processor;
	uint8_t		Emphase;
	uint8_t		AES192;
	uint8_t		AudioSource;
	uint8_t		Vol_MPX1;
	uint8_t		Vol_MPX2;
	uint8_t		switch_mod;
	uint32_t	timer_audio_on;
	uint32_t	timer_audio_off;
	uint8_t		level_audio_on;
	uint8_t 	level_audio_off;
	uint32_t    Frequencia;
	// Network
	uint8_t		EnableSNMP;
	uint16_t	PortWEB;
	uint8_t		IP_ADDR[4];
	uint8_t		MASK_ADDR[4];
	uint8_t		GW_ADDR[4];
	uint8_t		DNS_ADDR[4];
	// RDS Remote
	uint8_t		RDSRemote;
	uint16_t	RDSUDPPort;
	// Timezone
	uint8_t		NTP;
	uint8_t		Timezone;
	// Password
	uint8_t		PassAdmin[4];
	uint8_t		PassUser[4];
	// VSWR NULL
	uint8_t		ConfigHold;
	uint8_t		VSWR_Null;
	uint8_t 	Config_in_hold;
	float		VSWR_Null_Value;
	float		FWD_Null_Value;
	// Atenuacao
	float		Attenuation;
	// Serial Number
	char		SerialNumber[10];
	char		License[16];
	char		Token[16];
	// Servico
	uint8_t		servico;
	uint8_t 	model_type;
	uint16_t 	EepromID;
	uint16_t 	prg_rev;
	// UUID
    uint8_t		uuid[16];
} Cfg_var;

typedef struct {
	// Profile
	char Station[50];
	char City[50];
	char State[50];
	char Country[50];
	char Temp[50];
} Profile_var;

typedef struct {
	uint32_t LicSeq;
	uint32_t licenseTimer;
	uint32_t myseq;
	uint32_t mydays;
	uint32_t verify;
	uint32_t myverify;
	uint8_t ln1;
	uint8_t ln2;
	uint8_t ln3;
	char MyLic[10];
} license_var;

typedef struct {
	volatile float MaxIpa;
	volatile float MaxVswr;
	volatile float MaxTemp;
	volatile float GainFWD;
	volatile float GainSWR;
	volatile float GainIPA;
	volatile float GainVPA;
	volatile float GainTemp;
} AdvancedSettings;

typedef struct {
	uint32_t dia;
	uint8_t second;
	uint8_t minute;
	uint8_t hour;
	uint32_t total;
} SYS_UPTime;

typedef struct {
	uint8_t second;
	uint8_t minute;
	uint8_t hour;
	uint8_t day;
	uint8_t weekday;
	uint8_t month;
	uint16_t year;
	uint32_t total;
} SYS_DateTime;

typedef struct {
	float Forward;
	float Reflected;
	float VPA;
	float IPA;
	float Temperature;
	float SWR;
	float Load_MisMatch;
	float Return_Loss;
	uint16_t pwm_bias;
	uint16_t pwm_fan;
} SYS_Realtime;

enum LEDS_STATE {
	LED_OFF = 0,
	LED_ON
};

enum PWM_CHANNELS {
	PWM_FAN = 0,
	PWM_RF
};

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
