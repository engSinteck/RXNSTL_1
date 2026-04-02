/**
  ******************************************************************************
  * @file    LwIP/LwIP_HTTP_Server_Netconn_RTOS/Src/httpser-netconn.c 
  * @author  MCD Application Team
  * @brief   Basic http server implementation using LwIP netconn API  
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include "ctype.h"
#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/apps/fs.h"
#include "lwip.h"
#include "lwip/dns.h"
#include "lwip/stats.h"
#include "string.h"
#include "httpserver_netconn.h"
#include "cmsis_os.h"
#include "queue.h"
#include "semphr.h"
#include "tim.h"
#include "rtc.h"
//

#include "../Sinteck/lvgl/lvgl.h"
#include "../Sinteck/src/eeprom.h"
#include "../Sinteck/src/PE43711.h"
#include "../Sinteck/src/AD5242.h"
#include "../Sinteck/src/TPA6130A2.h"
#include "../Sinteck/src/mb1501.h"
#include "../Sinteck/src/defines.h"
#include "../Sinteck/src/PowerControl.h"
#include "../Sinteck/src/audio.h"
#include "../Sinteck/src/pwm.h"
#include "../Sinteck/tcp/ssl_client.h"
#include "../Sinteck/tcp/tcp_client.h"
#include "../Sinteck/tcp/json_util.h"
#include "../Sinteck/tcp/mqtt_paho.h"
#include "../cJSON/cJSON.h"

#define WEBSERVER_THREAD_PRIO    			( osPriorityLow5 )
#define LWIP_HTTPD_POST_MAX_PAYLOAD_LEN    	2048
#define HTTP_SOCKET_TIMEOUT_MS   			5000   // 5 segundos (ajustável)

// Estrutura para dados POST
typedef struct {
    char data[LWIP_HTTPD_POST_MAX_PAYLOAD_LEN];
    int data_received;
    int content_length;
} post_data_t;

// Variáveis globais
static post_data_t post_data;

extern osMutexId_t MutexHTTPDHandle;
extern MQTTClient mqttClient;

osThreadId ThreadHTTPDPHandle;
extern license_var lic;
extern Cfg_var cfg;
extern rds_var rds;
extern Profile_var Profile;
extern license_var lic;
extern SYS_UPTime uptime;
extern RTC_DateTypeDef gDate;
extern RTC_TimeTypeDef gTime;
RTC_DateTypeDef gDateAdj;
RTC_TimeTypeDef gTimeAdj;
SYS_Realtime Realtime;
extern AdvancedSettings adv;

extern volatile uint8_t flag_lvgl;
extern uint64_t falha;

uint32_t http_client = 0;
uint32_t timer_http_access = 0;
volatile uint8_t httpd_error = 0;
uint8_t http_access = 2;
uint8_t flag_telemetry = 0;

struct fs_file file;

extern uint8_t flag_telemetry;
extern uint32_t timer_reflesh;
extern const char* versao;
extern char version_flash[];
extern uint8_t Status_Battery, Status_Stereo, Status_FMDem;

const static char http_200_OK[] = "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n";
const static char http_200_OK_JSON[] = "HTTP/1.1 200 OK\r\nContent-type: application/json\r\n\r\n";
const static char http_400_BadRequest[] = "HTTP/1.1 400 BadRequest\r\nContent-type: text/html\r\n\r\n";
const static char http_401_Unauthorized[] = "HTTP/1.1 401 Unauthorized\r\nContent-type: text/html\r\n\r\n";

char buf_200_ok[104] = {0};
char buf_html[2504] = {0};
char str_mpx[104] = {0};
char str_status_off[4] = {0};
char str_status[104] = {0};
char str_freq[104] = {0};
char str_rds[24] = {0};
char str_ver[40] = {0};
char str_ver2[40] = {0};
char str_ver3[40] = {0};
char str_uptime[56] = {0};
char str_efic[16] = {0};
char str_target[16] = {0};
char out[1000] = {0};
char out1[104] = {0};
char rasc[104] = {0};
char rasc_freq[16] = {0};
char str_source[16] = {0};
char str_bat[16] = {0};
char str_sts[16] = {0};
char str_temp[152] = {0};
char buf_lvglmon[104] = {0};
char licsend[16] = {0};
char str_sn[32] = {0};

int val = 0;

portCHAR PAGE_BODY[1024];
portCHAR pagehits[10] = {0};
portCHAR heap[128] = {0};
uint32_t nPageHits = 0;
extern lv_mem_monitor_t mon;
uint32_t cnt_reset_lvgl = 0;

extern reset_cause_t reset_cause;
extern unsigned long stacked_r0;
extern unsigned long stacked_r1;
extern unsigned long stacked_r2;
extern unsigned long stacked_r3;
extern unsigned long stacked_r12;
extern unsigned long stacked_lr;
extern unsigned long stacked_pc;
extern unsigned long stacked_psr;
extern unsigned long stacked_bfar;
extern unsigned long stacked_cfsr;
extern unsigned long stacked_hfsr;
extern unsigned long stacked_dfsr;
extern unsigned long stacked_afsr;

extern struct netif gnetif;
extern ip4_addr_t ipaddr;
extern ip4_addr_t netmask;
extern ip4_addr_t gw;
extern ip4_addr_t dnsaddr;

extern volatile uint16_t adc_values[];
extern uint16_t adc_ext[];

void json_telemetry(struct netconn *conn)
{
	char response[1024] = {0};
	char str_uptime[32] = {0};
	char str_time[16] = {0};

	sprintf(str_uptime, "%ldd - %02ld:%02ld:%02ld", uptime.dia, (uptime.total/3600)%24, (uptime.total/60)%60, uptime.total%60);

	// Dia-Semana-Dia-Mes-Ano-HH:MM:SS
	sprintf(str_time, "%02d:%02d:%02d", gTime.Hours, gTime.Minutes, gTime.Seconds);

	cJSON *root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "FWD", 125.00);
	cJSON_AddNumberToObject(root, "REF", 8.00);
	cJSON_AddNumberToObject(root, "EFIC", 98.60);
	cJSON_AddNumberToObject(root, "TEMP", 33.20);
	cJSON_AddNumberToObject(root, "VPA", 55.00);
	cJSON_AddNumberToObject(root, "IPA", 7.00);
	cJSON_AddNumberToObject(root, "B1", 10);
	cJSON_AddNumberToObject(root, "B2", 12);
	cJSON_AddNumberToObject(root, "B3", 20);
	cJSON_AddStringToObject(root, "UPTIME", str_uptime);
	cJSON_AddNumberToObject(root, "FREQ", 937500);
	cJSON_AddStringToObject(root, "AUDIO", "MPX1");
	cJSON_AddStringToObject(root, "RDS", "ENABLE");
	cJSON_AddStringToObject(root, "STS", "OK");
	cJSON_AddStringToObject(root, "FAIL", "NONE");
	cJSON_AddStringToObject(root, "CLOCK", str_time);
	cJSON_AddStringToObject(root, "MODEL", "RXNSTL");
	cJSON_AddStringToObject(root, "SERIAL","0000-0000");

	char *json_string = cJSON_Print(root);
	printf("Generated JSON:\n%s\n", json_string);

	// Monta resposta HTTP completa
    snprintf(response, sizeof(response),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: application/json\r\n"
             "Access-Control-Allow-Origin: *\r\n"
             "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
             "Access-Control-Allow-Headers: Content-Type\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             strlen(json_string), json_string);

    // Envia resposta
    netconn_write(conn, response, strlen(response), NETCONN_COPY);

    cJSON_Delete(root); // Free the memory allocated for the JSON object
    free(json_string); // Free the memory allocated for the JSON string

}

// Função para enviar resposta JSON
void send_json_response(struct netconn *conn, const char *status, const char *message)
{
    char response[512];
    char json_body[256];

    // Cria o corpo JSON
    snprintf(json_body, sizeof(json_body),
             "{\"status\":\"%s\",\"message\":\"%s\",\"received_bytes\":%d,\"VPA\":56.6,\"IPA\":13.4,\"FWD\":125,\"REF\":12,\"EFIC\":98.6,\"TEMP\":35.2,\"B1\":10,\"B2\":12,\"B3\":20,\"FREQ\":940.0,\"AUDIO\":0,\"UPTIME\":0, \"RDS\":1, \"MODEL\":4, \"FAIL\":0, \"CLOCK\":10}",
             status, message, post_data.data_received);

    // Monta resposta HTTP completa
    snprintf(response, sizeof(response),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: application/json\r\n"
             "Access-Control-Allow-Origin: *\r\n"
             "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
             "Access-Control-Allow-Headers: Content-Type\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             strlen(json_body), json_body);

    // Envia resposta
    netconn_write(conn, response, strlen(response), NETCONN_COPY);
}

int process_json_post(char *json_data, int len)
{
	cJSON *root = cJSON_Parse(json_data);
    if (root == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            //logI("JSON Parse error: %s\n", error_ptr);
        }
        return -1;
    }
    //logI("=== JSON Recebido ===\n");
    // Extrai campos do JSON DPS
    cJSON *Text_DPS = cJSON_GetObjectItem(root, "DPS");
    if (cJSON_IsString(Text_DPS)) {
    	//logI("DPS: %s\n", Text_DPS->valuestring);
    }
    // Extrai campos do JSON RT
    cJSON *Text_RT = cJSON_GetObjectItem(root, "RT");
    if (cJSON_IsString(Text_RT)) {
    	//logI("RT: %s\n", Text_RT->valuestring);
    }

    cJSON_Delete(root);
    //logI("=== Fim do JSON ===\n");

    // Atualiza RDS Data
    if(strlen(Text_RT->valuestring) < 64) {
    	memcpy(rds.rt1, Text_RT->valuestring, strlen(Text_RT->valuestring));
    }

    if(strlen(Text_DPS->valuestring) < 64) {
    	memcpy(rds.dps1, Text_DPS->valuestring, strlen(Text_DPS->valuestring));
    }

    return 0;
}

// Função para extrair Content-Length do header
int extract_content_length(const char *header)
{
    char *cl_ptr = strstr(header, "Content-Length:");
    if (cl_ptr != NULL) {
        int length;
        if (sscanf(cl_ptr, "Content-Length: %d", &length) == 1) {
            return length;
        }
    }
    return -1;
}

const char * reset_cause_get_name(reset_cause_t reset_cause)
{
    const char * reset_cause_name = "TBD";

    switch (reset_cause)
    {
        case RESET_CAUSE_UNKNOWN:
            reset_cause_name = "UNKNOWN";
            break;
        case RESET_CAUSE_LOW_POWER_RESET:
            reset_cause_name = "LOW_POWER_RESET";
            break;
        case RESET_CAUSE_WINDOW_WATCHDOG_RESET:
            reset_cause_name = "WINDOW_WATCHDOG_RESET";
            break;
        case RESET_CAUSE_INDEPENDENT_WATCHDOG_RESET:
            reset_cause_name = "INDEPENDENT_WATCHDOG_RESET";
            break;
        case RESET_CAUSE_SOFTWARE_RESET:
            reset_cause_name = "SOFTWARE_RESET";
            break;
        case RESET_CAUSE_POWER_ON_POWER_DOWN_RESET:
            reset_cause_name = "POWER-ON_RESET (POR) / POWER-DOWN_RESET (PDR)";
            break;
        case RESET_CAUSE_EXTERNAL_RESET_PIN_RESET:
            reset_cause_name = "EXTERNAL_RESET_PIN_RESET";
            break;
        case RESET_CAUSE_BROWNOUT_RESET:
            reset_cause_name = "BROWNOUT_RESET (BOR)";
            break;
    }

    return reset_cause_name;
}

void prepare_license(void)
{
	if(cfg.License[0] >= 'A' && cfg.License[0] <= 'Z') licsend[0] = cfg.License[0];
	else licsend[0] = 'A';
	//
	if(cfg.License[1] >= 'A' && cfg.License[1] <= 'Z') licsend[1] = cfg.License[1];
	else licsend[1] = 'A';
	//
	if(cfg.License[2] >= 'A' && cfg.License[2] <= 'Z') licsend[2] = cfg.License[2];
	else licsend[2] = 'A';
	//
	if(cfg.License[3] >= 'A' && cfg.License[3] <= 'Z') licsend[3] = cfg.License[3];
	else licsend[3] = 'A';
	//
	if(cfg.License[4] >= 'A' && cfg.License[4] <= 'Z') licsend[4] = cfg.License[4];
	else licsend[4] = 'A';
	//
	if(cfg.License[5] >= 'A' && cfg.License[5] <= 'Z') licsend[5] = cfg.License[5];
	else licsend[5] = 'A';
	//
	if(cfg.License[6] >= 'A' && cfg.License[6] <= 'Z') licsend[6] = cfg.License[6];
	else licsend[6] = 'A';
	//
	if(cfg.License[7] >= 'A' && cfg.License[7] <= 'Z') licsend[7] = cfg.License[7];
	else licsend[7] = 'A';
	//
	licsend[8] = 0;
}

void DynWebPage(void)
{
  memset(PAGE_BODY, 0, 1024);

  /* Update the hit count */
  nPageHits++;
  sprintf(pagehits, "%d", (int)nPageHits);
  strcat(PAGE_BODY, pagehits);
  strcat((char *)PAGE_BODY, "<pre><br>Name          State  Priority  Stack      Num" );
  strcat((char *)PAGE_BODY, "<br>---------------------------------------------------------------------------<br>");

  /* The list of tasks and their status */
  vTaskList((char *)(PAGE_BODY + strlen(PAGE_BODY)));

  sprintf(heap, "<br>Heap Size: %d Free Heap: %d  Heap Size: %d ", configTOTAL_HEAP_SIZE, xPortGetFreeHeapSize(), xPortGetMinimumEverFreeHeapSize());
  strcat((char *)PAGE_BODY, heap);
  strcat((char *)PAGE_BODY, "<br><br>---------------------------------------------------------------------------");
  strcat((char *)PAGE_BODY, "<br>B : Blocked, X : Running, R : Ready, D : Deleted, S : Suspended<br>");
}

void resp_http_200(struct netconn *conn)
{
	sprintf(buf_200_ok, "%s OK", http_200_OK);
	netconn_write(conn, buf_200_ok, strlen(buf_200_ok), NETCONN_COPY);
}

void resp_http_400(struct netconn *conn)
{
	sprintf(buf_200_ok, "%s ERROR", http_400_BadRequest);
	netconn_write(conn, buf_200_ok, strlen(buf_200_ok), NETCONN_COPY);
}

void resp_http_401(struct netconn *conn)
{
	sprintf(buf_200_ok, "%s ERROR", http_401_Unauthorized);
	netconn_write(conn, buf_200_ok, strlen(buf_200_ok), NETCONN_COPY);
}

float Calcule_efic(void)
{
	return 100.0f;
}

float Get_Forward(void)
{
	float ret;

	ret = 5.0f;

	// Retorna
	return ret;
}

char * Get_Target(char * str)
{
	sprintf(str, "%0.0f W", 0.0f);
	return str;
}

void strstr_substring(const char *src, const char *delim1, const char *delim2, int pos)
{
        int len;
        char *ret;
        char *ret1;
        char text[700] = {0};

        memset(out, 0, 1000);
        ret = strstr(src, delim1);
        if(ret == NULL) return;

        ret1 = strstr(src, delim2);
        if(ret1 == NULL) return;

        len = ((int)(ret1 - src) - (int)(ret - src)) - 1;
        if(len < 0) return;
        if(len > 700) return;

        memcpy(text, ret, len);
        for(int x=0; x < len-pos; x++) {
        	out[x] = text[x+pos];
        }
}

static inline int ishex(int x)
{
	return	(x >= '0' && x <= '9')	||
			(x >= 'a' && x <= 'f')	||
			(x >= 'A' && x <= 'F');
}

int decode_string(const char *s, char *dec)
{
	char *o;
	const char *end = s + strlen(s);
	int c;

	for (o = dec; s <= end; o++) {
		c = *s++;
		if (c == '+') c = ' ';
		else if (c == '%' && (	!ishex(*s++) ||
					!ishex(*s++) ||
					!sscanf(s - 2, "%2x", &c)))
			return -1;

		if (dec) *o = c;
	}

	return o - dec;
}

static void handle_http_request(struct netconn *conn, const char *buf, uint16_t buflen)
{
	uint8_t x = 0;

	// Is this an HTTP GET command? (only check the first 5 chars, since
    // there are other formats for GET, and we're keeping it very simple )
	if ((buflen >=5) &&
	   ((strncmp(buf, "GET /", 5) == 0) ||
	   (strncmp(buf, "POST /", 6) == 0) ||
	   (strncmp(buf, "HEAD /", 6) == 0))) {
		if (strncmp((char const *)buf,"HEAD /robots.txt", 16) == 0) {
			// Load Error page
    		fs_open(&file, "/404.html");
    		netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
    		fs_close(&file);
    	}
    	// Index.html login.html
    	else if (strncmp((char const *)buf,"GET / ", 6) == 0) {
			fs_open(&file, "/login.html");
			netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
    		fs_close(&file);
    	}
    	// Check if request to get ST.gif
		else if (strncmp((char const *)buf,"GET /STM32H7xx_files/ST.gif", 27)==0) {
			fs_open(&file, "/STM32H7xx_files/ST.gif");
			netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
			fs_close(&file);
		}
    	// Check if request to get stm32.jpg
		else if (strncmp((char const *)buf,"GET /STM32H7xx_files/stm32.jpg",30)==0) {
			fs_open(&file, "/STM32H7xx_files/stm32.jpg");
			netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
			fs_close(&file);
		}
		else if (strncmp((char const *)buf,"GET /STM32H7xx_files/logo.jpg", 29) == 0) {
			// Check if request to get ST logo.jpg
			fs_open(&file, "/STM32H7xx_files/logo.jpg");
			netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
			fs_close(&file);
		}
		else if (strncmp((char const *)buf,"GET /css/style.css", 18) == 0) {
			// Check if request to get CSS Style
			fs_open(&file, "/css/style.css");
    		netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
    		fs_close(&file);
		}
		else if (strncmp((char const *)buf,"GET /css/style_new.css", 22) == 0) {
			// Check if request to get CSS Style
			fs_open(&file, "/css/style_new.css");
		    netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
		    fs_close(&file);
		}
		else if (strncmp((char const *)buf,"GET /js/dashboard.js", 20) == 0) {
			// Check if request to get Javacript
			fs_open(&file, "/js/dashboard.js");
			netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
			fs_close(&file);
		}
		else if (strncmp((char const *)buf,"GET /js/utils.js", 16) == 0) {
			// Check if request to get Javacript
			fs_open(&file, "/js/utils.js");
			netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
			fs_close(&file);
		}
		else if (strncmp((char const *)buf,"GET /STM32H7xx_files/favicon.ico", 29) == 0) {
			// Check if request to get ST logo.jpg
			fs_open(&file, "/STM32H7xx_files/favicon.ico");
			netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
			fs_close(&file);
		}
		else if (strncmp((char const *)buf,"GET /STM32H7xx_files/sics.gif", 29) == 0) {
			// Check if request to get ST logo.jpg
			fs_open(&file, "/STM32H7xx_files/sics.gif");
			netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
			fs_close(&file);
		}
		else if(( (strncmp(buf, "GET /STM32H7xx.html", 19) == 0) ||
		          (strncmp(buf, "GET /index.html", 15) == 0) ) &&
				  (http_access != 0) )    {

			// Load STM32H7xx page
			fs_open(&file, "/STM32H7xx.html");
			netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
			fs_close(&file);
		}
		else if( (strncmp(buf, "GET /index_new.html", 19) == 0) && (http_access != 0) ) {
			// Load index_new page
			fs_open(&file, "/index_new.html");
			netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
			fs_close(&file);
		}
		else if((strncmp(buf, "GET /audio.html", 15) == 0) && (http_access == 2)) {
			// Load Audio page
			fs_open(&file, "/audio.html");
			netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
			fs_close(&file);
		}
		else if((strncmp(buf, "GET /time.html", 14) == 0) && (http_access == 2)) {
			// Load Time page
			fs_open(&file, "/time.html");
			netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
			fs_close(&file);
		}
		else if((strncmp(buf, "GET /rds.html", 13) == 0) && (http_access == 2)) {
			// Load Time page
			fs_open(&file, "/rds.html");
			netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
			fs_close(&file);
		}
		else if((strncmp(buf, "GET /freq.html", 14) == 0) && (http_access == 2)) {
			// Load Time page
			fs_open(&file, "/freq.html");
			netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
			fs_close(&file);
		}
		else if((strncmp(buf, "GET /advset.html", 16) == 0) && (http_access == 2)) {
			// Load Time page
			fs_open(&file, "/advset.html");
			netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
			fs_close(&file);
		}
		else if((strncmp(buf, "GET /alerts.html", 16) == 0) && (http_access == 2)) {
			// Load Time page
			fs_open(&file, "/alerts.html");
			netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
			fs_close(&file);
		}
		else if((strncmp(buf, "GET /network.html", 17) == 0) && (http_access == 2)) {
			// Load Time page
			fs_open(&file, "/network.html");
			netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
			fs_close(&file);
		}
		else if((strncmp(buf, "GET /password.html", 18) == 0) && (http_access == 2)) {
			// Load Time page
			fs_open(&file, "/password.html");
			netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
			fs_close(&file);
		}
    	else if((strncmp(buf, "GET /confighold.html", 20) == 0) && (http_access == 2)) {
			// Load Time page
			fs_open(&file, "/confighold.html");
			netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
			fs_close(&file);
		}
		else if((strncmp(buf, "GET /login.html", 15) == 0)) {
			// Load Time page
			fs_open(&file, "/login.html");
			netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
			fs_close(&file);
		}
		else if((strncmp(buf, "GET /profile.html", 17) == 0)) {
			// Load Time page
          	fs_open(&file, "/profile.html");
          	netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
          	fs_close(&file);
		}
		else if((strncmp(buf, "GET /Fail.html", 14) == 0)) {
    		  /* Load Time page */
    		  fs_open(&file, "/Fail.html");
    		  netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
    		  fs_close(&file);
		}
		else if((strncmp(buf, "GET /reboot.html", 16) == 0)) {
			// Load Reboot page
    		fs_open(&file, "/reboot.html");
    		netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
    		fs_close(&file);
    		RF_Disable();
    		osDelay(1000);
    		HAL_NVIC_SystemReset();
		}
		else if((strncmp(buf, "GET /ClearLicense.html", 22) == 0)) {
			// Load ClearLicense page
			fs_open(&file, "/ClearLicense.html");
			netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
			fs_close(&file);
			//
			lic.LicSeq = 0;
			lic.licenseTimer = 999;
			cfg.License[0] = 0xFF; cfg.License[1] = 0xFF; cfg.License[2] = 0xFF; cfg.License[3] = 0xFF;
			cfg.License[4] = 0xFF; cfg.License[5] = 0xFF; cfg.License[6] = 0xFF; cfg.License[7] = 0xFF;
			flag_telemetry = 28;
		}
		else if( (strncmp(buf, "GET /tasks.html", 15) == 0) && (http_access == 2)) {
			// Load dynamic page
			fs_open(&file, "/tasks.html");
			netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
			fs_close(&file);
		}
		else if(strncmp(buf, "GET /Debug.html", 15) == 0) {
			// Load Debug page
			fs_open(&file, "/Debug.html");
			netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
			fs_close(&file);
		}
		else if((strncmp(buf, "GET /Token.html", 15) == 0) && (http_access == 2)) {
			// Load Time page
			fs_open(&file, "/Token.html");
			netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
			fs_close(&file);
		}
		else if((strncmp(buf, "GET /service.html", 17) == 0) && (http_access == 2)) {
			// Load Time page
			fs_open(&file, "/service.html");
			netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
			fs_close(&file);
		}
		else if((strncmp(buf, "GET /Forward_Graph.html", 23) == 0)) {
			// Load Time page
			fs_open(&file, "/Forward_Graph.html");
			netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
			fs_close(&file);
		}
		else if((strncmp(buf, "GET /Reflected_Graph.html", 25) == 0)) {
			// Load Time page
			fs_open(&file, "/Reflected_Graph.html");
			netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
			fs_close(&file);
		}
		else if((strncmp(buf, "GET /Temperature_Graph.html", 27) == 0)) {
			// Load Time page
			fs_open(&file, "/Temperature_Graph.html");
			netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
			fs_close(&file);
		}
		else if((strncmp(buf, "GET /rf.html", 12) == 0)) {
			// Load Time page
			fs_open(&file, "/rf.html");
			netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
			fs_close(&file);
		}
		else if((strncmp(buf, "GET /mp3.html", 13) == 0)) {
			// Load Time page
			fs_open(&file, "/mp3.html");
			netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
			fs_close(&file);
		}
		else if((strncmp(buf, "POST /login.html", 16) == 0) || (strncmp(buf, "POST / ", 6) == 0)) {
			char user[16] = {0};
			char pass[16] = {0};

			strstr_substring(buf, "user=", "pass=", 5);
			strcpy(user, out);
			strstr_substring(buf, "pass=", "fim=", 5);
			strcpy(pass, out);

			if ((strlen(user) > 0) && (strlen(user) < 16) && (strlen(pass) > 0) && (strlen(pass) < 16)) {
				// provide contiguous storage if p is a chained pbuf
				char buf_password[10];
				char buf_user_pass[10];

    			buf_password[0] = cfg.PassAdmin[0] + '0';
    			buf_password[1] = cfg.PassAdmin[1] + '0';
    			buf_password[2] = cfg.PassAdmin[2] + '0';
    			buf_password[3] = cfg.PassAdmin[3] + '0';
    			buf_password[4] = 0;

    			buf_user_pass[0] = cfg.PassUser[0] + '0';
    			buf_user_pass[1] = cfg.PassUser[1] + '0';
    			buf_user_pass[2] = cfg.PassUser[2] + '0';
    			buf_user_pass[3] = cfg.PassUser[3] + '0';
    			buf_user_pass[4] = 0;

				if (!strcmp(user, "admin") && !strcmp(pass, buf_password)) {
					// user and password are correct, create a "session"
    				// Load STM32H7xx page
					timer_http_access = 0;
					http_access = 2;
					fs_open(&file, "/STM32H7xx.html");
					netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
					fs_close(&file);
				}
				else if (!strcmp(user, "user") && !strcmp(pass, buf_user_pass)) {
					// user and password are correct, create a "session"
					// Load STM32H7xx page
					timer_http_access = 0;
					http_access = 1;
					fs_open(&file, "/STM32H7xx.html");
					netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
					fs_close(&file);
				}
				else {
					// Load Time page
					timer_http_access = 0;
    				http_access = 0;
    				fs_open(&file, "/loginfail.html");
    				netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
    				fs_close(&file);
    			}
    		}
			else {
				// Load Error page
				fs_open(&file, "/404.html");
				netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
				fs_close(&file);
			}
		}
		else if((strncmp(buf, "GET /readTime", 13) == 0)) {
			cJSON *root = cJSON_CreateObject();
			char *json_string = NULL;

			if (root == NULL) {
		         // Erro ao criar JSON - fallback para resposta simples
		         sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"JSON creation failed\"}", http_200_OK_JSON);
		         netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		         return;
		     }

		    // === Dados CLOCK ===
		    char str_date[32] = {0};
			sprintf(str_date, "%02d/%02d/%04d", gDate.Date, gDate.Month, (2000+gDate.Year));
			cJSON_AddStringToObject(root, "date", str_date);

			char str_time[32] = {0};
			sprintf(str_time, "%02d:%02d:%02d", gTime.Hours, gTime.Minutes, gTime.Seconds);
			cJSON_AddStringToObject(root, "time", str_time);
			cJSON_AddNumberToObject(root, "weeday", gDate.WeekDay);
			cJSON_AddNumberToObject(root, "timezone", cfg.Timezone);
			cJSON_AddBoolToObject(root,   "ntp", cfg.NTP);

		    // Converter JSON para string
		    json_string = cJSON_PrintUnformatted(root);

		    if(json_string != NULL) {
		        // Montar resposta HTTP
		        int json_len = strlen(json_string);
		        int response_len = strlen(http_200_OK_JSON) + json_len + 1;
		        char *response = (char*)malloc(response_len);

		        if(response != NULL) {
		            snprintf(response, response_len, "%s%s", http_200_OK_JSON, json_string);
		            netconn_write(conn, response, strlen(response), NETCONN_COPY);
		            free(response);
		        } else {
		            // Fallback se malloc falhar
		            sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"memory allocation failed\"}", http_200_OK_JSON);
		            netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		        }
		        free(json_string);
		    } else {
		        // Fallback se cJSON_Print falhar
		        sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"JSON serialization failed\"}", http_200_OK_JSON);
		        netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		    }
		    cJSON_Delete(root);
		}
		else if((strncmp(buf, "GET /setTime=", 13) == 0)) {
			resp_http_200(conn);
			// Dia-Semana-Dia-Mes-Ano-HH:MM:SS
			gDateAdj.WeekDay = buf[13] - '0';
    		gDateAdj.Year = ((buf[22] - '0') * 10) + ((buf[23] - '0') * 1);
    		gDateAdj.Month = ((buf[25] - '0') * 10) + ((buf[26] - '0') * 1);
    		gDateAdj.Date = ((buf[28] - '0') * 10) + ((buf[29] - '0') * 1);

    		gTimeAdj.Hours = ((buf[36] - '0') * 10) + ((buf[37] - '0') * 1);
    		gTimeAdj.Minutes = ((buf[39] - '0') * 10) + ((buf[40] - '0') * 1);
    		gTimeAdj.Seconds = ((buf[42] - '0') * 10) + ((buf[43] - '0') * 1);

    		cfg.NTP = buf[49] - '0';
    		char fuso_str[4] = {0};
    		fuso_str[0] = buf[56]; fuso_str[1] = buf[57]; fuso_str[2] = '\0';
    		cfg.Timezone = atoi(fuso_str);
			// Acerta Relogio
			HAL_PWR_EnableBkUpAccess();
    		__HAL_RCC_BKPRAM_CLK_ENABLE();
    		HAL_RTC_SetTime(&hrtc, &gTimeAdj, RTC_FORMAT_BIN);
    		// Get the RTC current Date */
    		HAL_RTC_SetDate(&hrtc, &gDateAdj, RTC_FORMAT_BIN);
    		__HAL_RCC_BKPRAM_CLK_DISABLE();
    		HAL_PWR_DisableBkUpAccess();
    		//
    		flag_telemetry = 4;
		}
    	else if((strncmp(buf, "GET /readFreq", 13) == 0)) {
			cJSON *root = cJSON_CreateObject();
			char *json_string = NULL;

			if (root == NULL) {
		         // Erro ao criar JSON - fallback para resposta simples
		         sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"JSON creation failed\"}", http_200_OK_JSON);
		         netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		         return;
		     }

		    // === Frequencia ===
			cJSON_AddNumberToObject(root, "freq", cfg.Frequencia);

		    // Converter JSON para string
		    json_string = cJSON_PrintUnformatted(root);

		    if(json_string != NULL) {
		        // Montar resposta HTTP
		        int json_len = strlen(json_string);
		        int response_len = strlen(http_200_OK_JSON) + json_len + 1;
		        char *response = (char*)malloc(response_len);

		        if(response != NULL) {
		            snprintf(response, response_len, "%s%s", http_200_OK_JSON, json_string);
		            netconn_write(conn, response, strlen(response), NETCONN_COPY);
		            free(response);
		        } else {
		            // Fallback se malloc falhar
		            sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"memory allocation failed\"}", http_200_OK_JSON);
		            netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		        }
		        free(json_string);
		    } else {
		        // Fallback se cJSON_Print falhar
		        sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"JSON serialization failed\"}", http_200_OK_JSON);
		        netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		    }
		    cJSON_Delete(root);
		}
		else if((strncmp(buf, "GET /setFreq=", 13) == 0)) {
			resp_http_200(conn);
			rasc[0] = 0; rasc[1] = 0; rasc[2] = 0;
			rasc[3] = 0; rasc[4] = 0; rasc[5] = 0;
			rasc[6] = 0; rasc[7] = 0; rasc[8] = 0;
			for(x = 0; x < 5; x++) {
				if(buf[x+13] != ';') {
					rasc_freq[x] = buf[x+13];
				}
				else {
					rasc_freq[x] = 0;
					x = 5;
				}
			}
			if(atoi(rasc_freq) >= 937500 && atoi(rasc_freq) <= 960000) {
				cfg.Frequencia = (long int)atoi(rasc_freq);
				// Desliga RF Power
				__HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_1, 0);
				mb1501(cfg.Frequencia);		// Update PLL
				flag_telemetry = 7;
			}
		}
		else if((strncmp(buf, "GET /resetpassword=yes", 22) == 0)) {
			sprintf(buf_html, "OK");
			netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
			flag_telemetry = 253;
		}
		else if((strncmp(buf, "GET /readNetwork", 16) == 0)) {
			cJSON *root = cJSON_CreateObject();
			char *json_string = NULL;

			if (root == NULL) {
		         // Erro ao criar JSON - fallback para resposta simples
		         sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"JSON creation failed\"}", http_200_OK_JSON);
		         netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		         return;
		     }

		    // === Dados Network ===
		    char str_ip[32] = {0};
			sprintf(str_ip, "%3d.%3d.%3d.%3d", cfg.IP_ADDR[0], cfg.IP_ADDR[1], cfg.IP_ADDR[2], cfg.IP_ADDR[3] );
			cJSON_AddStringToObject(root, "ip", str_ip);

		    char str_mask[32] = {0};
			sprintf(str_mask, "%3d.%3d.%3d.%3d", cfg.MASK_ADDR[0], cfg.MASK_ADDR[1], cfg.MASK_ADDR[2], cfg.MASK_ADDR[3] );
			cJSON_AddStringToObject(root, "mask", str_mask);

			char str_gw[32] = {0};
			sprintf(str_gw, "%3d.%3d.%3d.%3d", cfg.GW_ADDR[0], cfg.GW_ADDR[1], cfg.GW_ADDR[2], cfg.GW_ADDR[3] );
			cJSON_AddStringToObject(root, "gw", str_gw);

			char str_dns[32] = {0};
			sprintf(str_dns, "%3d.%3d.%3d.%3d", cfg.DNS_ADDR[0], cfg.DNS_ADDR[1], cfg.DNS_ADDR[2], cfg.DNS_ADDR[3] );
			cJSON_AddStringToObject(root, "dns", str_dns);

			cJSON_AddNumberToObject(root, "port", cfg.PortWEB);
			cJSON_AddNumberToObject(root, "snmp", cfg.EnableSNMP);

			char str_mqtt[32] = {0};
			sprintf(str_mqtt, "%3d.%3d.%3d.%3d", 3, 23, 178, 219 );
			cJSON_AddStringToObject(root, "mqtt", str_mqtt);

		    // Converter JSON para string
		    json_string = cJSON_PrintUnformatted(root);

		    if(json_string != NULL) {
		        // Montar resposta HTTP
		        int json_len = strlen(json_string);
		        int response_len = strlen(http_200_OK_JSON) + json_len + 1;
		        char *response = (char*)malloc(response_len);

		        if(response != NULL) {
		            snprintf(response, response_len, "%s%s", http_200_OK_JSON, json_string);
		            netconn_write(conn, response, strlen(response), NETCONN_COPY);
		            free(response);
		        } else {
		            // Fallback se malloc falhar
		            sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"memory allocation failed\"}", http_200_OK_JSON);
		            netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		        }
		        free(json_string);
		    } else {
		        // Fallback se cJSON_Print falhar
		        sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"JSON serialization failed\"}", http_200_OK_JSON);
		        netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		    }
		    cJSON_Delete(root);
		}
		else if((strncmp(buf, "GET /setNetwork=", 16) == 0)) {
			resp_http_200(conn);
    	    strstr_substring(buf, "IP1:", "IP2:", 4);
    	    cfg.IP_ADDR[0] = atoi(out);
    	    strstr_substring(buf, "IP2:", "IP3:", 4);
    	    cfg.IP_ADDR[1] = atoi(out);
    	    strstr_substring(buf, "IP3:", "IP4:", 4);
    	    cfg.IP_ADDR[2] = atoi(out);
    	    strstr_substring(buf, "IP4:", "MASK1:", 4);
    	    cfg.IP_ADDR[3] = atoi(out);
			//
    	    strstr_substring(buf, "MASK1:", "MASK2:", 6);
    	    cfg.MASK_ADDR[0] = atoi(out);
    	    strstr_substring(buf, "MASK2:", "MASK3:", 6);
    	    cfg.MASK_ADDR[1] = atoi(out);
    	    strstr_substring(buf, "MASK3:", "MASK4:", 6);
    	    cfg.MASK_ADDR[2] = atoi(out);
    	    strstr_substring(buf, "MASK4:", "GW1:", 6);
    	    cfg.MASK_ADDR[3] = atoi(out);
			//
    	    strstr_substring(buf, "GW1:", "GW2:", 4);
    	    cfg.GW_ADDR[0] = atoi(out);
    	    strstr_substring(buf, "GW2:", "GW3:", 4);
    	    cfg.GW_ADDR[1] = atoi(out);
    	    strstr_substring(buf, "GW3:", "GW4:", 4);
    	    cfg.GW_ADDR[2] = atoi(out);
    	    strstr_substring(buf, "GW4:", "DNS1:", 4);
    	    cfg.GW_ADDR[3] = atoi(out);
			//
    	    strstr_substring(buf, "DNS1:", "DNS2:", 5);
			cfg.DNS_ADDR[0] = atoi(out);
    	    strstr_substring(buf, "DNS2:", "DNS3:", 5);
    	    cfg.DNS_ADDR[1] = atoi(out);
    	    strstr_substring(buf, "DNS3:", "DNS4:", 5);
    	    cfg.DNS_ADDR[2] = atoi(out);
    	    strstr_substring(buf, "DNS4:", "PORT:", 5);
    	    cfg.DNS_ADDR[3] = atoi(out);
    	    strstr_substring(buf, "PORT:", "FIM", 5);
    	    cfg.PortWEB = atoi(out);
			//
			// Atualiza IPS
			IP4_ADDR(&ipaddr, cfg.IP_ADDR[0], cfg.IP_ADDR[1], cfg.IP_ADDR[2], cfg.IP_ADDR[3]);
			IP4_ADDR(&netmask, cfg.MASK_ADDR[0], cfg.MASK_ADDR[1] , cfg.MASK_ADDR[2], cfg.MASK_ADDR[3]);
			IP4_ADDR(&gw, cfg.GW_ADDR[0], cfg.GW_ADDR[1], cfg.GW_ADDR[2], cfg.GW_ADDR[3]);
			IP4_ADDR(&dnsaddr, cfg.DNS_ADDR[0], cfg.DNS_ADDR[1], cfg.DNS_ADDR[2], cfg.DNS_ADDR[3]);
			dns_setserver(0, &dnsaddr);
			netif_set_addr(&gnetif, &ipaddr, &netmask, &gw);

			flag_telemetry = 12;
		}
		else if((strncmp(buf, "GET /setSNMP=", 13) == 0)) {
			resp_http_200(conn);
			if(buf[13] == '1') {
				cfg.EnableSNMP = 1;
			}
			else {
				cfg.EnableSNMP = 0;
			}
			flag_telemetry = 31;
		}
    	else if((strncmp(buf, "GET /GetSaveToken=", 18) == 0)) {
			resp_http_200(conn);
			flag_telemetry = 25;
		}
		else if((strncmp(buf, "GET /GetSaveBroker=", 19) == 0)) {
			resp_http_200(conn);
			strstr_substring(buf, "BROKER:", "FIM", 7);
			flag_telemetry = 29;
		}
		else if((strncmp(buf, "GET /GetToken=", 14) == 0)) {
			resp_http_200(conn);
			Get_Token();
		}
		else if((strncmp(buf, "GET /readPassword", 17) == 0)) {
			cJSON *root = cJSON_CreateObject();
			char *json_string = NULL;

			if (root == NULL) {
		         // Erro ao criar JSON - fallback para resposta simples
		         sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"JSON creation failed\"}", http_200_OK_JSON);
		         netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		         return;
		     }

		    // === Dados Password ===
		    char str_admin[32] = {0};
			sprintf(str_admin, "%d%d%d%d", cfg.PassAdmin[0], cfg.PassAdmin[1], cfg.PassAdmin[2], cfg.PassAdmin[3]);
			cJSON_AddStringToObject(root, "admin", str_admin);

		    char str_user[32] = {0};
			sprintf(str_user, "%d%d%d%d", cfg.PassUser[0], cfg.PassUser[1], cfg.PassUser[2], cfg.PassUser[3] );
			cJSON_AddStringToObject(root, "user", str_user);

			// Converter JSON para string
		    json_string = cJSON_PrintUnformatted(root);

		    if(json_string != NULL) {
		        // Montar resposta HTTP
		        int json_len = strlen(json_string);
		        int response_len = strlen(http_200_OK_JSON) + json_len + 1;
		        char *response = (char*)malloc(response_len);

		        if(response != NULL) {
		            snprintf(response, response_len, "%s%s", http_200_OK_JSON, json_string);
		            netconn_write(conn, response, strlen(response), NETCONN_COPY);
		            free(response);
		        } else {
		            // Fallback se malloc falhar
		            sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"memory allocation failed\"}", http_200_OK_JSON);
		            netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		        }
		        free(json_string);
		    } else {
		        // Fallback se cJSON_Print falhar
		        sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"JSON serialization failed\"}", http_200_OK_JSON);
		        netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		    }
		    cJSON_Delete(root);
		}
		else if((strncmp(buf, "GET /setPassword=", 17) == 0)) {
			resp_http_200(conn);
			if( (buf[17] >= '0' && buf[17] <= '9') && (buf[18] >= '0' && buf[18] <= '9')  &&
    	        (buf[19] >= '0' && buf[19] <= '9') && (buf[20] >= '0' && buf[20] <= '9') ) {
				// Admin
				cfg.PassAdmin[0] = buf[17] - '0';
				cfg.PassAdmin[1] = buf[18] - '0';
				cfg.PassAdmin[2] = buf[19] - '0';
				cfg.PassAdmin[3] = buf[20] - '0';
			}
			else {
				// Admin
				cfg.PassAdmin[0] = 1;
    			cfg.PassAdmin[1] = 2;
				cfg.PassAdmin[2] = 3;
				cfg.PassAdmin[3] = 4;
			}
			if( (buf[22] >= '0' && buf[22] <= '9') && (buf[23] >= '0' && buf[23] <= '9')  &&
    	        (buf[24] >= '0' && buf[24] <= '9') && (buf[25] >= '0' && buf[25] <= '9') ) {
				// User
				cfg.PassUser[0] = buf[22] - '0';
				cfg.PassUser[1] = buf[23] - '0';
    			cfg.PassUser[2] = buf[24] - '0';
    			cfg.PassUser[3] = buf[25] - '0';
			}
			else {
				// User
				cfg.PassUser[0] = 1;
				cfg.PassUser[1] = 2;
				cfg.PassUser[2] = 3;
				cfg.PassUser[3] = 4;
			}
			flag_telemetry = 9;
		}
		//
		else if((strncmp(buf, "GET /readProfile", 16) == 0) ) {
			cJSON *root = cJSON_CreateObject();
			char *json_string = NULL;

			if (root == NULL) {
		         // Erro ao criar JSON - fallback para resposta simples
		         sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"JSON creation failed\"}", http_200_OK_JSON);
		         netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		         return;
		     }

		    // === Dados Password ===
			cJSON_AddStringToObject(root, "station", Profile.Station);
			cJSON_AddStringToObject(root, "city", Profile.City);
			cJSON_AddStringToObject(root, "state", Profile.State);
			cJSON_AddStringToObject(root, "country", Profile.Country);
			cJSON_AddStringToObject(root, "exttemp", Profile.Temp);

			// Converter JSON para string
		    json_string = cJSON_PrintUnformatted(root);

		    if(json_string != NULL) {
		        // Montar resposta HTTP
		        int json_len = strlen(json_string);
		        int response_len = strlen(http_200_OK_JSON) + json_len + 1;
		        char *response = (char*)malloc(response_len);

		        if(response != NULL) {
		            snprintf(response, response_len, "%s%s", http_200_OK_JSON, json_string);
		            netconn_write(conn, response, strlen(response), NETCONN_COPY);
		            free(response);
		        } else {
		            // Fallback se malloc falhar
		            sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"memory allocation failed\"}", http_200_OK_JSON);
		            netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		        }
		        free(json_string);
		    } else {
		        // Fallback se cJSON_Print falhar
		        sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"JSON serialization failed\"}", http_200_OK_JSON);
		        netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		    }
		    cJSON_Delete(root);
		}
		else if((strncmp(buf, "GET /setStationProfile=", 23) == 0) ) {
			sprintf(buf_html, "%s OK", http_200_OK);
			netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
			// Profile
			strstr_substring(buf, "STATION:", "CITY:", 8);
			decode_string(out, out1);
			memset(Profile.Station, 0 , 50);
			if(strlen(out1) > 50) {
				memcpy(Profile.Station, out1, 48);
				Profile.Station[48] = 0;
				Profile.Station[49] = 0;
			}
			else {
				memcpy(Profile.Station, out1, strlen(out1));
			}
			// CITY
			strstr_substring(buf, "CITY:", "STATE:", 5);
			decode_string(out, out1);
			memset(Profile.City, 0 , 50);
			if(strlen(out1) > 50) {
				memcpy(Profile.City, out1, 48);
				Profile.City[48] = 0;
				Profile.City[49] = 0;
			}
			else {
				memcpy(Profile.City, out1, strlen(out1));
			}
			// Estado
			strstr_substring(buf, "STATE:", "COUNTRY:", 6);
			decode_string(out, out1);
			memset(Profile.State, 0 , 50);
			if(strlen(out1) > 50) {
				memcpy(Profile.State, out1, 48);
				Profile.State[48] = 0;
				Profile.State[49] = 0;
			}
			else {
				memcpy(Profile.State, out1, strlen(out1));
			}
			// Pais
			strstr_substring(buf, "COUNTRY:", "FIM:", 8);
			decode_string(out, out1);
			memset(Profile.Country, 0 , 50);
			if(strlen(out1) > 50) {
				memcpy(Profile.Country, out1, 48);
				Profile.Country[48] = 0;
				Profile.Country[49] = 0;
			}
			else {
				memcpy(Profile.Country, out1, strlen(out1));
			}
			// Marca para Salvar em EEPROM
			flag_telemetry = 27;
		}
    	else if((strncmp(buf, "GET /readConfigHold", 19) == 0)) {
    	    cJSON *root = cJSON_CreateObject();
			char *json_string = NULL;

			if (root == NULL) {
		         // Erro ao criar JSON - fallback para resposta simples
		         sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"JSON creation failed\"}", http_200_OK_JSON);
		         netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		         return;
		     }

		    // === Dados ConfigHold ===
			cJSON_AddNumberToObject(root, "confighold", cfg.ConfigHold);
			cJSON_AddNumberToObject(root, "vswr", 0.0);
			cJSON_AddNumberToObject(root, "reflected", Realtime.Reflected);
			cJSON_AddNumberToObject(root, "relay1", Realtime.Relay1);
			cJSON_AddNumberToObject(root, "relay2", Realtime.Relay2);

			// Converter JSON para string
		    json_string = cJSON_PrintUnformatted(root);

		    if(json_string != NULL) {
		        // Montar resposta HTTP
		        int json_len = strlen(json_string);
		        int response_len = strlen(http_200_OK_JSON) + json_len + 1;
		        char *response = (char*)malloc(response_len);

		        if(response != NULL) {
		            snprintf(response, response_len, "%s%s", http_200_OK_JSON, json_string);
		            netconn_write(conn, response, strlen(response), NETCONN_COPY);
		            free(response);
		        } else {
		            // Fallback se malloc falhar
		            sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"memory allocation failed\"}", http_200_OK_JSON);
		            netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		        }
		        free(json_string);
		    } else {
		        // Fallback se cJSON_Print falhar
		        sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"JSON serialization failed\"}", http_200_OK_JSON);
		        netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		    }
		    cJSON_Delete(root);
		}
		else if((strncmp(buf, "GET /setConfigHold=", 19) == 0)) {
			resp_http_200(conn);
			cfg.ConfigHold = buf[19] - '0';
			strstr_substring(buf, "REM1:", "REM2:", 5);
			Realtime.Relay1 = atoi(out);
			strstr_substring(buf, "REM2:", "FIM:", 5);
			Realtime.Relay2 = atoi(out);

			// Atualiza Reles
			HAL_GPIO_WritePin(RELAY_1_GPIO_Port, RELAY_1_Pin,  Realtime.Relay1);
			HAL_GPIO_WritePin(RELAY_2_GPIO_Port, RELAY_2_Pin,  Realtime.Relay2);

			flag_telemetry = 10;
		}
		else if((strncmp(buf, "GET /SetRestoreFactory", 22) == 0)) {
			resp_http_200(conn);
			Carrega_Prog_Default();
		}
		else if((strncmp(buf, "GET /readTasks=", 15) == 0)) {
			DynWebPage();
			sprintf(buf_lvglmon, "used: %6ld (%3d %%), frag: %3d %%, biggest free: %6d \n", (int)mon.total_size - mon.free_size,
    				  	  	  	  	mon.used_pct,
									mon.frag_pct,
									(int)mon.free_biggest_size);
//			sprintf(buf_html, "%s TASKS:%s;LVGL:%s;CNT:%ld;FIM:", http_200_OK, PAGE_BODY, buf_lvglmon, cnt_reset_lvgl);
//			netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
			cJSON *root = cJSON_CreateObject();
			char *json_string = NULL;

			if (root == NULL) {
		         // Erro ao criar JSON - fallback para resposta simples
		         sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"JSON creation failed\"}", http_200_OK_JSON);
		         netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		         return;
		     }

		    // === Dados Tasks ===
			cJSON_AddStringToObject(root, "tasks", PAGE_BODY);
			cJSON_AddStringToObject(root, "lvgl", buf_lvglmon);
			cJSON_AddNumberToObject(root, "cnt", cnt_reset_lvgl);

			// Converter JSON para string
		    json_string = cJSON_PrintUnformatted(root);

		    if(json_string != NULL) {
		        // Montar resposta HTTP
		        int json_len = strlen(json_string);
		        int response_len = strlen(http_200_OK_JSON) + json_len + 1;
		        char *response = (char*)malloc(response_len);

		        if(response != NULL) {
		            snprintf(response, response_len, "%s%s", http_200_OK_JSON, json_string);
		            netconn_write(conn, response, strlen(response), NETCONN_COPY);
		            free(response);
		        } else {
		            // Fallback se malloc falhar
		            sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"memory allocation failed\"}", http_200_OK_JSON);
		            netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		        }
		        free(json_string);
		    } else {
		        // Fallback se cJSON_Print falhar
		        sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"JSON serialization failed\"}", http_200_OK_JSON);
		        netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		    }
		    cJSON_Delete(root);
		}
		else if((strncmp(buf, "GET /readToken=", 15) == 0)) {
			prepare_license();
//    	    sprintf(buf_html, "%s UUID:%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X;TOKEN:%c%c%c%c-%c%c%c%c;LIC:%c%c%c%c%c%c%c%c;SEQ:%ld;LICTMR:%ld;FLAGLIC:%d;FIM:",
//						http_200_OK,
//						cfg.uuid[0], cfg.uuid[1], cfg.uuid[2], cfg.uuid[3], cfg.uuid[4], cfg.uuid[5], cfg.uuid[6], cfg.uuid[7],
//						cfg.uuid[8], cfg.uuid[9], cfg.uuid[10], cfg.uuid[11], cfg.uuid[12], cfg.uuid[13], cfg.uuid[14], cfg.uuid[15],
//						cfg.Token[0], cfg.Token[1], cfg.Token[2], cfg.Token[3], cfg.Token[4], cfg.Token[5], cfg.Token[6], cfg.Token[7],
//						licsend[0], licsend[1], licsend[2], licsend[3], licsend[4], licsend[5], licsend[6], licsend[7],
//    	  				lic.LicSeq, lic.licenseTimer, 0);
//    	    netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
//		}
			cJSON *root = cJSON_CreateObject();
			char *json_string = NULL;

			if (root == NULL) {
				// Erro ao criar JSON - fallback para resposta simples
				sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"JSON creation failed\"}", http_200_OK_JSON);
				netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
				return;
			}

			// === Dados Token ===
			char str_uuid[128] = {0};
			sprintf(str_uuid, "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
					cfg.uuid[0], cfg.uuid[1], cfg.uuid[2], cfg.uuid[3], cfg.uuid[4], cfg.uuid[5], cfg.uuid[6], cfg.uuid[7],
					cfg.uuid[8], cfg.uuid[9], cfg.uuid[10], cfg.uuid[11], cfg.uuid[12], cfg.uuid[13], cfg.uuid[14], cfg.uuid[15]);

			cJSON_AddStringToObject(root, "uuid", str_uuid);

			char str_token[32] = {0};
			sprintf(str_token, "%c%c%c%c-%c%c%c%c", cfg.Token[0], cfg.Token[1], cfg.Token[2], cfg.Token[3],
					                                cfg.Token[4], cfg.Token[5], cfg.Token[6], cfg.Token[7]  );

			cJSON_AddStringToObject(root, "token", str_token);

			char str_lic[32] = {0};
			sprintf(str_lic,"%c%c%c%c%c%c%c%c", licsend[0], licsend[1], licsend[2], licsend[3],
					                            licsend[4], licsend[5], licsend[6], licsend[7] );
			cJSON_AddStringToObject(root, "lic", str_lic);
			cJSON_AddNumberToObject(root, "seq", lic.LicSeq);
			cJSON_AddNumberToObject(root, "timer", lic.licenseTimer);
			cJSON_AddNumberToObject(root, "flag", 0);

			// Converter JSON para string
			json_string = cJSON_PrintUnformatted(root);

			if(json_string != NULL) {
				// Montar resposta HTTP
				int json_len = strlen(json_string);
				int response_len = strlen(http_200_OK_JSON) + json_len + 1;
				char *response = (char*)malloc(response_len);

				if(response != NULL) {
					snprintf(response, response_len, "%s%s", http_200_OK_JSON, json_string);
					netconn_write(conn, response, strlen(response), NETCONN_COPY);
					free(response);
				} else {
					// Fallback se malloc falhar
					sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"memory allocation failed\"}", http_200_OK_JSON);
					netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
				}
				free(json_string);
			} else {
				// Fallback se cJSON_Print falhar
				sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"JSON serialization failed\"}", http_200_OK_JSON);
				netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
			}
			cJSON_Delete(root);
		}
		else if((strncmp(buf, "GET /readDebug=", 15) == 0)) {
#ifdef STM32H743xx
			bkSRAM_ReadVariable((0*4), &stacked_r0);
			bkSRAM_ReadVariable((1*4), &stacked_r1);
			bkSRAM_ReadVariable((2*4), &stacked_r2);
			bkSRAM_ReadVariable((3*4), &stacked_r3);

			bkSRAM_ReadVariable((4*4), &stacked_r12);
			bkSRAM_ReadVariable((5*4), &stacked_lr);
			bkSRAM_ReadVariable((6*4), &stacked_pc);
			bkSRAM_ReadVariable((7*4), &stacked_psr);

			bkSRAM_ReadVariable((8*4), &stacked_bfar);
			bkSRAM_ReadVariable((9*4), &stacked_cfsr);
			bkSRAM_ReadVariable((10*4), &stacked_hfsr);
			bkSRAM_ReadVariable((11*4), &stacked_dfsr);
			bkSRAM_ReadVariable((12*4), &stacked_afsr);
#else
			stacked_r0 = *(unsigned long *) (0x40024000 + (0));
			stacked_r1 = *(unsigned long *) (0x40024000 + (1*4));
			stacked_r2 = *(unsigned long *) (0x40024000 + (2*4));
			stacked_r3 = *(unsigned long *) (0x40024000 + (3*4));

			stacked_r12 = *(unsigned long *) (0x40024000 + (4*4));
			stacked_lr  = *(unsigned long *) (0x40024000 + (5*4));
			stacked_pc  = *(unsigned long *) (0x40024000 + (6*4));
			stacked_psr = *(unsigned long *) (0x40024000 + (7*4));

			stacked_bfar = *(unsigned long *) (0x40024000 + (8*4));
			stacked_cfsr = *(unsigned long *) (0x40024000 + (9*4));
			stacked_hfsr = *(unsigned long *) (0x40024000 + (10*4));
			stacked_dfsr = *(unsigned long *) (0x40024000 + (11*4));
			stacked_afsr = *(unsigned long *) (0x40024000 + (12*4));
#endif
//			sprintf(buf_html, "%s RESET:%s;R0:0x%08lX;R1:0x%08lX;R2:0x%08lX;R3:0x%08lX;R12:0x%08lX;LR:0x%08lX;PC:0x%08lX;PSR:0x%08lX;BFAR:0x%08lX;CFSR:0x%08lX;HFSR:0x%08lX;DFSR:0x%08lX;AFSR:0x%08lX;FIM:",
//						http_200_OK, reset_cause_get_name(reset_cause), stacked_r0, stacked_r1, stacked_r2, stacked_r3, stacked_r12,
//						stacked_lr, stacked_pc, stacked_psr, stacked_bfar, stacked_cfsr, stacked_hfsr, stacked_dfsr, stacked_afsr);
//			netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
    	    cJSON *root = cJSON_CreateObject();
			char *json_string = NULL;

			if (root == NULL) {
		         // Erro ao criar JSON - fallback para resposta simples
		         sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"JSON creation failed\"}", http_200_OK_JSON);
		         netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		         return;
		     }

		    // === Dados Debug ===
			cJSON_AddStringToObject(root, "reset", reset_cause_get_name(reset_cause));
			cJSON_AddNumberToObject(root, "r0", stacked_r0);
			cJSON_AddNumberToObject(root, "r1", stacked_r1);
			cJSON_AddNumberToObject(root, "r2", stacked_r2);
			cJSON_AddNumberToObject(root, "r3", stacked_r3);
			cJSON_AddNumberToObject(root, "r12", stacked_r12);
			cJSON_AddNumberToObject(root, "lr", stacked_lr);
			cJSON_AddNumberToObject(root, "pc", stacked_pc);
			cJSON_AddNumberToObject(root, "psr", stacked_psr);
			cJSON_AddNumberToObject(root, "bfar", stacked_bfar);
			cJSON_AddNumberToObject(root, "cfsr", stacked_cfsr);
			cJSON_AddNumberToObject(root, "hfsr", stacked_hfsr);
			cJSON_AddNumberToObject(root, "dfsr", stacked_dfsr);
			cJSON_AddNumberToObject(root, "afsr", stacked_afsr);

			// Converter JSON para string
		    json_string = cJSON_PrintUnformatted(root);

		    if(json_string != NULL) {
		        // Montar resposta HTTP
		        int json_len = strlen(json_string);
		        int response_len = strlen(http_200_OK_JSON) + json_len + 1;
		        char *response = (char*)malloc(response_len);

		        if(response != NULL) {
		            snprintf(response, response_len, "%s%s", http_200_OK_JSON, json_string);
		            netconn_write(conn, response, strlen(response), NETCONN_COPY);
		            free(response);
		        } else {
		            // Fallback se malloc falhar
		            sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"memory allocation failed\"}", http_200_OK_JSON);
		            netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		        }
		        free(json_string);
		    } else {
		        // Fallback se cJSON_Print falhar
		        sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"JSON serialization failed\"}", http_200_OK_JSON);
		        netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		    }
		    cJSON_Delete(root);
		}
    	// Comandos
		else if((strncmp(buf, "GET /readLogin", 14) == 0) ) {
			cJSON *root = cJSON_CreateObject();
			char *json_string = NULL;

			if (root == NULL) {
		         // Erro ao criar JSON - fallback para resposta simples
		         sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"JSON creation failed\"}", http_200_OK_JSON);
		         netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		         return;
		     }

		    // === Dados Básicos do Login ===
		    const char* login_user = "UNKNOWN";
		    if(http_access == 0) login_user = "";
		    else if(http_access == 1) login_user = "User";
		    else if(http_access == 2) login_user = "Admin";
		    cJSON_AddStringToObject(root, "User", login_user);
		    if(cfg.ConfigHold == 0)
		    	cJSON_AddNumberToObject(root, "Config", 0);
		    else
		    	cJSON_AddNumberToObject(root, "Config", 1);

		    // Converter JSON para string
		    json_string = cJSON_PrintUnformatted(root);

		    if(json_string != NULL) {
		        // Montar resposta HTTP
		        int json_len = strlen(json_string);
		        int response_len = strlen(http_200_OK_JSON) + json_len + 1;
		        char *response = (char*)malloc(response_len);

		        if(response != NULL) {
		            snprintf(response, response_len, "%s%s", http_200_OK_JSON, json_string);
		            netconn_write(conn, response, strlen(response), NETCONN_COPY);
		            free(response);
		        } else {
		            // Fallback se malloc falhar
		            sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"memory allocation failed\"}", http_200_OK_JSON);
		            netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		        }
		        free(json_string);
		    } else {
		        // Fallback se cJSON_Print falhar
		        sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"JSON serialization failed\"}", http_200_OK_JSON);
		        netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		    }
		    cJSON_Delete(root);
		}
		else if( strncmp(buf, "GET /readTELEMETRYJSON", 22) == 0 && (http_access != 0) ) {
			//send_json_response(conn, "success", "JSON processed successfully");
			json_telemetry(conn);
		}
		else if( strncmp(buf, "GET /readTELEMETRY", 18) == 0 && (http_access != 0) ) {
			cJSON *root = cJSON_CreateObject();
			char *json_string = NULL;

			if (root == NULL) {
		         // Erro ao criar JSON - fallback para resposta simples
		         sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"JSON creation failed\"}", http_200_OK_JSON);
		         netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		         return;
		     }

		    // === Dados Básicos do Sistema ===
		    cJSON_AddStringToObject(root, "model", "RXNSTL 900MHz");
		    cJSON_AddStringToObject(root, "versionmcu", versao);
		    cJSON_AddStringToObject(root, "versionmem", version_flash);

		    // === RDS ===
		    const char* rds_mode = "UNKNOWN";
		    rds_mode = (rds.enable == 0) ? "DISABLE" : "ENABLE";
		    cJSON_AddStringToObject(root, "rds", rds_mode);

		    // === Frequência ===
		    cJSON_AddNumberToObject(root, "frequency", cfg.Frequencia);

		    // === Status ===
		    cJSON_AddStringToObject(root, "status", "OK");
		    cJSON_AddStringToObject(root, "fail",   "OK");

		    // === Audio Source ===
		    const char* audio_mode = "UNKNOWN";
		    if(cfg.AudioSource == 0) audio_mode = "MPX1";
		    else if(cfg.AudioSource == 1) audio_mode = "MPX2";
		    else if(cfg.AudioSource == 2) audio_mode = "MPX3";
		    cJSON_AddStringToObject(root, "audiosource", audio_mode);

		    // === Battery ===
		    const char* bat_mode = "UNKNOWN";
		    if(Status_Battery) bat_mode = "ON";
		    else bat_mode = "OFF";
		    cJSON_AddStringToObject(root, "battery", bat_mode);

		    // === Uptime ===
		    char str_uptime[32] = {0};
			sprintf(str_uptime, "%ldd - %02ld:%02ld:%02ld", uptime.dia, (uptime.total/3600)%24, (uptime.total/60)%60, uptime.total%60);
			cJSON_AddStringToObject(root, "uptime", str_uptime);

		    // === Data/Hora ===
			char str_clock[32] = {0};
		    sprintf(str_clock, "%02d/%02d/%04d %02d:%02d:%02d", gDate.Date, gDate.Month, (2000+gDate.Year),
		    		                                 gTime.Hours, gTime.Minutes, gTime.Seconds);
		    cJSON_AddStringToObject(root, "clock", str_clock);

		    // === Profile ===
		    cJSON_AddStringToObject(root, "station", Profile.Station);
		    cJSON_AddStringToObject(root, "city", Profile.City);
		    cJSON_AddStringToObject(root, "state", Profile.State);
		    cJSON_AddStringToObject(root, "country", Profile.Country);
		    cJSON_AddStringToObject(root, "exttemp", Profile.Temp);

		    // === Telemetry ===
		    cJSON_AddNumberToObject(root, "rssisignal", adc_values[0]);
		    cJSON_AddNumberToObject(root, "rssilimit",  adc_values[1]);
		    cJSON_AddNumberToObject(root, "mpxpilot",   adc_values[2]);
		    cJSON_AddNumberToObject(root, "mpx57k",     adc_values[3]);
		    cJSON_AddNumberToObject(root, "mpx",      	adc_values[4]);
		    cJSON_AddNumberToObject(root, "mono",      	adc_values[5]);
		    cJSON_AddNumberToObject(root, "left",      	adc_values[6]);
		    cJSON_AddNumberToObject(root, "right",      adc_values[7]);
		    cJSON_AddBoolToObject(root,   "dem",        Status_FMDem);
		    cJSON_AddBoolToObject(root,   "stereo",     Status_Stereo);

		    // Converter JSON para string
		    json_string = cJSON_PrintUnformatted(root);

		    if(json_string != NULL) {
		        // Montar resposta HTTP
		        int json_len = strlen(json_string);
		        int response_len = strlen(http_200_OK_JSON) + json_len + 1;
		        char *response = (char*)malloc(response_len);

		        if(response != NULL) {
		            snprintf(response, response_len, "%s%s", http_200_OK_JSON, json_string);
		            netconn_write(conn, response, strlen(response), NETCONN_COPY);
		            free(response);
		        } else {
		            // Fallback se malloc falhar
		            sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"memory allocation failed\"}", http_200_OK_JSON);
		            netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		        }

		        free(json_string);
		    } else {
		        // Fallback se cJSON_Print falhar
		        sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"JSON serialization failed\"}", http_200_OK_JSON);
		        netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		    }

		    cJSON_Delete(root);
		}
		else if((strncmp(buf, "GET /RFState=1", 14) == 0) && (http_access == 2) ) {
			// Power-ON
			if(cfg.ConfigHold == 0 && !(falha & (1ULL << FAIL_LICENSE))) {
				resp_http_200(conn);
				flag_telemetry = 1;
        		RF_Enable();
        		timer_reflesh = 0;
			}
			else {
				resp_http_401(conn);
			}
		}
		else if((strncmp(buf, "GET /RFState=2", 14) == 0) && (http_access == 2) ) {
			// Power-OFF
			if(cfg.ConfigHold == 0) {
				resp_http_200(conn);
				flag_telemetry = 1;
        		// Desliga Falhas de SWR And REFLECTED
        		falha &= ~((uint64_t)1ULL << FAIL_SWR);
        		falha &= ~((uint64_t)1ULL << FAIL_RFL);
        		RF_Disable();
        		timer_reflesh = 0;
			}
			else {
				resp_http_401(conn);
			}
		}
		else if((strncmp(buf, "GET /GetGraph=", 14) == 0)) {
			uint8_t id_graph = buf[14] - '0';
			if(id_graph == 1) {
				sprintf(buf_html, "%s", http_200_OK);
				netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
			}
			else if(id_graph == 2) {
				sprintf(buf_html, "%s", http_200_OK);
				netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
			}
			else if(id_graph == 3) {
				sprintf(buf_html, "%s", http_200_OK);
				netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
			}
		}
		else if((strncmp(buf, "GET /readAUDIO", 14) == 0)) {
			cJSON *root = cJSON_CreateObject();
			char *json_string = NULL;

			if (root == NULL) {
		         // Erro ao criar JSON - fallback para resposta simples
		         sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"JSON creation failed\"}", http_200_OK_JSON);
		         netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		         return;
		     }

		    // === Dados Básicos do Sistema ===
			cJSON_AddBoolToObject(root, "emphasis", cfg.Emphase);
			cJSON_AddBoolToObject(root, "processor", cfg.Processor);
			cJSON_AddBoolToObject(root, "aes192", cfg.AES192);
			cJSON_AddBoolToObject(root, "dspcfg", Realtime.DSP_Cfg);
			cJSON_AddBoolToObject(root, "dsp1", Realtime.DSP_Bit_1);
			cJSON_AddBoolToObject(root, "dsp2", Realtime.DSP_Bit_2);
			//
			cJSON_AddNumberToObject(root, "dsppwm", Realtime.DSP_PWM);
			//
			cJSON_AddNumberToObject(root, "dspvol1",    cfg.Vol_MPX1);
			cJSON_AddNumberToObject(root, "dspvol2",    cfg.Vol_MPX2);
			cJSON_AddNumberToObject(root, "dspvolfone", cfg.Vol_HeadPhone);
			//
			cJSON_AddNumberToObject(root, "levelaudioon", cfg.level_audio_on);
			cJSON_AddNumberToObject(root, "timeraudioon", cfg.timer_audio_on/(1000*60));
			cJSON_AddNumberToObject(root, "levelaudiooff", cfg.level_audio_off);
			cJSON_AddNumberToObject(root, "timeraudiooff", cfg.timer_audio_off/(1000*60));

		    // Converter JSON para string
		    json_string = cJSON_PrintUnformatted(root);

		    if(json_string != NULL) {
		        // Montar resposta HTTP
		        int json_len = strlen(json_string);
		        int response_len = strlen(http_200_OK_JSON) + json_len + 1;
		        char *response = (char*)malloc(response_len);

		        if(response != NULL) {
		            snprintf(response, response_len, "%s%s", http_200_OK_JSON, json_string);
		            netconn_write(conn, response, strlen(response), NETCONN_COPY);
		            free(response);
		        } else {
		            // Fallback se malloc falhar
		            sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"memory allocation failed\"}", http_200_OK_JSON);
		            netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		        }
		        free(json_string);
		    } else {
		        // Fallback se cJSON_Print falhar
		        sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"JSON serialization failed\"}", http_200_OK_JSON);
		        netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		    }
		    cJSON_Delete(root);
		}
		else if((strncmp(buf, "GET /setAUDIOVOL=", 17) == 0)) {
			// MPX1 VOL
			resp_http_200(conn);
			strstr_substring(buf, "MPX1:", "MPX2:", 5);
			cfg.Vol_MPX1 = atoi(out);
			if(cfg.Vol_MPX1 >= 0 && cfg.Vol_MPX1 <= 255) {
				cfg.Vol_MPX1 = atoi(out);
			}
			else {
				cfg.Vol_MPX1 = 128;
			}
			// MPX2-VOL
			strstr_substring(buf, "MPX2:", "PHONE:", 5);
			cfg.Vol_MPX2 = atoi(out);
			if(cfg.Vol_MPX2 >= 0 && cfg.Vol_MPX2 <= 255) {
				cfg.Vol_MPX2 = atoi(out);
			}
			else {
				cfg.Vol_MPX2 = 128;
			}
			// Volume Head-Phone
			strstr_substring(buf, "PHONE:", "FIM", 6);
			cfg.Vol_HeadPhone = atoi(out);
			if(cfg.Vol_HeadPhone >= 0 && cfg.Vol_HeadPhone <= 255) {
				cfg.Vol_HeadPhone = atoi(out);
			}
			else {
				cfg.Vol_HeadPhone = 32;
			}
			// Atualiza Valores
			Write_AD5242(AD524X_RDAC0, cfg.Vol_MPX1, 0, 0);
			Write_AD5242(AD524X_RDAC1, cfg.Vol_MPX2, 0, 0);

			// Atualiza Volume HeadPhone
			tpa6130_set_volume(cfg.Vol_HeadPhone);

			flag_telemetry = 6;
		}
		else if((strncmp(buf, "GET /setALARMMPX=", 17) == 0)) {
			resp_http_200(conn);
			// MPX_VALUE
			strstr_substring(buf, "MPXVALUE:", "MPXTIMER:", 9);
			cfg.level_audio_on = atoi(out);
			if(cfg.level_audio_on >= 0 && cfg.level_audio_on <= 48) {
				cfg.level_audio_on = atoi(out);
			}
			else {
				cfg.level_audio_on = 0;
			}

			// MPX_TIMER
			strstr_substring(buf, "MPXTIMER:", "MPXVALUEOFF:", 9);
			cfg.timer_audio_on = atoi(out) * 1000 * 60;
			if(cfg.timer_audio_on >= 60000 && cfg.timer_audio_on <= 3600000) {
				cfg.timer_audio_on = atoi(out) * 1000 * 60;
			}
			else {
				cfg.timer_audio_on = 3 * 1000 * 60;
			}
			// MPX VALUE OFF
			strstr_substring(buf, "MPXVALUEOFF:", "MPXTIMEROFF:", 12);
			cfg.level_audio_off = atoi(out);
			if(cfg.level_audio_off >= 0 && cfg.level_audio_off <= 48) {
				cfg.level_audio_off = atoi(out);
			}
			else {
				cfg.level_audio_off = 0;
			}
			// MPX VALUE OFF
			strstr_substring(buf, "MPXTIMEROFF:", "FIM:", 12);
			cfg.timer_audio_off = atoi(out) * 1000 * 60;
			if(cfg.timer_audio_off >= 60000 && cfg.timer_audio_off <= 3600000) {
				cfg.timer_audio_off = atoi(out) * 1000 * 60;
			}
			else {
				cfg.timer_audio_off = 5 * 1000 * 60;
			}

			flag_telemetry = 33;
		}
		else if((strncmp(buf, "GET /setAUDIO=", 14) == 0)) {
			resp_http_200(conn);
			cfg.Emphase = buf[14] - '0';
			cfg.Processor = buf[15] - '0';
			cfg.AES192 = buf[16] - '0';
			Realtime.DSP_Cfg = buf[17] - '0';
			Realtime.DSP_Bit_1 = buf[18] - '0';
			Realtime.DSP_Bit_2 = buf[19] - '0';
			strstr_substring(buf, "DSPPWM:", "FIM", 7);
			Realtime.DSP_PWM = atoi(out);
			// Salva na EEPROM

			// Atualiza Valores
			UpdateValores();

			flag_telemetry = 2;
		}
		else if((strncmp(buf, "GET /readAdvSet", 15) == 0)) {
			cJSON *root = cJSON_CreateObject();
			char *json_string = NULL;

			if (root == NULL) {
		         // Erro ao criar JSON - fallback para resposta simples
		         sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"JSON creation failed\"}", http_200_OK_JSON);
		         netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		         return;
		     }

		    // === Dados Advanced Settings ===
			cJSON_AddNumberToObject(root, "rssi1", adv.GainRSSI1);
			cJSON_AddNumberToObject(root, "rssi2", adv.GainRSSI2);
			cJSON_AddNumberToObject(root, "mpx", adv.GainMPX);
			cJSON_AddNumberToObject(root, "left", adv.GainLeft);
			cJSON_AddNumberToObject(root, "right", adv.GainRight);

		    // Converter JSON para string
		    json_string = cJSON_PrintUnformatted(root);

		    if(json_string != NULL) {
		        // Montar resposta HTTP
		        int json_len = strlen(json_string);
		        int response_len = strlen(http_200_OK_JSON) + json_len + 1;
		        char *response = (char*)malloc(response_len);

		        if(response != NULL) {
		            snprintf(response, response_len, "%s%s", http_200_OK_JSON, json_string);
		            netconn_write(conn, response, strlen(response), NETCONN_COPY);
		            free(response);
		        } else {
		            // Fallback se malloc falhar
		            sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"memory allocation failed\"}", http_200_OK_JSON);
		            netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		        }
		        free(json_string);
		    } else {
		        // Fallback se cJSON_Print falhar
		        sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"JSON serialization failed\"}", http_200_OK_JSON);
		        netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		    }
		    cJSON_Delete(root);
		}
        else if((strncmp(buf, "GET /setAdvSet=", 15) == 0)) {
			resp_http_200(conn);
        	strstr_substring(buf, "RSSI1:", "RSSI2:", 6);
        	if(atof(out) > 0.09 && atof(out) <= 2.00) {
				adv.GainRSSI1 = atof(out);
        	}
        	else {
				adv.GainRSSI1 = 1.0f;
			}
        	strstr_substring(buf, "RSSI2:", "MPX:", 6);
        	if(atof(out) > 0.09 && atof(out) <= 2.00) {
				adv.GainRSSI2 = atof(out);
			}
			else {
				adv.GainRSSI2 = 1.0f;
        	}
        	strstr_substring(buf, "MPX:", "LEFT:", 4);
        	if(atof(out) > 0.09 && atof(out) <= 2.00) {
				adv.GainMPX = atof(out);
			}
			else {
				adv.GainMPX = 1.0f;
			}
			strstr_substring(buf, "LEFT:", "RIGHT:", 5);
			if(atof(out) > 0.09 && atof(out) <= 2.00) {
				adv.GainLeft = atof(out);
			}
			else {
				adv.GainLeft = 1.0f;
			}
			strstr_substring(buf, "RIGHT:", "FIM:", 6);
			if(atof(out) > 0.09 && atof(out) <= 2.00) {
				adv.GainRight = atof(out);
			}
			else {
				adv.GainRight = 1.0f;
			}
			flag_telemetry = 11;
		}
		else if((strncmp(buf, "GET /readMaxSN", 14) == 0)) {
			cJSON *root = cJSON_CreateObject();
			char *json_string = NULL;

			if (root == NULL) {
		         // Erro ao criar JSON - fallback para resposta simples
		         sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"JSON creation failed\"}", http_200_OK_JSON);
		         netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		         return;
		     }

		    // === Dados Serial Number (String) ===
		    char str_sn[32] = {0};
			sprintf(str_sn, "%d%d%d%d%d%d%d%d", cfg.SerialNumber[0], cfg.SerialNumber[1],
					                            cfg.SerialNumber[2], cfg.SerialNumber[3],
												cfg.SerialNumber[4], cfg.SerialNumber[5],
												cfg.SerialNumber[6], cfg.SerialNumber[7] );
			cJSON_AddStringToObject(root, "serialnumber", str_sn);

		    // Converter JSON para string
		    json_string = cJSON_PrintUnformatted(root);

		    if(json_string != NULL) {
		        // Montar resposta HTTP
		        int json_len = strlen(json_string);
		        int response_len = strlen(http_200_OK_JSON) + json_len + 1;
		        char *response = (char*)malloc(response_len);

		        if(response != NULL) {
		            snprintf(response, response_len, "%s%s", http_200_OK_JSON, json_string);
		            netconn_write(conn, response, strlen(response), NETCONN_COPY);
		            free(response);
		        } else {
		            // Fallback se malloc falhar
		            sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"memory allocation failed\"}", http_200_OK_JSON);
		            netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		        }
		        free(json_string);
		    } else {
		        // Fallback se cJSON_Print falhar
		        sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"JSON serialization failed\"}", http_200_OK_JSON);
		        netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		    }
		    cJSON_Delete(root);
		}
		else if((strncmp(buf, "GET /setService=", 16) == 0)) {
			resp_http_200(conn);
			if(buf[16] == '1') {
				cfg.servico = 2;
			}
			else {
				cfg.servico = 0;
			}
			flag_telemetry = 19;
		}
		else if((strncmp(buf, "GET /setPWM1=", 13) == 0)) {
			resp_http_200(conn);
			strstr_substring(buf, "PWM:", "FIM", 4);
			__HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_1, atoi(out));		// PWM_CH1 = 0   RF
		}
		else if((strncmp(buf, "GET /setPWM2=", 13) == 0)) {
			resp_http_200(conn);
			strstr_substring(buf, "PWM:", "FIM", 4);
			__HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_1, atoi(out));		// PWM_CH1 = 0   RF
		}
		else if((strncmp(buf, "GET /setPWMFAN=", 15) == 0)) {
			resp_http_200(conn);
			strstr_substring(buf, "PWM:", "FIM", 4);
			__HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_2, atoi(out));		// PWM_CH2 = 0  	FAN
		}
		else if((strncmp(buf, "GET /setSN=", 11) == 0)) {
			resp_http_200(conn);
			cfg.SerialNumber[0] = buf[14] - '0';
			cfg.SerialNumber[1] = buf[15] - '0';
			cfg.SerialNumber[2] = buf[16] - '0';
			cfg.SerialNumber[3] = buf[17] - '0';
			cfg.SerialNumber[4] = buf[18] - '0';
			cfg.SerialNumber[5] = buf[19] - '0';
			cfg.SerialNumber[6] = buf[20] - '0';
			cfg.SerialNumber[7] = buf[21] - '0';
			flag_telemetry = 17;
		}
		else if((strncmp(buf, "GET /readService", 16) == 0)) {
//			sprintf(str_sn, "%d%d%d%d%d%d%d%d", cfg.SerialNumber[0], cfg.SerialNumber[1],
//        			                            cfg.SerialNumber[2], cfg.SerialNumber[3],
//												cfg.SerialNumber[4], cfg.SerialNumber[5],
//												cfg.SerialNumber[6], cfg.SerialNumber[7]);
//			sprintf(buf_html, "%s SN:%s;PWM:%d;BAT:%d;FM:%d;STMO:%d;BW:%d;ATN:%0.1f;DSPSEL:%d;DSP1:%d;DSP2:%d;RST:%s;ADC0: 0x%X [ %d ] mV: %0.2f;ADC1: 0x%X [ %d ] mV: %0.2f;ADC2: 0x%X [ %d ] mV: %0.2f;ADC3: 0x%X [ %d ] mV: %0.2f;ADC4: 0x%X [ %d ] mV: %0.2f;ADC5: 0x%X [ %d ] mV: %0.2f;ADC6: 0x%X [ %d ] mV: %0.2f;ADC7: 0x%X [ %d ] mV: %0.2f;FIM",
//						http_200_OK, str_sn, Realtime.DSP_PWM, Status_Battery, Status_FMDem, Status_Stereo, cfg.BW, cfg.Atten, Realtime.DSP_Cfg, Realtime.DSP_Bit_1, Realtime.DSP_Bit_2,
//						reset_cause_get_name(reset_cause),
//						adc_values[0], adc_values[0], (float)((3000.0/65535)*adc_values[0]),
//						adc_values[1], adc_values[1], (float)((3000.0/65535)*adc_values[1]),
//						adc_values[2], adc_values[2], (float)((3000.0/65535)*adc_values[2]),
//						adc_values[3], adc_values[3], (float)((3000.0/65535)*adc_values[3]),
//						adc_values[4], adc_values[4], (float)((3000.0/65535)*adc_values[4]),
//						adc_values[5], adc_values[5], (float)((3000.0/65535)*adc_values[5]),
//						adc_values[6], adc_values[6], (float)((3000.0/65535)*adc_values[6]),
//						adc_values[7], adc_values[7], (float)((3000.0/65535)*adc_values[7]) );
//
//			netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
//
			cJSON *root = cJSON_CreateObject();
			char *json_string = NULL;

			if (root == NULL) {
		         // Erro ao criar JSON - fallback para resposta simples
		         sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"JSON creation failed\"}", http_200_OK_JSON);
		         netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		         return;
		     }

		    // === Dados Service ===
		    char str_sn[32] = {0};
			sprintf(str_sn, "%d%d%d%d%d%d%d%d", cfg.SerialNumber[0], cfg.SerialNumber[1],
					                            cfg.SerialNumber[2], cfg.SerialNumber[3],
												cfg.SerialNumber[4], cfg.SerialNumber[5],
												cfg.SerialNumber[6], cfg.SerialNumber[7] );
			cJSON_AddStringToObject(root, "serialnumber", str_sn);
			cJSON_AddStringToObject(root, "reset", reset_cause_get_name(reset_cause));
			cJSON_AddNumberToObject(root, "pwm", Realtime.DSP_PWM);
			cJSON_AddBoolToObject(root, "battery", Status_Battery);
			cJSON_AddBoolToObject(root, "demulador", Status_FMDem);
			cJSON_AddBoolToObject(root, "stereo", Status_Stereo);
			cJSON_AddBoolToObject(root, "band", cfg.BW);
			cJSON_AddNumberToObject(root, "atennuador", cfg.Atten);
			cJSON_AddNumberToObject(root, "dsp_cfg", Realtime.DSP_Cfg);
			cJSON_AddNumberToObject(root, "dspbit1", Realtime.DSP_Bit_1);
			cJSON_AddNumberToObject(root, "dspbit2", Realtime.DSP_Bit_2);
			cJSON_AddNumberToObject(root, "adc0", adc_values[0]);
			cJSON_AddNumberToObject(root, "adc1", adc_values[1]);
			cJSON_AddNumberToObject(root, "adc2", adc_values[2]);
			cJSON_AddNumberToObject(root, "adc3", adc_values[3]);
			cJSON_AddNumberToObject(root, "adc4", adc_values[4]);
			cJSON_AddNumberToObject(root, "adc5", adc_values[5]);
			cJSON_AddNumberToObject(root, "adc6", adc_values[6]);
			cJSON_AddNumberToObject(root, "adc7", adc_values[7]);

		    // Converter JSON para string
		    json_string = cJSON_PrintUnformatted(root);

		    if(json_string != NULL) {
		        // Montar resposta HTTP
		        int json_len = strlen(json_string);
		        int response_len = strlen(http_200_OK_JSON) + json_len + 1;
		        char *response = (char*)malloc(response_len);

		        if(response != NULL) {
		            snprintf(response, response_len, "%s%s", http_200_OK_JSON, json_string);
		            netconn_write(conn, response, strlen(response), NETCONN_COPY);
		            free(response);
		        } else {
		            // Fallback se malloc falhar
		            sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"memory allocation failed\"}", http_200_OK_JSON);
		            netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		        }
		        free(json_string);
		    } else {
		        // Fallback se cJSON_Print falhar
		        sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"JSON serialization failed\"}", http_200_OK_JSON);
		        netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		    }
		    cJSON_Delete(root);
		}
		else if((strncmp(buf, "GET /readRF", 11) == 0)) {
			cJSON *root = cJSON_CreateObject();
			char *json_string = NULL;

			if (root == NULL) {
		         // Erro ao criar JSON - fallback para resposta simples
		         sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"JSON creation failed\"}", http_200_OK_JSON);
		         netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		         return;
		     }

		    // === Dados RF ===
			cJSON_AddBoolToObject(root, "band", cfg.BW);
			cJSON_AddNumberToObject(root, "attenuation", cfg.Atten);

		    // Converter JSON para string
		    json_string = cJSON_PrintUnformatted(root);

		    if(json_string != NULL) {
		        // Montar resposta HTTP
		        int json_len = strlen(json_string);
		        int response_len = strlen(http_200_OK_JSON) + json_len + 1;
		        char *response = (char*)malloc(response_len);

		        if(response != NULL) {
		            snprintf(response, response_len, "%s%s", http_200_OK_JSON, json_string);
		            netconn_write(conn, response, strlen(response), NETCONN_COPY);
		            free(response);
		        } else {
		            // Fallback se malloc falhar
		            sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"memory allocation failed\"}", http_200_OK_JSON);
		            netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		        }
		        free(json_string);
		    } else {
		        // Fallback se cJSON_Print falhar
		        sprintf(buf_html, "%s {\"status\":\"error\",\"message\":\"JSON serialization failed\"}", http_200_OK_JSON);
		        netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		    }
		    cJSON_Delete(root);
		}
		else if((strncmp(buf, "GET /setRF=", 11) == 0)) {
			resp_http_200(conn);
			cfg.BW = buf[11] - '0';
			// Attenuation
			strstr_substring(buf, "ATTN:", "FIM", 5);
			float float_atn = atof(out);

			if(float_atn >= 0.0f && float_atn <= 31.75f) {
				cfg.Atten = float_atn;
			}
			else {
				cfg.Atten = 0.0f;
			}
			// Atualiza Estado
			if(cfg.BW) {
				HAL_GPIO_WritePin(BW_SEL_GPIO_Port, BW_SEL_Pin, GPIO_PIN_SET);
			}
			else {
				HAL_GPIO_WritePin(BW_SEL_GPIO_Port, BW_SEL_Pin, GPIO_PIN_RESET);
			}
			//
			PE43711(cfg.Atten);
		}
		else if((strncmp(buf, "GET /MP3-PREV", 13) == 0) || (strncmp(buf, "GET /MP3-PLAY", 13) == 0) ||
          		(strncmp(buf, "GET /MP3-NEXT", 13) == 0) || (strncmp(buf, "GET /MP3-STOP", 13) == 0) ) {
			//
          	fs_open(&file, "/mp3.html");
          	netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
          	fs_close(&file);

          	if( (strncmp(buf, "GET /MP3-PREV", 13) == 0) ) {
				// Botao MP3-PREV
			}
            if( (strncmp(buf, "GET /MP3-PLAY", 13) == 0) ) {
				// Botao MP3-PLAY
			}
			if( (strncmp(buf, "GET /MP3-NEXT", 13) == 0) ) {
				// Botao MP3-NEXT
			}
			if( (strncmp(buf, "GET /MP3-STOP", 13) == 0) ) {
				// Botao MP3-STOP
			}
		}
		else if((strncmp(buf, "GET /readMP3", 12) == 0)) {
			sprintf(buf_html, "%s MUS:%s;MP3:%d;FIM\n", http_200_OK, "", 0);
			netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
		}
		//
		else {
			/* Load Error page */
			fs_open(&file, "/404.html");
			netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
			fs_close(&file);
		}
	}
	// Short delay to ensure data is sent
	osDelay(10);
}

static void process_http_connection(struct netconn *conn)
{
    struct netbuf *inbuf = NULL;
    char *buf = NULL;
    u16_t buflen;
    err_t recv_err;

    /* Short timeouts */
    netconn_set_recvtimeout(conn, 3000);  // 3s receive timeout

    /* Process ONE request only */
    recv_err = netconn_recv(conn, &inbuf);

    if (recv_err == ERR_OK && inbuf != NULL) {
        netbuf_data(inbuf, (void**)&buf, &buflen);

        if (buf && buflen > 0) {
            /* Take semaphore briefly */
            if (xSemaphoreTake(MutexHTTPDHandle, pdMS_TO_TICKS(100)) == pdTRUE) {
                handle_http_request(conn, buf, buflen);
                xSemaphoreGive(MutexHTTPDHandle);
            }
        }

        netbuf_delete(inbuf);
    }

    /* Force TCP connection close */
    netconn_disconnect(conn);
}

static void http_server_netconn_thread(void *arg)
{
    struct netconn *conn = NULL, *newconn = NULL;
    err_t err = ERR_OK;

    while(1) {
        /* Create and setup listener */
        conn = netconn_new(NETCONN_TCP);
        if (!conn) {
            osDelay(1000);
            continue;
        }

        /* Non-blocking accept */
        netconn_set_nonblocking(conn, 0);

        err = netconn_bind(conn, NULL, cfg.PortWEB);
        if (err != ERR_OK) {
            netconn_delete(conn);
            osDelay(1000);
            continue;
        }

        netconn_listen(conn);
        //logI("HTTP Server listening on port %d\n", cfg.PortWEB);

        /* Accept loop with timeout */
        while(1) {
            newconn = NULL;
            err = netconn_accept(conn, &newconn);

            if (err != ERR_OK) {
                if (err != ERR_TIMEOUT) {
                    //logI("Accept error: %d, recreating listener\n", err);
                    break;
                }
                /* Timeout is normal - check resources and continue */
                print_memp_stats();
                continue;
            }

            if (!newconn) {
                continue;
            }

            netconn_set_recvtimeout(newconn, HTTP_SOCKET_TIMEOUT_MS);

//            if (osSemaphoreAcquire(BinarySemHTTPDHandle, osWaitForever) == osOK) {
            	/* Process this connection */
            	process_http_connection(newconn);
                /* Liberar semáforo */
//                osSemaphoreRelease(BinarySemHTTPDHandle);
//            }

            /* Immediate cleanup */
            netconn_close(newconn);
            netconn_delete(newconn);
            newconn = NULL;

            /* Prevent flooding */
            osDelay(1);
        }

        /* Cleanup listener */
        if (conn) {
            netconn_close(conn);
            netconn_delete(conn);
            conn = NULL;
        }

        //logI("Listener cleanup complete, recreating...\n");
        osDelay(100);
    }
}

void http_server_netconn_init(void)
{
	ThreadHTTPDPHandle = sys_thread_new("HTTP", http_server_netconn_thread, NULL, 8192, WEBSERVER_THREAD_PRIO);
}

void print_memp_stats(void)
{
	//logI("=== Detailed TCP Stats ===\n");
	//logI("TCP_PCB: %d/%d used\n", lwip_stats.memp[MEMP_TCP_PCB]->used, lwip_stats.memp[MEMP_TCP_PCB]->max);
	//logI("TCP_PCB_LISTEN: %d/%d used\n", lwip_stats.memp[MEMP_TCP_PCB_LISTEN]->used, lwip_stats.memp[MEMP_TCP_PCB_LISTEN]->max);
	//logI("NETCONN pool: %d used, %d max ever\n",lwip_stats.memp[MEMP_NETCONN]->used, lwip_stats.memp[MEMP_NETCONN]->max);
	//logI("PBUF pool: %d used\n", lwip_stats.memp[MEMP_PBUF]->used);
#if LWIP_STATS
	//logI("TCP Connections: %ld\n", lwip_stats.tcp.);
	//logI("TCP Closed: %ld\n", lwip_stats.tcp.);
	//logI("TCP Drop: %ld\n", lwip_stats.tcp.drop);
	//logI("TCP Mem Err: %ld\n", lwip_stats.tcp.memerr);
	//logI("MEM Avail.: %ld/%ld used\n", lwip_stats.mem.avail, lwip_stats.mem.used);
#endif
	//logI("FreeRTOS Heap: %d free\n", xPortGetFreeHeapSize());
	//logI("MQTT State: %d/%d\n", mqttClient.isconnected, Telemetry_State);

	//logI("===========================\n");
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
