/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <time.h>
#include "iwdg.h"
#include "queue.h"
#include "semphr.h"
#include "stdio.h"
#include "string.h"
#include "tim.h"
#include "rtc.h"
#include "fatfs.h"
#include "mbedtls.h"
#include "lwip/apps/lwiperf.h"
#include "lwip/apps/httpd.h"
#include "lwip/apps/snmp.h"
#include "lwip/apps/snmpv3.h"
#include "lwip/apps/sntp.h"
//#include "lwip/netconn.h"

#include "ethernetif.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"

#include "../Sinteck/src/uuid.h"
#include "../Sinteck/display/Drv_SSD1963.h"
#include "../Sinteck/src/stm32_qspi_512.h"
#include "../Sinteck/src/SAA6579.h"
#include "../Sinteck/lvgl/lvgl.h"
#include "../Sinteck/lv_fs_if/lv_fs_if.h"
#include "../Sinteck/tcp/httpserver_netconn.h"
#include "../Sinteck/tcp/mqtt_paho.h"
#include "../Sinteck/tcp/snmp_app.h"
#include "../Sinteck/tcp/mqtt_client.h"

#include "../Sinteck/src/audio.h"
#include "../Sinteck/src/keys.h"
#include "../Sinteck/src/buzzer.h"
#include "../Sinteck/src/eeprom.h"
#include "../Sinteck/src/defines.h"

#include <GUI/screen_main_TX.h>
#include <GUI/screen_main_RX.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
extern RTC_DateTypeDef gDateAdj;
extern RTC_TimeTypeDef gTimeAdj;

#define NTP_SERVER "pool.ntp.org"
//#define NTP_SERVER "a.st1.ntp.br"
#define NTP_PORT 123
#define NTP_TIMEOUT 5000
#define NTP_RETRY_DELAY 3000

const int fuso_offset[25] = { -12, -11, -10, -9, -8, -7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };

// Estrutura do pacote NTP (RFC 2030)
typedef struct {
    uint8_t li_vn_mode;
    uint8_t stratum;
    uint8_t poll;
    uint8_t precision;
    uint32_t root_delay;
    uint32_t root_dispersion;
    uint32_t reference_id;
    uint32_t ref_ts_sec;
    uint32_t ref_ts_frac;
    uint32_t orig_ts_sec;
    uint32_t orig_ts_frac;
    uint32_t recv_ts_sec;
    uint32_t recv_ts_frac;
    uint32_t trans_ts_sec;
    uint32_t trans_ts_frac;
} ntp_packet_t;

#define NTP_OFFSET 2208988800U  // Segundos entre 1900 e 1970
char str_ntp[256] = {0};

extern volatile uint8_t rx_uart[];

void Read_Remote(void);
extern void MX_LWIP_Init(void);
extern void MX_USB_DEVICE_Init(void);
void Mount_FATFS(void);
void my_loglvgl(lv_log_level_t level, const char *file, uint32_t line, const char *dsc);

volatile unsigned long ulHighFrequencyTimerTicks = 0;

static lv_disp_buf_t disp_buf __attribute__((section(".tftram")));
static lv_color_t buf_tft[LV_HOR_RES_MAX * 10] __attribute__((section(".tftram")));		// Declare a buffer for 10 lines
lv_mem_monitor_t mon;

FATFS *pfs;
char line[100]; /* Line buffer */
FRESULT fr;     /* FatFs return code */
DWORD fre_clust;
uint32_t totalSpace, freeSpace, SpaceUsed;

// FLASH QSPI W25Q512
extern uint8_t retUSER;    /* Return value for USER */
extern FATFS USERFatFS;    /* File system object for USER  cal drive */
extern FIL USERFile;       /* File object for USER */

// USB HOST
extern uint8_t retUSBH;    /* Return value for USBH */
extern FATFS USBHFatFS;    /* File system object for USBH logical drive */
extern FIL USBHFile;       /* File object for USBH */

lv_mem_monitor_t mon;

uint8_t qspi_ret;
uint32_t duracao = 0;
uint32_t size;
unsigned int ByteRead;
char version_flash[32] = {0};
uint32_t timer_alarm_lvgl = 0;
uint32_t timer_read_rtc = 0;

RTC_DateTypeDef gDate;
RTC_TimeTypeDef gTime;
SYS_DateTime TimeInicio;
extern SYS_Realtime Realtime;

uint8_t Telemetry_State = 0;
uint32_t timer_teste = 0;
uint32_t rds_report = 0;

extern Cfg_var cfg;
extern license_var lic;
extern Profile_var Profile;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for TaskMain */
osThreadId_t TaskMainHandle;
const osThreadAttr_t TaskMain_attributes = {
  .name = "TaskMain",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityNormal2,
};
/* Definitions for TaskGUI */
osThreadId_t TaskGUIHandle;
const osThreadAttr_t TaskGUI_attributes = {
  .name = "TaskGUI",
  .stack_size = 4096 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for TaskNTP */
osThreadId_t TaskNTPHandle;
const osThreadAttr_t TaskNTP_attributes = {
  .name = "TaskNTP",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for QueueUSART3 */
osMessageQueueId_t QueueUSART3Handle;
const osMessageQueueAttr_t QueueUSART3_attributes = {
  .name = "QueueUSART3"
};
/* Definitions for QueueUSART6 */
osMessageQueueId_t QueueUSART6Handle;
const osMessageQueueAttr_t QueueUSART6_attributes = {
  .name = "QueueUSART6"
};
/* Definitions for QueueBuzzer */
osMessageQueueId_t QueueBuzzerHandle;
const osMessageQueueAttr_t QueueBuzzer_attributes = {
  .name = "QueueBuzzer"
};
/* Definitions for QueueHttp */
osMessageQueueId_t QueueHttpHandle;
const osMessageQueueAttr_t QueueHttp_attributes = {
  .name = "QueueHttp"
};
/* Definitions for QueueMqtt */
osMessageQueueId_t QueueMqttHandle;
const osMessageQueueAttr_t QueueMqtt_attributes = {
  .name = "QueueMqtt"
};
/* Definitions for MutexI2C */
osMutexId_t MutexI2CHandle;
const osMutexAttr_t MutexI2C_attributes = {
  .name = "MutexI2C"
};
/* Definitions for MutexLVGL */
osMutexId_t MutexLVGLHandle;
const osMutexAttr_t MutexLVGL_attributes = {
  .name = "MutexLVGL"
};
/* Definitions for MutexHTTPD */
osMutexId_t MutexHTTPDHandle;
const osMutexAttr_t MutexHTTPD_attributes = {
  .name = "MutexHTTPD"
};
/* Definitions for MutexMQTT */
osMutexId_t MutexMqtt;
const osMutexAttr_t MutexMqtt_attributes = {
  .name = "MutexMqtt"
};

/* Definitions for BinarySemI2C */
osSemaphoreId_t BinarySemI2CHandle;
const osSemaphoreAttr_t BinarySemI2C_attributes = {
  .name = "BinarySemI2C"
};
/* Definitions for BinarySemLVGL */
osSemaphoreId_t BinarySemLVGLHandle;
const osSemaphoreAttr_t BinarySemLVGL_attributes = {
  .name = "BinarySemLVGL"
};
/* Definitions for BinarySemHTTPD */
osSemaphoreId_t BinarySemHTTPDHandle;
const osSemaphoreAttr_t BinarySemHTTPD_attributes = {
  .name = "BinarySemHTTPD"
};
/* Definitions for BinarySemMQTT */
osSemaphoreId_t BinarySemMQTTHandle;
const osSemaphoreAttr_t BinarySemMQTT_attributes = {
  .name = "BinarySemMQTT"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartTaskMain(void *argument);
void StartTaskGUI(void *argument);
void StartTaskNTP(void *argument);

extern void MX_LWIP_Init(void);
extern void MX_USB_HOST_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void configureTimerForRunTimeStats(void);
unsigned long getRunTimeCounterValue(void);
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName);

/* USER CODE BEGIN 1 */
/* Functions needed when configGENERATE_RUN_TIME_STATS is on */
__weak void configureTimerForRunTimeStats(void)
{
	ulHighFrequencyTimerTicks = 0;
}

__weak unsigned long getRunTimeCounterValue(void)
{
	return ulHighFrequencyTimerTicks;
}
/* USER CODE END 1 */

/* USER CODE BEGIN 4 */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
   /* Run time stack overflow checking is performed if
   configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
   called if a stack overflow is detected. */
}
/* USER CODE END 4 */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* creation of MutexI2C */
  MutexI2CHandle = osMutexNew(&MutexI2C_attributes);

  /* creation of MutexLVGL */
  MutexLVGLHandle = osMutexNew(&MutexLVGL_attributes);

  /* creation of MutexHTTPD */
  MutexHTTPDHandle = osMutexNew(&MutexHTTPD_attributes);

  /* creation of MutexMqtt */
  MutexMqtt = osMutexNew(&MutexMqtt_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of BinarySemI2C */
  BinarySemI2CHandle = osSemaphoreNew(1, 0, &BinarySemI2C_attributes);

  /* creation of BinarySemLVGL */
  BinarySemLVGLHandle = osSemaphoreNew(1, 0, &BinarySemLVGL_attributes);

  /* creation of BinarySemHTTPD */
  BinarySemHTTPDHandle = osSemaphoreNew(1, 0, &BinarySemHTTPD_attributes);

  /* creation of BinarySemMQTT */
  BinarySemMQTTHandle = osSemaphoreNew(1, 1, &BinarySemMQTT_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of QueueUSART3 */
  QueueUSART3Handle = osMessageQueueNew (16, sizeof(uint8_t), &QueueUSART3_attributes);

  /* creation of QueueUSART6 */
  QueueUSART6Handle = osMessageQueueNew (16, sizeof(uint8_t), &QueueUSART6_attributes);

  /* creation of QueueBuzzer */
  QueueBuzzerHandle = osMessageQueueNew (1, sizeof(uint32_t), &QueueBuzzer_attributes);

  /* creation of QueueHttp */
  QueueHttpHandle = osMessageQueueNew (1, sizeof(uint32_t), &QueueHttp_attributes);

  /* creation of QueueMqtt */
  QueueMqttHandle = osMessageQueueNew (50, 10, &QueueMqtt_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of TaskMain */
  TaskMainHandle = osThreadNew(StartTaskMain, NULL, &TaskMain_attributes);

  /* creation of TaskGUI */
  TaskGUIHandle = osThreadNew(StartTaskGUI, NULL, &TaskGUI_attributes);

  /* creation of TaskNTP */
  TaskNTPHandle = osThreadNew(StartTaskNTP, NULL, &TaskNTP_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartTaskMain */
/**
  * @brief  Function implementing the TaskMain thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartTaskMain */
void StartTaskMain(void *argument)
{
  /* init code for USB_HOST */
  MX_USB_HOST_Init();
  /* USER CODE BEGIN StartTaskMain */

  // Grava Hora de Inicio
  // Get the RTC current Time
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_BKPRAM_CLK_ENABLE();
  HAL_RTC_GetTime(&hrtc, &gTime, RTC_FORMAT_BIN);
  // Get the RTC current Date */
  HAL_RTC_GetDate(&hrtc, &gDate, RTC_FORMAT_BIN);
  __HAL_RCC_BKPRAM_CLK_DISABLE();
  HAL_PWR_DisableBkUpAccess();

  TimeInicio.day = gDate.Date;
  TimeInicio.month = gDate.Month;
  TimeInicio.year = 2000+gDate.Year;
  TimeInicio.hour = gTime.Hours;
  TimeInicio.minute = gTime.Minutes;
  TimeInicio.second = gTime.Seconds;
  TimeInicio.total = (gTime.Hours * 3600) + (gTime.Minutes * 60) + gTime.Seconds;

  // Carrega Programacao da EEPROM
  cfg.EepromID = 0xFFFF;
  cfg.prg_rev  = 0xFFFF;
  lic.LicSeq = 0;
  cfg.License[0] = 0xFF; cfg.License[1] = 0xFF; cfg.License[2] = 0xFF; cfg.License[3] = 0xFF;
  cfg.License[4] = 0xFF; cfg.License[5] = 0xFF; cfg.License[6] = 0xFF; cfg.License[7] = 0xFF;
  cfg.License[8] = 0;
  lic.licenseTimer = 999;

  // Profile
  memset(Profile.Station, 0, 50);
  memset(Profile.City, 0, 50);
  memset(Profile.State, 0, 50);
  memset(Profile.Country, 0, 50);

  ReadIdEeprom();
  if(cfg.EepromID != 0x55AA || cfg.prg_rev != PRG_REVISION || cfg.model_type != MODELO) {
	  Carrega_Prog_Default();
	  Read_Enviroment();
  }
  else {
	  Read_Profile();
	  ReadConfig();
	  UpdateValores();
  }

  Realtime.Forward = 5.0f;
  Realtime.Reflected = 1.2f;
  Realtime.Temperature = 32.4f;
  Realtime.SWR = 0.0f;
  Realtime.Load_MisMatch = 0.0f;
  Realtime.Return_Loss = 0.0f;
  Realtime.VPA = 20.4f;
  Realtime.IPA = 3.4f;

  Get_UUID();

  // Start Ethernet Stack
  MX_LWIP_Init();
  MX_MBEDTLS_Init();

  // init code HTTPD
  http_server_netconn_init();

  // Se Nao Tem Numero de Serie Token 0000-0000
  if(cfg.SerialNumber[0] == 0xFF && cfg.SerialNumber[1] == 0xFF && cfg.SerialNumber[2] == 0xFF && cfg.SerialNumber[3] == 0xFF &&
     cfg.SerialNumber[4] == 0xFF && cfg.SerialNumber[5] == 0xFF && cfg.SerialNumber[6] == 0xFF && cfg.SerialNumber[7] == 0xFF     ) {
	 //
		cfg.Token[0] = '0'; cfg.Token[1] = '0'; cfg.Token[2] = '0'; cfg.Token[3] = '0';
		cfg.Token[4] = '0'; cfg.Token[5] = '0'; cfg.Token[6] = '0'; cfg.Token[7] = '0';
		cfg.Token[8] = 0;
		cfg.Token[12] = 1;
	}
	else {
		// Token = SN
		cfg.Token[0] = cfg.SerialNumber[0]+'0'; cfg.Token[1] = cfg.SerialNumber[1]+'0'; cfg.Token[2] = cfg.SerialNumber[2]+'0'; cfg.Token[3] = cfg.SerialNumber[3]+'0';
		cfg.Token[4] = cfg.SerialNumber[4]+'0'; cfg.Token[5] = cfg.SerialNumber[5]+'0'; cfg.Token[6] = cfg.SerialNumber[6]+'0'; cfg.Token[7] = cfg.SerialNumber[7]+'0';
		cfg.Token[8] = 0;
		cfg.Token[12] = 1;
	}
  mqtt_init();

  /* init code SNMP */
  if( cfg.EnableSNMP != 0 ) {
  	initialize_snmp();
  }

  /* Infinite loop */
  for(;;)
  {
	  if(HAL_GetTick() - timer_read_rtc > 1000) {
		  timer_read_rtc = HAL_GetTick();
		  // Get the RTC current Time
		  HAL_PWR_EnableBkUpAccess();
		  __HAL_RCC_BKPRAM_CLK_ENABLE();
		  HAL_RTC_GetTime(&hrtc, &gTime, RTC_FORMAT_BIN);
		  // Get the RTC current Date */
		  HAL_RTC_GetDate(&hrtc, &gDate, RTC_FORMAT_BIN);
		  __HAL_RCC_BKPRAM_CLK_DISABLE();
		  HAL_PWR_DisableBkUpAccess();

		  // Calcule UpTime
		  Calculate_UPTime();
	  }

	  // Leitura GPIO BattInput
	  Read_Remote();

	  // RDS
	  RDS_Task();
	  if (HAL_GetTick() - rds_report > 5000) {
		  RDS_PrintStats();
		  rds_report = HAL_GetTick();
	  }

	  // Save Telemetry Data
	  Telemetry_save();

	  // Protecao Watch-Dog
	  HAL_IWDG_Refresh(&hiwdg1);

	  osDelay(10);
  }
  /* USER CODE END StartTaskMain */
}

/* USER CODE BEGIN Header_StartTaskGUI */
/**
* @brief Function implementing the TaskGUI thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTaskGUI */
void StartTaskGUI(void *argument)
{
  /* USER CODE BEGIN StartTaskGUI */
  Evt_InitQueue();
  KeyboardInit(0x01);

	// Init Flash QSPI
  qspi_ret = BSP_QSPI_Init();
  // Mount Flash FATFS
  MX_FATFS_Init();
  Mount_FATFS();

  lv_disp_buf_init(&disp_buf, buf_tft, NULL, LV_HOR_RES_MAX * 10);    // Initialize the display buffer
  lv_init(0);

  // Lvgl File System
  lv_fs_if_init();

  // LVGL Setup
  lv_disp_drv_t disp_drv;               		// Descriptor of a display driver
  lv_disp_drv_init(&disp_drv);          		// Basic initialization
  // SSD1963
  disp_drv.hor_res = 480;               		// Set the horizontal resolution
  disp_drv.ver_res = 128;               		// Set the vertical resolution
  disp_drv.flush_cb = drv_ssd1963_flush_3;		// Set your driver function
  disp_drv.buffer = &disp_buf;          		// Assign the buffer to display
  lv_disp_drv_register(&disp_drv);      		// Finally register the driver

  // Add Log
#if LV_USE_LOG
  lv_log_register_print_cb(my_loglvgl);
#endif

  main_screen_RX();

  /* Infinite loop */
  for(;;)
  {
  		// Eventos da GUI LittleVGL
  		xSemaphoreTake(MutexLVGLHandle, portMAX_DELAY);
  		lv_task_handler();
 		xSemaphoreGive(MutexLVGLHandle);
  		// Eventos  Teclado
  		ButtonEvent();
  		// Dados Uso Memoria LVGL
 		lv_mem_monitor(&mon);

 		osDelay(5);
  }
  /* USER CODE END StartTaskGUI */
}

/* Private application code --------------------------------------------------*/
static void ntp_set_packet(ntp_packet_t *packet) {
    memset(packet, 0, sizeof(ntp_packet_t));

    // LI = 0 (sem aviso), VN = 4 (vers?o 4), Mode = 3 (cliente)
    packet->li_vn_mode = (0 << 6) | (4 << 3) | (3 << 0);

    // Timestamp de transmiss?o (tempo atual aproximado)
    uint32_t current_time = (uint32_t)time(NULL) + NTP_OFFSET;
    packet->trans_ts_sec = __REV(current_time);
}

static time_t ntp_get_time(void) {
    struct netconn *conn = NULL;
    err_t err;
    time_t ntp_time = 0;

    // Criar conex?o UDP
    conn = netconn_new(NETCONN_UDP);
    if (conn == NULL) {
        return 0;
    }

    // Configurar timeout
    netconn_set_recvtimeout(conn, NTP_TIMEOUT);

    // Resolver endere?o do servidor NTP
    ip_addr_t ntp_addr;
    err = netconn_gethostbyname(NTP_SERVER, &ntp_addr);
    if (err != ERR_OK) {
        netconn_close(conn);
        netconn_delete(conn);
        return 0;
    }

    // Conectar ao servidor NTP (opcional para UDP, mas ?til)
    err = netconn_connect(conn, &ntp_addr, NTP_PORT);
    if (err != ERR_OK) {
        netconn_close(conn);
        netconn_delete(conn);
        return 0;
    }

    // Preparar pacote NTP
    ntp_packet_t ntp_request;
    ntp_set_packet(&ntp_request);

    // Enviar requisi??o CORRETAMENTE
    struct netbuf *buf_send = netbuf_new();
    if (buf_send) {
        void *payload = netbuf_alloc(buf_send, sizeof(ntp_packet_t));
        if (payload) {
            memcpy(payload, &ntp_request, sizeof(ntp_packet_t));
            err = netconn_send(conn, buf_send);
            if (err != ERR_OK) {
            }
        }
        netbuf_delete(buf_send);
    }

    // Receber resposta
    struct netbuf *buf_recv;
    err = netconn_recv(conn, &buf_recv);
    if (err == ERR_OK) {
        void *data;
        u16_t len;

        netbuf_data(buf_recv, &data, &len);

        if (len >= sizeof(ntp_packet_t)) {
            ntp_packet_t *response = (ntp_packet_t *)data;

            // Converter timestamp de rede para host
            uint32_t ntp_seconds = __REV(response->trans_ts_sec);

            // Converter de tempo NTP (1900) para Unix (1970)
            ntp_time = (time_t)(ntp_seconds - NTP_OFFSET);

        } else {
        }

        netbuf_delete(buf_recv);
    } else {
    }

    // Fechar e deletar conex?o
    netconn_close(conn);
    netconn_delete(conn);

    return ntp_time;
}

void StartTaskNTP(void *argument)
{
	struct tm *info;

	 /* USER CODE BEGIN StartTaskNTP */
	for(;;)
	{
		time_t current_time = ntp_get_time();

		if (current_time > 0) {
			// Atualizar o rel?gio do sistema
			// (implemente sua fun??o de set_time aqui)
			info = localtime(&current_time);
			sprintf(str_ntp, "NTP: W: %d - D: %02d/%02d/%04d  T: %02d:%02d:%02d\n",
					info->tm_wday, info->tm_mday, info->tm_mon + 1, info->tm_year + 1900,
					info->tm_hour, info->tm_min, info->tm_sec );
			//
			if(cfg.NTP) {
				// Dia-Semana-Dia-Mes-Ano-HH:MM:SS
				gDateAdj.WeekDay = info->tm_wday;
				gDateAdj.Year = (uint8_t)((info->tm_year + 1900)-2000);
				gDateAdj.Month = info->tm_mon + 1;
				gDateAdj.Date = info->tm_mday;

				gTimeAdj.Hours   = info->tm_hour + fuso_offset[cfg.Timezone];
				gTimeAdj.Minutes = info->tm_min;
				gTimeAdj.Seconds = info->tm_sec;

				// Acerta Relogio
				HAL_PWR_EnableBkUpAccess();
				__HAL_RCC_BKPRAM_CLK_ENABLE();
				HAL_RTC_SetTime(&hrtc, &gTimeAdj, RTC_FORMAT_BIN);
				// Get the RTC current Date */
				HAL_RTC_SetDate(&hrtc, &gDateAdj, RTC_FORMAT_BIN);
				__HAL_RCC_BKPRAM_CLK_DISABLE();
				HAL_PWR_DisableBkUpAccess();
			}

			// Sincronizar a cada hora
			vTaskDelay(pdMS_TO_TICKS(3600000));
		} else {
			// Tentar novamente ap?s delay em caso de falha
			vTaskDelay(pdMS_TO_TICKS(NTP_RETRY_DELAY));
		}
	}
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void Mount_FATFS(void)
{
	  if(retUSER != 0 || retUSBH != 0) {
		  // Erro FATFS
		  printf("Erro MX_FATFS_Init !!\n");
	  }
	  else {
		  printf("MX_FATFS_Init OK !!\n");
	  }

	  // Mount FileSystem FLASH QSPI
	  fr = f_mount(&USERFatFS, "", 0);
	  if(fr != FR_OK) {
		  printf("Erro f_mount !!\n");
	  }
	  else {
		  printf("f_mount OK !!\n");
	  }
	  // Check freeSpace space
	  fr = f_getfree("0:", &fre_clust, &pfs);
	  if(fr != FR_OK){
		  printf("Erro f_getfree !!\n");
	  }
	  else {
		  printf("f_getfree OK !!\n");

		  totalSpace = (uint32_t)((pfs->n_fatent - 2) * pfs->csize * 0.5);
		  freeSpace = (uint32_t)(fre_clust * pfs->csize * 0.5);
		  SpaceUsed = totalSpace - freeSpace;
	  }
	  // Test Open config.txt
	  fr = f_open(&USERFile, "/Config.bin", FA_READ);
	  if(fr != FR_OK) {
		  printf("Erro f_open Config.bin !!\n");
	  }
	  else {
		  fr = f_sync(&USERFile);
		  size = f_size(&USERFile);
	      fr = f_read(&USERFile, line , size, &ByteRead);
	      if(fr == FR_OK) {
	    	  for(uint8_t x = 0; x < size-1; x++) {
				  version_flash[x] = line[x+26];
			  }
	      }
	      else {

	      }
	      f_close(&USERFile);
	  }
	  // Test Open File + 8 Caracteres Tela_Main_2025.bin
	  duracao = HAL_GetTick();
	  fr = f_open(&USERFile, "/Img_2025/LED_VD_2.bin", FA_READ);
	  if(fr != FR_OK) {
		  printf("Erro f_open LED_VD_2.bin !!\n");
	  }
	  else {
		 duracao = HAL_GetTick() - duracao;
	 	 size = f_size(&USERFile);
	      fr = f_read(&USERFile, line , 100, &ByteRead);
	      if(fr == FR_OK) {
	    	  printf("Erro f_read !!\n");
	      }
	      else {
	    	  printf("f_read OK !!\n");
	      }
	      f_close(&USERFile);
	  }
}

void my_loglvgl(lv_log_level_t level, const char *file, uint32_t line, const char *dsc)
{
}
/* USER CODE END Application */

