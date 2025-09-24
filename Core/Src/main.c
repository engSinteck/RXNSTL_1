/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "adc.h"
#include "crc.h"
#include "dma.h"
#include "fatfs.h"
#include "i2c.h"
#include "i2s.h"
#include "iwdg.h"
#include "quadspi.h"
#include "rng.h"
#include "rtc.h"
#include "tim.h"
#include "usb_host.h"
#include "gpio.h"
#include "fmc.h"
#include "mbedtls.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <sys/time.h>
#include "../Sinteck/display/Drv_SSD1963.h"
#include "../Sinteck/src/keys.h"
#include "../Sinteck/src/buzzer.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
reset_cause_t reset_cause_get(void);
void Read_Remote(void);
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
#define DEBOUNCE_REMOTE		500
extern volatile unsigned long ulHighFrequencyTimerTicks;

extern volatile unsigned long MilliTimer;
uint32_t timer_lvgl = 0;
uint32_t timer_key = 0;
GPIO_PinState BatteryInput;
GPIO_PinState StereoMonoInput;
GPIO_PinState FMDemInput;

uint32_t deb_battery_l = 0;
uint32_t deb_battery_h = 0;
uint8_t Status_Battery = 0;

uint32_t deb_fmdem_l = 0;
uint32_t deb_fmdem_h = 0;
uint8_t Status_FMDem = 0;

uint32_t deb_stmo_l = 0;
uint32_t deb_stmo_h = 0;
uint8_t Status_Stereo = 0;

reset_cause_t reset_cause;

uint32_t TelaAtiva = 0;
uint32_t filter_adc = 0;

/* Variable containing ADC conversions data */
//ALIGN_32BYTES (static uint16_t   adcBuffer[8]);
#define ADC_CONVERTED_DATA_BUFFER_SIZE   ((uint32_t)  8)   /* Size of array aADCxConvertedData[] */
ALIGN_32BYTES (static uint16_t   adcBuffer[ADC_CONVERTED_DATA_BUFFER_SIZE]);
uint16_t adcH7[] = {0};
uint16_t adc_ext[11] = {0};

// Vari�veis para valores convertidos
volatile uint16_t adc_values[8];
volatile uint8_t adc_ready = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MPU_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* Enable the CPU Cache */

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_FMC_Init();
  MX_I2C1_Init();
  MX_QUADSPI_Init();
  MX_RTC_Init();
  MX_TIM1_Init();
  MX_TIM12_Init();
  MX_IWDG1_Init();
  MX_CRC_Init();
  MX_RNG_Init();
  MX_ADC1_Init();
  MX_I2S2_Init();
  /* Call PreOsInit function */
  //MX_MBEDTLS_Init();
  /* USER CODE BEGIN 2 */
  __HAL_RCC_D2SRAM1_CLK_ENABLE();
  __HAL_RCC_D2SRAM2_CLK_ENABLE();
  __HAL_RCC_D2SRAM3_CLK_ENABLE();
  __HAL_RCC_BKPRAM_CLKAM_ENABLE();

  // LEDS
  HAL_GPIO_WritePin(LED_FAIL_GPIO_Port, LED_FAIL_Pin, LED_OFF);		// LED FAIL OFF
  HAL_GPIO_WritePin(LED_LOCK_GPIO_Port, LED_LOCK_Pin, LED_OFF);		// LED LOCK OFF
  HAL_GPIO_WritePin(LED_RSSI_GPIO_Port, LED_RSSI_Pin, LED_OFF);		// LED RF OFF

  HAL_TIM_Base_Start_IT(&htim12);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_1, 0);		// PWM_CH1 = 0      RF
  __HAL_DBGMCU_FREEZE_IWDG1();

  // Calibrate RTC
 // HAL_RTCEx_SetSmoothCalib(&hrtc, RTC_SMOOTHCALIB_PERIOD_32SEC, 1, 500);

  // Config ADC
  adc_ext[0] = 0;
  adc_ext[1] = 0;
  adc_ext[2] = 0;
  adc_ext[3] = 0;
  adc_ext[4] = 0;
  adc_ext[5] = 0;
  adc_ext[6] = 0;
  adc_ext[7] = 0;
  adc_ext[8] = 0;
  adc_ext[9] = 0;

  // Start ADC Calibration
  HAL_ADC_Stop(&hadc1);
  if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED) != HAL_OK)
  {
      // A calibra��o falhou, trate o erro.
      Error_Handler();
  }

  if(HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET_LINEARITY, ADC_SINGLE_ENDED) != HAL_OK) {
	  Error_Handler();
  }
  // Start ADC in DMA
  if(HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adcBuffer, 8) != HAL_OK) {
	  Error_Handler();
  }

  // Causa Reset
  reset_cause = reset_cause_get();

  // Inicia Buzzer Vars
  Buzzer_init();

  // Init SSD1963
  drv_ssd1963_init();
  drv_ssd1963_SetBacklight(200);
  // Tela de Start-UP
  drv_ssd1963_bmp();
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();  /* Call init function for freertos objects (in cmsis_os2.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_HIGH);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE
                              |RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 480;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 8;
  RCC_OscInitStruct.PLL.PLLR = 8;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_1;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_FMC|RCC_PERIPHCLK_QSPI
                              |RCC_PERIPHCLK_USB|RCC_PERIPHCLK_ADC;
  PeriphClkInitStruct.PLL2.PLL2M = 4;
  PeriphClkInitStruct.PLL2.PLL2N = 480;
  PeriphClkInitStruct.PLL2.PLL2P = 2;
  PeriphClkInitStruct.PLL2.PLL2Q = 2;
  PeriphClkInitStruct.PLL2.PLL2R = 8;
  PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_1;
  PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
  PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
  PeriphClkInitStruct.PLL3.PLL3M = 4;
  PeriphClkInitStruct.PLL3.PLL3N = 480;
  PeriphClkInitStruct.PLL3.PLL3P = 2;
  PeriphClkInitStruct.PLL3.PLL3Q = 20;
  PeriphClkInitStruct.PLL3.PLL3R = 10;
  PeriphClkInitStruct.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_1;
  PeriphClkInitStruct.PLL3.PLL3VCOSEL = RCC_PLL3VCOWIDE;
  PeriphClkInitStruct.PLL3.PLL3FRACN = 0;
  PeriphClkInitStruct.FmcClockSelection = RCC_FMCCLKSOURCE_PLL2;
  PeriphClkInitStruct.QspiClockSelection = RCC_QSPICLKSOURCE_PLL2;
  PeriphClkInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_PLL3;
  PeriphClkInitStruct.AdcClockSelection = RCC_ADCCLKSOURCE_PLL3;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
int _gettimeofday( struct timeval *tv, void *tzvp )
{
    // you can add code here there many example in google search.
    return 0;  // return non-zero for error
} // end _gettimeofday()

reset_cause_t reset_cause_get(void)
{
    reset_cause_t reset_cause;

//    if (__HAL_RCC_GET_FLAG(RCC_FLAG_LPWRRST))
//    {
//        reset_cause = RESET_CAUSE_LOW_POWER_RESET;
//    }
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDG1RST))
    {
        reset_cause = RESET_CAUSE_WINDOW_WATCHDOG_RESET;
    }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDG1RST))
    {
        reset_cause = RESET_CAUSE_INDEPENDENT_WATCHDOG_RESET;
    }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST))
    {
        reset_cause = RESET_CAUSE_SOFTWARE_RESET; // This reset is induced by calling the ARM CMSIS `NVIC_SystemReset()` function!
    }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST))
    {
        reset_cause = RESET_CAUSE_POWER_ON_POWER_DOWN_RESET;
    }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST))
    {
        reset_cause = RESET_CAUSE_EXTERNAL_RESET_PIN_RESET;
    }
    // Needs to come *after* checking the `RCC_FLAG_PORRST` flag in order to ensure first that the reset cause is
    // NOT a POR/PDR reset. See note below.
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_BORRST))
    {
        reset_cause = RESET_CAUSE_BROWNOUT_RESET;
    }
    else
    {
        reset_cause = RESET_CAUSE_UNKNOWN;
    }

    // Clear all the reset flags or else they will remain set during future resets until system power is fully removed.
    __HAL_RCC_CLEAR_RESET_FLAGS();

    return reset_cause;
}

#ifdef __GNUC__
  /* With GCC, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
//  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
//PUTCHAR_PROTOTYPE
//{
//  /* Place your implementation of fputc here */
//  /* e.g. write a character to the EVAL_COM1 and Loop until the end of transmission */
//  //HAL_UART_Transmit(&huart3, (uint8_t *)&ch, 1, 0xFFFF);
//  CDC_Transmit_FS((uint8_t*)&ch, 1);
//
//  return ch;
//}
int _write(int fd, char * ptr, int len)
{
  //CDC_Transmit_FS((uint8_t *) ptr, len);
  return len;
}

void Read_Remote(void)
{
	// Debounce Battery Input
	BatteryInput = HAL_GPIO_ReadPin(DET_BAT_GPIO_Port, DET_BAT_Pin);
	if(BatteryInput == GPIO_PIN_RESET) {
		deb_battery_l++;
		deb_battery_h = 0;
		if(deb_battery_l >= DEBOUNCE_REMOTE) {
			deb_battery_l = DEBOUNCE_REMOTE + 1;
			Status_Battery = 0;
		}
	}
	else {
		deb_battery_h++;
		deb_battery_l = 0;
		if(deb_battery_h >= DEBOUNCE_REMOTE) {
			deb_battery_h = DEBOUNCE_REMOTE + 1;
			Status_Battery = 1;
		}
	}
	// FM Demulador
	FMDemInput = HAL_GPIO_ReadPin(FM_DEM_GPIO_Port, FM_DEM_Pin);
	if(FMDemInput == GPIO_PIN_RESET) {
		deb_fmdem_l++;
		deb_fmdem_h = 0;
		if(deb_fmdem_l >= DEBOUNCE_REMOTE) {
			deb_fmdem_l = DEBOUNCE_REMOTE + 1;
			Status_FMDem = 0;
		}
	}
	else {
		deb_fmdem_h++;
		deb_fmdem_l = 0;
		if(deb_fmdem_h >= DEBOUNCE_REMOTE) {
			deb_fmdem_h = DEBOUNCE_REMOTE + 1;
			Status_FMDem = 1;
		}
	}
	// Stereo / Mono
	StereoMonoInput = HAL_GPIO_ReadPin(ST_MO_GPIO_Port, ST_MO_Pin);
	if(StereoMonoInput == GPIO_PIN_RESET) {
		deb_stmo_l++;
		deb_stmo_h = 0;
		if(deb_stmo_l >= DEBOUNCE_REMOTE) {
			deb_stmo_l = DEBOUNCE_REMOTE + 1;
			Status_Stereo = 0;
		}
	}
	else {
		deb_stmo_h++;
		deb_stmo_l = 0;
		if(deb_stmo_h >= DEBOUNCE_REMOTE) {
			deb_stmo_h = DEBOUNCE_REMOTE + 1;
			Status_Stereo = 1;
		}
	}
}

/**
  * @brief  Conversion complete callback in non-blocking mode
  * @param  hadc: ADC handle
  * @retval None
  */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
  /* Invalidate Data Cache to get the updated content of the SRAM on the first half of the ADC converted data buffer: 32 bytes */
//  SCB_InvalidateDCache_by_Addr((uint32_t *)adcBuffer, sizeof(adcBuffer) / 2);
}

/**
  * @brief  Conversion DMA half-transfer callback in non-blocking mode
  * @param  hadc: ADC handle
  * @retval None
  */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
   /* Invalidate Data Cache to get the updated content of the SRAM on the second half of the ADC converted data buffer: 32 bytes */
	  // Copia os valores do buffer para o array principal
	  for (int i = 0; i < 8; i++) {
	    adc_values[i] = adcBuffer[i];
	  }
	  adc_ready = 1;
	filter_adc++;
}

// Callback de erro
void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc)
{
  // Tratamento de erro - pode reiniciar a convers�o
  HAL_ADC_Stop_DMA(&hadc1);
  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adcBuffer, 8);
}
/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.BaseAddress = 0x30020000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_128KB;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER2;
  MPU_InitStruct.BaseAddress = 0x30040000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_512B;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER3;
  MPU_InitStruct.BaseAddress = 0x60000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4KB;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER4;
  MPU_InitStruct.BaseAddress = 0x60080000;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */
  if (htim->Instance == TIM12) {
	  ulHighFrequencyTimerTicks++;
	  // Controle Buzzer
	  Buzzer_Run();
  }
  
  if (htim->Instance == TIM6) {
	  MilliTimer++;

	  timer_lvgl++;
	  if(timer_lvgl >= 10) {
		  timer_lvgl = 0;
		  lv_tick_inc(10);
	  }

	  timer_key++;
	  if(timer_key >= PUSHBTN_TMR_PERIOD) {
		  timer_key = 0;
		  Key_Read();
	  }
  }
  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
