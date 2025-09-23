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
#include "../Sinteck/src/defines.h"
#include "../Sinteck/src/PowerControl.h"
#include "../Sinteck/src/audio.h"
#include "../Sinteck/src/pwm.h"
#include "../Sinteck/tcp/ssl_client.h"
#include "../Sinteck/tcp/tcp_client.h"
#include "../Sinteck/tcp/json_util.h"
#include "../cJSON/tiny-json.h"

#define WEBSERVER_THREAD_PRIO    ( osPriorityLow5 )

extern osMutexId_t MutexHTTPDHandle;

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

struct netbuf *inbuf;
static char* buf;
static err_t recv_err;
static uint16_t buflen;
struct fs_file file;

extern uint8_t flag_telemetry;
extern uint32_t timer_reflesh;
extern const char* versao;
extern char version_flash[];
extern uint8_t Status_Battery;

const static char http_200_OK[] = "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n";
//const static char http_200_OK_JSON[] = "HTTP/1.1 200 OK\r\nContent-type: application/json\r\n\r\n";
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

extern volatile uint16_t adc_values[];
extern uint16_t adc_ext[];

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


static void http_server_serve(struct netconn *conn)
{
   uint8_t x;

  /* Read the data from the port, blocking if nothing yet there.
     We assume the request (the part we care about) is in one netbuf */
  recv_err = netconn_recv(conn, &inbuf);

  if (recv_err == ERR_OK) {
    if (netconn_err(conn) == ERR_OK) {
      netbuf_data(inbuf, (void**)&buf, &buflen);

      /* Is this an HTTP GET command? (only check the first 5 chars, since
         there are other formats for GET, and we're keeping it very simple )*/
      if ((buflen >=5) && ((strncmp(buf, "GET /", 5) == 0) || (strncmp(buf, "POST /", 6) == 0) || (strncmp(buf, "HEAD /", 6) == 0))) {
    	  if (strncmp((char const *)buf,"HEAD /robots.txt", 16) == 0) {
    		  /* Load Error page */
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
    	  /* Check if request to get ST.gif */
    	  else if (strncmp((char const *)buf,"GET /STM32H7xx_files/ST.gif", 27)==0)
    	  {
    		  fs_open(&file, "/STM32H7xx_files/ST.gif");
    		  netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
    		  fs_close(&file);
    	  }
    	  /* Check if request to get stm32.jpg */
    	  else if (strncmp((char const *)buf,"GET /STM32H7xx_files/stm32.jpg",30)==0)
    	  {
    		  fs_open(&file, "/STM32H7xx_files/stm32.jpg");
    		  netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
    		  fs_close(&file);
    	  }
    	  else if (strncmp((char const *)buf,"GET /STM32H7xx_files/logo.jpg", 29) == 0)
    	  {
    		  /* Check if request to get ST logo.jpg */
    		  fs_open(&file, "/STM32H7xx_files/logo.jpg");
    		  netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
    		  fs_close(&file);
    	  }
    	  else if (strncmp((char const *)buf,"GET /css/style.css", 18) == 0)
    	  {
    		  /* Check if request to get CSS Style */
    		  fs_open(&file, "/css/style.css");
    		  netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
    		  fs_close(&file);
    	  }
    	  else if (strncmp((char const *)buf,"GET /js/utils.js", 16) == 0)
    	  {
    		  /* Check if request to get Javacript */
    		  fs_open(&file, "/js/utils.js");
    		  netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
    		  fs_close(&file);
    	  }
    	  else if (strncmp((char const *)buf,"GET /STM32H7xx_files/favicon.ico", 29) == 0)
    	  {
    		  /* Check if request to get ST logo.jpg */
    		  fs_open(&file, "/STM32H7xx_files/favicon.ico");
    		  netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
    		  fs_close(&file);
    	  }
    	  else if (strncmp((char const *)buf,"GET /STM32H7xx_files/sics.gif", 29) == 0)
    	  {
    		  /* Check if request to get ST logo.jpg */
    		  fs_open(&file, "/STM32H7xx_files/sics.gif");
    		  netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
    		  fs_close(&file);
    	  }
    	  else if(( (strncmp(buf, "GET /STM32H7xx.html", 19) == 0) || (strncmp(buf, "GET /index.html", 15) == 0) ) && (http_access != 0) )
    	  {
    		  /* Load STM32H7xx page */
    		  fs_open(&file, "/STM32H7xx.html");
    		  netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
    		  fs_close(&file);
    	  }
    	  else if((strncmp(buf, "GET /audio.html", 15) == 0) && (http_access == 2)) {
    		  /* Load Audio page */
    		  fs_open(&file, "/audio.html");
    		  netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
    		  fs_close(&file);
    	  }
    	  else if((strncmp(buf, "GET /time.html", 14) == 0) && (http_access == 2)) {
    		  /* Load Time page */
    		  fs_open(&file, "/time.html");
    		  netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
    		  fs_close(&file);
    	  }
    	  else if((strncmp(buf, "GET /rds.html", 13) == 0) && (http_access == 2)) {
    		  /* Load Time page */
    		  fs_open(&file, "/rds.html");
    		  netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
    		  fs_close(&file);
    	  }
    	  else if((strncmp(buf, "GET /freq.html", 14) == 0) && (http_access == 2)) {
    		  /* Load Time page */
              fs_open(&file, "/freq.html");
              netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
              fs_close(&file);
    	  }
    	  else if((strncmp(buf, "GET /advset.html", 16) == 0) && (http_access == 2)) {
    		  /* Load Time page */
    		  fs_open(&file, "/advset.html");
    		  netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
    		  fs_close(&file);
    	  }
    	  else if((strncmp(buf, "GET /alerts.html", 16) == 0) && (http_access == 2)) {
    		  /* Load Time page */
    		  fs_open(&file, "/alerts.html");
    		  netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
    		  fs_close(&file);
    	  }
    	  else if((strncmp(buf, "GET /network.html", 17) == 0) && (http_access == 2)) {
    		  /* Load Time page */
    		  fs_open(&file, "/network.html");
    		  netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
    		  fs_close(&file);
    	  }
    	  else if((strncmp(buf, "GET /password.html", 18) == 0) && (http_access == 2)) {
    		  /* Load Time page */
    		  fs_open(&file, "/password.html");
    		  netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
    		  fs_close(&file);
    	  }
    	  else if((strncmp(buf, "GET /confighold.html", 20) == 0) && (http_access == 2)) {
    		  /* Load Time page */
    		  fs_open(&file, "/confighold.html");
    		  netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
    		  fs_close(&file);
    	  }
    	  else if((strncmp(buf, "GET /login.html", 15) == 0)) {
    		  /* Load Time page */
    		  fs_open(&file, "/login.html");
    		  netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
    		  fs_close(&file);
    	  }
          else if((strncmp(buf, "GET /profile.html", 17) == 0)) {
          	/* Load Time page */
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
    		  /* Load Reboot page */
    		  fs_open(&file, "/reboot.html");
    		  netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
    		  fs_close(&file);
    		  RF_Disable();
    		  osDelay(1000);
    		  HAL_NVIC_SystemReset();
    	  }
    	  else if((strncmp(buf, "GET /ClearLicense.html", 22) == 0)) {
			  /* Load ClearLicense page */
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
             /* Load dynamic page */
          	fs_open(&file, "/tasks.html");
          	netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
          	fs_close(&file);
          }
          else if(strncmp(buf, "GET /Debug.html", 15) == 0) {
          	/* Load Debug page */
          	fs_open(&file, "/Debug.html");
          	netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
          	fs_close(&file);
          }
          else if((strncmp(buf, "GET /Token.html", 15) == 0) && (http_access == 2)) {
          	/* Load Time page */
              fs_open(&file, "/Token.html");
              netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
              fs_close(&file);
          }
          else if((strncmp(buf, "GET /service.html", 17) == 0) && (http_access == 2)) {
          	/* Load Time page */
          	fs_open(&file, "/service.html");
          	netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
              fs_close(&file);
          }
    	  else if((strncmp(buf, "GET /Forward_Graph.html", 23) == 0)) {
    		  /* Load Time page */
    		  fs_open(&file, "/Forward_Graph.html");
    		  netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
    		  fs_close(&file);
    	  }
    	  else if((strncmp(buf, "GET /Reflected_Graph.html", 25) == 0)) {
    		  /* Load Time page */
    		  fs_open(&file, "/Reflected_Graph.html");
    		  netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
    		  fs_close(&file);
    	  }
    	  else if((strncmp(buf, "GET /Temperature_Graph.html", 27) == 0)) {
    		  /* Load Time page */
    		  fs_open(&file, "/Temperature_Graph.html");
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
    			  /* provide contiguous storage if p is a chained pbuf */
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
    				  /* user and password are correct, create a "session" */
    				  /* Load STM32H7xx page */
    				  timer_http_access = 0;
    				  http_access = 2;
    				  fs_open(&file, "/STM32H7xx.html");
    				  netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
    				  fs_close(&file);
    			  }
    			  else if (!strcmp(user, "user") && !strcmp(pass, buf_user_pass)) {
    				  /* user and password are correct, create a "session" */
    				  /* Load STM32H7xx page */
    				  timer_http_access = 0;
    				  http_access = 1;
    				  fs_open(&file, "/STM32H7xx.html");
    				  netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
    				  fs_close(&file);
    			  }
    			  else {
    				  /* Load Time page */
    				  timer_http_access = 0;
    				  http_access = 0;
    				  fs_open(&file, "/loginfail.html");
    				  netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
    				  fs_close(&file);
    			  }
    		  }
    		  else {
    			  /* Load Error page */
    	          fs_open(&file, "/404.html");
    	          netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
    	          fs_close(&file);
    		  }
    	  }
    	  else if((strncmp(buf, "GET /readTime", 13) == 0)) {
    		  // Dia-Semana-Dia-Mes-Ano-HH:MM:SS
    		  sprintf(buf_html, "%s DAY:%d;DATE:%02d/%02d/%04d;TIME:%02d:%02d:%02d;NTP:%d;FUSO:%d;FIM:\n",
    	  					http_200_OK, gDate.WeekDay, gDate.Date, gDate.Month, 2000+gDate.Year, gTime.Hours, gTime.Minutes, gTime.Seconds,
    	  					cfg.NTP, cfg.Timezone);

    		  netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
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
    		  sprintf(buf_html, "%s FREQ:%ld;FIM:", http_200_OK, cfg.Frequencia);
    		  netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
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
    	      if(atoi(rasc_freq) >= 937500 && atoi(rasc_freq) <= 960000)
    	    	  cfg.Frequencia = (long int)atoi(rasc_freq);
    	      // Desliga RF Power
    	      __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_1, 0);
    	      flag_telemetry = 7;
    	  }
    	  else if((strncmp(buf, "GET /resetpassword=yes", 22) == 0)) {
    		  sprintf(buf_html, "OK");
    		  netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
    		  flag_telemetry = 253;
    	  }
    	  else if((strncmp(buf, "GET /readNetwork", 16) == 0)) {
    		  sprintf(buf_html,"%s IP:%3d.%3d.%3d.%3d;MASK:%3d.%3d.%3d.%3d;GW:%3d.%3d.%3d.%3d;DNS:%3d.%3d.%3d.%3d;PORT:%d;BROKER:%s;SNMP:%d;FIM:",
      				http_200_OK,
      				cfg.IP_ADDR[0],   cfg.IP_ADDR[1],   cfg.IP_ADDR[2],   cfg.IP_ADDR[3],
  					cfg.MASK_ADDR[0], cfg.MASK_ADDR[1], cfg.MASK_ADDR[2], cfg.MASK_ADDR[3],
  					cfg.GW_ADDR[0],   cfg.GW_ADDR[1],   cfg.GW_ADDR[2],   cfg.GW_ADDR[3],
  					cfg.DNS_ADDR[0],  cfg.DNS_ADDR[1],  cfg.DNS_ADDR[2],  cfg.DNS_ADDR[3], cfg.PortWEB,
  					"1.1.1.1", cfg.EnableSNMP );
    		  netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
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
    		  sprintf(buf_html,"%s PASS:%d%d%d%d;USER:%d%d%d%d;FIM:", http_200_OK,
    				            cfg.PassAdmin[0], cfg.PassAdmin[1], cfg.PassAdmin[2], cfg.PassAdmin[3],
    	      				    cfg.PassUser[0],  cfg.PassUser[1],  cfg.PassUser[2],  cfg.PassUser[3]);
    	      netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
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
    		  sprintf(buf_html, "%s STATION:%s;CITY:%s;STATE:%s;COUNTRY:%s;TEMP:%s;FIM:\n", http_200_OK,
    	          			    Profile.Station, Profile.City, Profile.State, Profile.Country, Profile.Temp  );
    	      netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
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
    		  sprintf(buf_html, "%s HOLD:%d;VSWR:%d;CLOCK:%d;REF:%0.0f;VALUE:%0.0f;FWDNULL1:%0.0f;FIM:", http_200_OK,
    				             cfg.ConfigHold, cfg.VSWR_Null, 0, Realtime.Reflected, cfg.VSWR_Null_Value, cfg.FWD_Null_Value);
    	      netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
    	  }
    	  else if((strncmp(buf, "GET /setConfigHold=", 19) == 0)) {
    		  resp_http_200(conn);
    		  cfg.ConfigHold = buf[19] - '0';
    	      //OSC10MCfg = buf[21] - '0';
    		  cfg.VSWR_Null = buf[23] - '0';
    	      strstr_substring(buf, "VSWR:", "NFWD:", 5);
    	      cfg.VSWR_Null_Value = atoi(out);
    	      if(cfg.VSWR_Null_Value >=0 && cfg.VSWR_Null_Value <= 10) {
    	    	  cfg.VSWR_Null_Value = atoi(out);
    	      }
    	      else {
    	    	  cfg.VSWR_Null_Value = 0;
    	      }
    	      strstr_substring(buf, "NFWD:", "FIM:", 5);
    	      cfg.FWD_Null_Value = atoi(out);
    	      if(cfg.FWD_Null_Value >=0 && cfg.FWD_Null_Value <= 30) {
    	    	  cfg.FWD_Null_Value = atoi(out);
    	      }
    	      else {
    	    	  cfg.FWD_Null_Value = 0;
    	      }
    	      flag_telemetry = 10;
    	  }
    	  else if((strncmp(buf, "GET /readVSWR", 13) == 0)) {
    		  sprintf(buf_html, "%s REF:%0.1fW;VALUE:%0.1fW;FWDNULL:%0.1fW;FIM:", http_200_OK,
    				             Realtime.Reflected, cfg.VSWR_Null_Value, cfg.FWD_Null_Value);
    		  netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
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
    		  sprintf(buf_html, "%s TASKS:%s;LVGL:%s;CNT:%ld;FIM:", http_200_OK, PAGE_BODY, buf_lvglmon, cnt_reset_lvgl);
    		  netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
    	  }
    	  else if((strncmp(buf, "GET /readToken=", 15) == 0)) {
    	      		prepare_license();
    	      		sprintf(buf_html, "%s UUID:%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X;TOKEN:%c%c%c%c-%c%c%c%c;LIC:%c%c%c%c%c%c%c%c;SEQ:%ld;LICTMR:%ld;FLAGLIC:%d;FIM:",
    	      	    		http_200_OK,
							cfg.uuid[0], cfg.uuid[1], cfg.uuid[2], cfg.uuid[3], cfg.uuid[4], cfg.uuid[5], cfg.uuid[6], cfg.uuid[7],
							cfg.uuid[8], cfg.uuid[9], cfg.uuid[10], cfg.uuid[11], cfg.uuid[12], cfg.uuid[13], cfg.uuid[14], cfg.uuid[15],
							cfg.Token[0], cfg.Token[1], cfg.Token[2], cfg.Token[3], cfg.Token[4], cfg.Token[5], cfg.Token[6], cfg.Token[7],
							licsend[0], licsend[1], licsend[2], licsend[3], licsend[4], licsend[5], licsend[6], licsend[7],
    	  					lic.LicSeq, lic.licenseTimer, 0);
    	      	    netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
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
    		  sprintf(buf_html, "%s RESET:%s;R0:0x%08lX;R1:0x%08lX;R2:0x%08lX;R3:0x%08lX;R12:0x%08lX;LR:0x%08lX;PC:0x%08lX;PSR:0x%08lX;BFAR:0x%08lX;CFSR:0x%08lX;HFSR:0x%08lX;DFSR:0x%08lX;AFSR:0x%08lX;FIM:",
      				   http_200_OK, reset_cause_get_name(reset_cause), stacked_r0, stacked_r1, stacked_r2, stacked_r3, stacked_r12,
      				   stacked_lr, stacked_pc, stacked_psr, stacked_bfar, stacked_cfsr, stacked_hfsr, stacked_dfsr, stacked_afsr);
    		  netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
      	}

    	  // Comandos
          else if((strncmp(buf, "GET /readLogin", 14) == 0) ) {
          	if(http_access == 0) {
          		sprintf(buf_html, "%s User:%s;Config:%d;FIM:\n", http_200_OK, "", cfg.ConfigHold);
          	}
          	else if(http_access == 1) {
          		sprintf(buf_html, "%s User:%s;Config:%d;FIM:\n", http_200_OK, "User", cfg.ConfigHold);
          	}
          	else {
          		sprintf(buf_html, "%s User:%s;Config:%d;FIM:\n", http_200_OK, "Admin", cfg.ConfigHold);
          	}
          	netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
          }
          else if((strncmp(buf, "GET /readProfile", 16) == 0) ) {
          	sprintf(buf_html, "%s STATION:%s;CITY:%s;STATE:%s;COUNTRY:%s;TEMP:%s;FIM:\n", http_200_OK,
          			           Profile.Station, Profile.City, Profile.State, Profile.Country, Profile.Temp  );
          	netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
          }
          else if( strncmp(buf, "GET /readTELEMETRY", 12) == 0 && (http_access != 0) ) {
        	  sprintf(str_ver, "PLL: %s", versao);
        	  sprintf(str_ver3, "MEM: %s", version_flash);
        	  sprintf(str_uptime, "%ldd - %02ld:%02ld:%02ld", uptime.dia, (uptime.total/3600)%24, (uptime.total/60)%60, uptime.total%60);
              memset(str_rds, 0, 20);
    		  if(rds.enable) {
    			  sprintf(str_rds, "ON");
    		  }
    		  else {
    			  sprintf(str_rds, "OFF");
    		  }
    		  //
    		  if(cfg.RFEnable) {
    			  sprintf(str_sts, "ON");
    		  }
    		  else {
    			  sprintf(str_sts, "OFF");
    		  }

    		  memset(str_freq, 0, 100);
    		  sprintf(str_freq, "%ld", cfg.Frequencia);
    		  sprintf(str_freq, "%d%d%d.%d%d%d MHz", str_freq[0]-'0', str_freq[1]-'0', str_freq[2]-'0', str_freq[3]-'0', str_freq[4]-'0', 0);

    		  memset(str_source, 0, 16);
    		  sprintf(str_source, "MPX%d", cfg.AudioSource);

    		  memset(str_bat, 0, 16);
    		  if(Status_Battery == 1) {
    			  sprintf(str_bat, "OFF");
    		  }
    		  else {
    			  sprintf(str_bat, "ON");
    		  }

        	  memset(buf_html, 0, 2500);
		      sprintf(buf_html, "%s FWD:%0.0f W [ %0.2f ];REF:%0.0f W [ VSWR: %0.2f:1  Load_Mismatch: %0.2f%% Return Loss: %0.2fdB ];TEMP:%2.1f &#8451;VPA:%0.1f V;IPA:%0.1fA;UPTIME:%s;EFIC:%s;TIPO:%d;EXTTEMP:%s;MODEL:%s;RDS:%s;FREQ:%s;SOURCE:%s;VERMCU:%s;VERMEM:%s;STS:%s;FAIL:%s;BAT:%s;CLOCK:%02d/%02d/%04d %02d:%02d:%02d;B1:%d;B2:%d;B3:%d;FIM\n",
		    		  http_200_OK, Get_Forward(), 3000.00f, Realtime.Reflected, Realtime.SWR, Realtime.Load_MisMatch, Realtime.Return_Loss, Realtime.Temperature, Realtime.VPA, Realtime.IPA, str_uptime, "100.0%", 0, "25.0°C", "RXNSTL 900MHz", str_rds, str_freq, str_source, str_ver, str_ver3, str_sts, "OK", str_bat, gDate.Date, gDate.Month, 2000+gDate.Year, gTime.Hours, gTime.Minutes, gTime.Seconds, 5, 10, 18);
		      netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
          }
          else if((strncmp(buf, "GET /RFState=1", 14) == 0) && (http_access == 2) ) {
        	  // Power-ON
        	  if(cfg.ConfigHold == 0 && !(falha & (1ULL << FAIL_LICENSE))) {
        		  resp_http_200(conn);
        		  cfg.RFEnable = 1;
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
        		  cfg.RFEnable = 0;
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
//        		  sprintf(buf_html, "%s H0:%0.0f;H1:%0.0f;H2:%0.0f;H3:%0.0f;H4:%0.0f;H5:%0.0f;H6:%0.0f;H7:%0.0f;H8:%0.0f;H9:%0.0f;H10:%0.0f;H11:%0.0f;H12:%0.0f;H13:%0.0f;H14:%0.0f;H15:%0.0f;H16:%0.0f;H17:%0.0f;H18:%0.0f;H19:%0.0f;H20:%0.0f;H21:%0.0f;H22:%0.0f;H23:%0.0f;FIM:",
//      				    http_200_OK,
//  					    gf_fwd.hora[0], gf_fwd.hora[1], gf_fwd.hora[2], gf_fwd.hora[3], gf_fwd.hora[4], gf_fwd.hora[5], gf_fwd.hora[6],
//  					    gf_fwd.hora[7], gf_fwd.hora[8], gf_fwd.hora[9], gf_fwd.hora[10],gf_fwd.hora[11], gf_fwd.hora[12], gf_fwd.hora[13],
//  					    gf_fwd.hora[14], gf_fwd.hora[15], gf_fwd.hora[16], gf_fwd.hora[17], gf_fwd.hora[18], gf_fwd.hora[19], gf_fwd.hora[20],
//  					    gf_fwd.hora[21],gf_fwd.hora[22],gf_fwd.hora[23] );

        		  sprintf(buf_html, "%s", http_200_OK);
        		  netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
        	  }
        	  else if(id_graph == 2) {
//        		  sprintf(buf_html, "%s H0:%0.0f;H1:%0.0f;H2:%0.0f;H3:%0.0f;H4:%0.0f;H5:%0.0f;H6:%0.0f;H7:%0.0f;H8:%0.0f;H9:%0.0f;H10:%0.0f;H11:%0.0f;H12:%0.0f;H13:%0.0f;H14:%0.0f;H15:%0.0f;H16:%0.0f;H17:%0.0f;H18:%0.0f;H19:%0.0f;H20:%0.0f;H21:%0.0f;H22:%0.0f;H23:%0.0f;"
//      					             "M0:%0.0f;M1:%0.0f;M2:%0.0f;M3:%0.0f;M4:%0.0f;M5:%0.0f;M6:%0.0f;M7:%0.0f;M8:%0.0f;M9:%0.0f;M10:%0.0f;M11:%0.0f;M12:%0.0f;M13:%0.0f;M14:%0.0f;M15:%0.0f;M16:%0.0f;M17:%0.0f;M18:%0.0f;M19:%0.0f;M20:%0.0f;M21:%0.0f;M22:%0.0f;M23:%0.0f;FIM:",
//      				    http_200_OK,
//  					    gf_ref.hora[0], gf_ref.hora[1], gf_ref.hora[2], gf_ref.hora[3], gf_ref.hora[4], gf_ref.hora[5], gf_ref.hora[6],
//  					    gf_ref.hora[7], gf_ref.hora[8], gf_ref.hora[9], gf_ref.hora[10],gf_ref.hora[11], gf_ref.hora[12], gf_ref.hora[13],
//  					    gf_ref.hora[14], gf_ref.hora[15], gf_ref.hora[16], gf_ref.hora[17], gf_ref.hora[18], gf_ref.hora[19], gf_ref.hora[20],
//  					    gf_ref.hora[21],gf_ref.hora[22],gf_ref.hora[23],
//  						//
//  						max_ref.hora[0], max_ref.hora[1], max_ref.hora[2], max_ref.hora[3], max_ref.hora[4], max_ref.hora[5], max_ref.hora[6],
//  						max_ref.hora[7], max_ref.hora[8], max_ref.hora[9], max_ref.hora[10],max_ref.hora[11], max_ref.hora[12], max_ref.hora[13],
//  						max_ref.hora[14], max_ref.hora[15], max_ref.hora[16], max_ref.hora[17], max_ref.hora[18], max_ref.hora[19], max_ref.hora[20],
//  						max_ref.hora[21],max_ref.hora[22],max_ref.hora[23] );

        		  sprintf(buf_html, "%s", http_200_OK);
        		  netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);

        	  }
        	  else if(id_graph == 3) {
//        		  sprintf(buf_html, "%s H0:%0.0f;H1:%0.0f;H2:%0.0f;H3:%0.0f;H4:%0.0f;H5:%0.0f;H6:%0.0f;H7:%0.0f;H8:%0.0f;H9:%0.0f;H10:%0.0f;H11:%0.0f;H12:%0.0f;H13:%0.0f;H14:%0.0f;H15:%0.0f;H16:%0.0f;H17:%0.0f;H18:%0.0f;H19:%0.0f;H20:%0.0f;H21:%0.0f;H22:%0.0f;H23:%0.0f;"
//      					             "M0:%0.0f;M1:%0.0f;M2:%0.0f;M3:%0.0f;M4:%0.0f;M5:%0.0f;M6:%0.0f;M7:%0.0f;M8:%0.0f;M9:%0.0f;M10:%0.0f;M11:%0.0f;M12:%0.0f;M13:%0.0f;M14:%0.0f;M15:%0.0f;M16:%0.0f;M17:%0.0f;M18:%0.0f;M19:%0.0f;M20:%0.0f;M21:%0.0f;M22:%0.0f;M23:%0.0f; FIM:",
//      				    http_200_OK,
//  					    gf_temp.hora[0], gf_temp.hora[1], gf_temp.hora[2], gf_temp.hora[3], gf_temp.hora[4], gf_temp.hora[5], gf_temp.hora[6],
//  					    gf_temp.hora[7], gf_temp.hora[8], gf_temp.hora[9], gf_temp.hora[10],gf_temp.hora[11], gf_temp.hora[12], gf_temp.hora[13],
//  					    gf_temp.hora[14], gf_temp.hora[15], gf_temp.hora[16], gf_temp.hora[17], gf_temp.hora[18], gf_temp.hora[19], gf_temp.hora[20],
//  					    gf_temp.hora[21],gf_temp.hora[22],gf_temp.hora[23],
//  						//
//  						max_temp.hora[0], max_temp.hora[1], max_temp.hora[2], max_temp.hora[3], max_temp.hora[4], max_temp.hora[5], max_temp.hora[6],
//  						max_temp.hora[7], max_temp.hora[8], max_temp.hora[9], max_temp.hora[10], max_temp.hora[11], max_temp.hora[12], max_temp.hora[13],
//  						max_temp.hora[14], max_temp.hora[15], max_temp.hora[16], max_temp.hora[17], max_temp.hora[18], max_temp.hora[19], max_temp.hora[20],
//  						max_temp.hora[21], max_temp.hora[22], max_temp.hora[23] );

        		  sprintf(buf_html, "%s", http_200_OK);
        		  netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
      			}
          }
          else if((strncmp(buf, "GET /readAUDIO", 14) == 0)) {
        	  sprintf(buf_html, "%s MPX:%d;STEREO:%d;EMP:%d;PROC:%d;TOS:%d;AES:%d;IMP:%d;FIM\n",
                  			     http_200_OK, cfg.AudioSource, cfg.MonoStereo, cfg.Emphase, cfg.Processor, cfg.Toslink, cfg.AES192, cfg.Imp_600_10K);
        	  netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
          }
          else if((strncmp(buf, "GET /readAudioVol", 17) == 0)) {
        	  sprintf(buf_html, "%s MPX1:%d;MPX2:%d;MPX3:%d;SCA:%d;LEFT:%d;RIGHT:%d;FIM\n", http_200_OK,
        			  cfg.Vol_MPX1, cfg.Vol_MPX2, cfg.Vol_MPX3, cfg.Vol_SCA, cfg.Vol_Left, cfg.Vol_Right);
        	  netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
          }
          else if((strncmp(buf, "GET /readAlarmMPX", 17) == 0)) {
        	  sprintf(buf_html, "%s MPXVALUE:%d;MPXTIMER:%ld;MPXVALUEOFF:%d;MPXTIMEROFF:%ld;FIM\n", http_200_OK, cfg.level_audio_on, cfg.timer_audio_on/(1000*60), cfg.level_audio_off, cfg.timer_audio_off/(1000*60));
        	  netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
          }
          else if((strncmp(buf, "GET /setAUDIOVOL=", 17) == 0)) {
        	  // MPX1 VOL
        	  resp_http_200(conn);
        	  strstr_substring(buf, "MPX1:", "MPX2:", 5);
        	  cfg.Vol_MPX1 = atoi(out);
        	  if(cfg.Vol_MPX1 >= 0 && cfg.Vol_MPX1 <= 60) {
        		  cfg.Vol_MPX1 = atoi(out);
        	  }
        	  else {
        		  cfg.Vol_MPX1 = 48;
        	  }
        	  // MPX2-VOL
        	  strstr_substring(buf, "MPX2:", "MPX3:", 5);
        	  cfg.Vol_MPX2 = atoi(out);
        	  if(cfg.Vol_MPX2 >= 0 && cfg.Vol_MPX2 <= 60) {
        		  cfg.Vol_MPX2 = atoi(out);
        	  }
        	  else {
        		  cfg.Vol_MPX2 = 48;
        	  }
        	  // MPX3-VOL
        	  strstr_substring(buf, "MPX3:", "SCA:", 5);
        	  cfg.Vol_MPX3 = atoi(out);
        	  if(cfg.Vol_MPX3 >= 0 && cfg.Vol_MPX3 <= 60) {
        		  cfg.Vol_MPX3 = atoi(out);
        	  }
        	  else {
        		  cfg.Vol_MPX3 = 48;
        	  }
        	  // SCA-VOL
        	  strstr_substring(buf, "SCA:", "LEFT:", 4);
        	  cfg.Vol_SCA = atoi(out);
        	  if(cfg.Vol_SCA >= 0 && cfg.Vol_SCA <= 60) {
        		  cfg.Vol_SCA = atoi(out);
        	  }
        	  else {
        		  cfg.Vol_SCA = 48;
        	  }
        	  // LEFT-VOL
        	  strstr_substring(buf, "LEFT:", "RIGHT:", 5);
        	  cfg.Vol_Left = atoi(out);
        	  if(cfg.Vol_Left >= 0 && cfg.Vol_Left <= 60) {
        		  cfg.Vol_Left = atoi(out);
        	  }
        	  else {
        		  cfg.Vol_Left = 48;
        	  }
        	  // RIGHT-VOL
        	  strstr_substring(buf, "RIGHT:", "FIM", 6);
        	  cfg.Vol_Right = atoi(out);
        	  if(cfg.Vol_Right >= 0 && cfg.Vol_Right <= 60) {
        		  cfg.Vol_Right = atoi(out);
        	  }
        	  else {
        		  cfg.Vol_Right = 48;
        	  }
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
        	  cfg.AudioSource = buf[14] - '0';
        	  cfg.MonoStereo =  buf[15] - '0';
        	  cfg.Emphase = buf[16] - '0';
        	  cfg.Processor = buf[17] - '0';
        	  cfg.Toslink = buf[18] - '0';
        	  cfg.AES192 = buf[19] - '0';
        	  cfg.Imp_600_10K = buf[20] - '0';
              // Salva na EEPROM
              flag_telemetry = 2;
          }
          else if((strncmp(buf, "GET /setRDS=", 12) == 0)) {
        	  sprintf(buf_html, "%s OK", http_200_OK);
        	  netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);

        	  rds.enable =  buf[12] - '0';
              rds.enable_station = buf[12] - '0';
              rds.enable_text = buf[12] - '0';
              rds.type = buf[14] - '0';
              rds.ct = buf[16] - '0';

              // clear
              for(x = 0; x < 10; x++){
                  rds.ps[x] = ' ';
                  rds.ptyn[x] = ' ';
              }
              rds.ps[8] = 0;
              rds.ptyn[8] = 0;

              strstr_substring(buf, "STATION:", "PI:", 8);
              decode_string(out, out1);
              for(x = 0; x < strlen(out1); x++) {
            	  rds.ps[x] = out1[x];
              }
              rds.ps[x] = 0;

              for(x = 0; x < 72; x++) {
            	  rds.rt1[x] = 0;
            	  rds.dps1[x] = 0;
              }
              rds.rt1[71] = 0;
              rds.dps1[71] = 0;

              //PI
              strstr_substring(buf, "PI:", "PTY:", 3);
              decode_string(out, out1);
              rds.pi[0] = out1[0];
              rds.pi[1] = out1[1];
              rds.pi[2] = out1[2];
              rds.pi[3] = out1[3];

              // PTY
              strstr_substring(buf, "PTY:", "PTYN:", 4);
              decode_string(out, out1);
              rds.pty = out1[0] - '0';

              // PTYN
              strstr_substring(buf, "PTYN:", "MS:", 5);
              decode_string(out, out1);
              for(x = 0; x < strlen(out1); x++) {
            	  rds.ptyn[x] = out1[x];
              }
              rds.ptyn[x] = 0;

              // Music / Speech
              strstr_substring(buf, "MS:", "AF1:", 3);
              decode_string(out, out1);
              rds.ms = out1[0] - '0';
              // AF1
              strstr_substring(buf, "AF1:", "AF2:", 4);
              decode_string(out, out1);
              val = atoi(out1);
              rds.af[0] = val;
              // AF2
              strstr_substring(buf, "AF2:", "AF3:", 4);
              decode_string(out, out1);
              val = atoi(out1);
              rds.af[1] = val;
              // AF3
              strstr_substring(buf, "AF3:", "AF4:", 4);
              decode_string(out, out1);
              val = atoi(out1);
              rds.af[2] = val;
              // AF4
              strstr_substring(buf, "AF4:", "AF5:", 4);
              decode_string(out, out1);
              val = atoi(out1);
              rds.af[3] = val;
              // AF5
              strstr_substring(buf, "AF5:", "TESTE:", 4);
              decode_string(out, out1);
              val = atoi(out1);
              rds.af[4] = val;
              // RT1
              strstr_substring(buf, "TEXT:", "DPS1:", 5);
              decode_string(out, out1);
              for(x = 0; x < strlen(out1); x++) {
            	  rds.rt1[x] = out1[x];
              }
              rds.rt1[x] = 0;

              // DPS1
              strstr_substring(buf, "DPS1:", "REMOTE:", 5);
              decode_string(out, out1);
              for(x = 0; x < strlen(out1); x++) {
            	  rds.dps1[x] = out1[x];
              }
              rds.dps1[x] = 0;

              // UDP
              strstr_substring(buf, "REMOTE:", "PORT_UDP:", 7);
              decode_string(out, out1);
              cfg.RDSRemote = out1[0] - '0';
              // Port
              strstr_substring(buf, "PORT_UDP:", "FIM:", 9);
              decode_string(out, out1);
              cfg.RDSUDPPort = atoi(out1);

              flag_telemetry = 5;
          }
          else if((strncmp(buf, "GET /readRDS", 12) == 0) ) {
        	  if( (falha & (1ULL << FAIL_SYNCRDS)) ) {
        		  sprintf(str_rds, "No");
        	  }
        	  else {
        		  sprintf(str_rds, "Yes");
        	  }
        	  sprintf(buf_html, "%s ENABLE:%d;CT:%d;RDS:%d;STATION:%s;PI:%C%C%C%C;PTY:%d;PTYN:%s;MS:%d;AF1:%d;AF2:%d;AF3:%d;AF4:%d;AF5:%d;TEXT:%s;DPS1:%s;VER:%s;SYNC:%s;REMOTE:%d;PORT_UDP:%d;FIM:\n",
                  			http_200_OK,
                  			rds.enable, rds.ct, rds.type,	rds.ps, rds.pi[0], rds.pi[1], rds.pi[2], rds.pi[3], rds.pty,
          					rds.ptyn, rds.ms, rds.af[0], rds.af[1], rds.af[2], rds.af[3], rds.af[4],
          					rds.rt1, rds.dps1, rds.version, str_rds, cfg.RDSRemote, cfg.RDSUDPPort);
        	  netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
          }
          else if((strncmp(buf, "GET /readRDSSYNC", 16) == 0)) {
        	  //
        	  if( (falha & (1ULL << FAIL_SYNCRDS)) ) {
        		  sprintf(str_rds, "Sync: No");
        	  }
        	  else {
        		  sprintf(str_rds, "Sync: Yes");
        	  }
        	  sprintf(buf_html, "%s RDSSYNC:%s;%s;FIM\n", http_200_OK, str_rds, rds.version);
        	  netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
          }
          else if((strncmp(buf, "GET /readAdvSet", 15) == 0)) {
        	  sprintf(buf_html, "%s MODEL:%d;ADV:%d;MAXIPA:%0.1f;MAXSWR:%0.0f;MINAC:%0.0f;MAXTEMP:%0.0f;GFWD:%1.2f;GREF:%1.2f;GIPA:%1.2f;GVPA:%1.2f;GTEMP:%1.2f;FIM:",
              				     http_200_OK, 0, 0, adv.MaxIpa, adv.MaxVswr, 0.0f, adv.MaxTemp, adv.GainFWD, adv.GainSWR, adv.GainIPA, adv.GainVPA, adv.GainTemp);
        	  netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
          }
          else if((strncmp(buf, "GET /setAdvSet=", 15) == 0)) {
        	  resp_http_200(conn);
        	  strstr_substring(buf, "EN:", "MAXIPA:", 3);

        	  float float_f1 = atof(out);
        	  if(float_f1 < 5.0f) {
        		  adv.MaxIpa = 5.0f;
        	  }
        	  else if(float_f1 > 3.0){
        		  adv.MaxIpa = 5.0;
        	  }
        	  else {
        		  adv.MaxIpa = float_f1;
        	  }

        	  strstr_substring(buf, "MAXSWR:", "MINAC:", 7);
        	  float_f1 = atof(out);
        	  if(float_f1 < 15.0f) {
        		  adv.MaxVswr = 15.0f;
        	  }
        	  else if(float_f1 > 30.0){
        		  adv.MaxVswr = 30.0;
        	  }
        	  else {
        		  adv.MaxVswr = float_f1;
        	  }

        	  strstr_substring(buf, "MAXTEMP:", "GFWD:", 8);
        	  float_f1 = atof(out);
        	  if(float_f1 < 30.0f) {
        		  adv.MaxTemp = 30.0f;
        	  }
        	  else if(float_f1 > 60.0f){
        		  adv.MaxTemp = 60.0f;
        	  }
        	  else {
        		  adv.MaxTemp = float_f1;
        	  }

        	  strstr_substring(buf, "GFWD:", "GREF:", 5);
        	  if(atof(out) > 0.09 && atof(out) <= 2.00) {
        		  adv.GainFWD = atof(out);
        	  }
        	  else {
        		  adv.GainFWD = 1.0f;
        	  }
        	  strstr_substring(buf, "GREF:", "GIPA:", 5);
        	  if(atof(out) > 0.09 && atof(out) <= 2.00) {
        		  adv.GainSWR = atof(out);
        	  }
        	  else {
        		  adv.GainSWR = 1.0f;
        	  }
        	  strstr_substring(buf, "GIPA:", "GVPA:", 5);
        	  if(atof(out) > 0.09 && atof(out) <= 2.00) {
        		  adv.GainIPA = atof(out);
        	  }
        	  else {
        		  adv.GainIPA = 1.0f;
        	  }
        	  strstr_substring(buf, "GVPA:", "GTEMP:", 5);
        	  if(atof(out) > 0.09 && atof(out) <= 2.00) {
        		  adv.GainVPA = atof(out);
        	  }
        	  else {
        		  adv.GainVPA = 1.0f;
        	  }
        	  strstr_substring(buf, "GTEMP:", "GDRV:", 6);
        	  if(atof(out) > 0.09 && atof(out) <= 2.00) {
        		  adv.GainTemp = atof(out);
        	  }
        	  else {
        		  adv.GainTemp = 1.0f;
        	  }
        	  strstr_substring(buf, "GDRV:", "GAC:", 5);
        	  if(atof(out) > 0.09 && atof(out) <= 2.00) {
        		  adv.GainTemp = atof(out);
        	  }
        	  else {
        		  adv.GainTemp = 1.0f;
              }

        	  flag_telemetry = 11;
          }
          else if((strncmp(buf, "GET /readMaxSN", 14) == 0)) {
        	  sprintf(str_sn, "%d%d%d%d%d%d%d%d", cfg.SerialNumber[0], cfg.SerialNumber[1],
        			                              cfg.SerialNumber[2], cfg.SerialNumber[3],
												  cfg.SerialNumber[4], cfg.SerialNumber[5],
												  cfg.SerialNumber[6], cfg.SerialNumber[7]);

        	  sprintf(buf_html, "%s SN:%s;FIM", http_200_OK, str_sn);
        	  netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
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
        	  sprintf(str_sn, "%d%d%d%d%d%d%d%d", cfg.SerialNumber[0], cfg.SerialNumber[1],
        			                              cfg.SerialNumber[2], cfg.SerialNumber[3],
												  cfg.SerialNumber[4], cfg.SerialNumber[5],
												  cfg.SerialNumber[6], cfg.SerialNumber[7]);
        	  sprintf(buf_html, "%s SN:%s;PWMD:%d;PWM1:%d;PWM2:%d;FWD:%0.1f;REF:%0.1f;VPA:%0.1f;IPA:%0.1f;TEMP:%0.1f;RST:%s;ADC0: 0x%X [ %d ] mV: %0.2f;ADC1: 0x%X [ %d ] mV: %0.2f;ADC2: 0x%X [ %d ] mV: %0.2f;ADC3: 0x%X [ %d ] mV: %0.2f;ADC4: 0x%X [ %d ] mV: %0.2f;CH0: 0x%X [ %d ] mV: %0.2f;CH1: 0x%X [ %d ] mV: %0.2f;CH2: 0x%X [ %d ] mV: %0.2f;CH3: 0x%X [ %d ] mV: %0.2f;CH4: 0x%X [ %d ] mV: %0.2f;CH5: 0x%X [ %d ] mV: %0.2f;CH6: 0x%X [ %d ] mV: %0.2f;CH7: 0x%X [ %d ] mV: %0.2f;FIM",
        			  http_200_OK, str_sn, 0, Realtime.pwm_bias, Realtime.pwm_fan, Realtime.Forward, Realtime.Reflected, Realtime.VPA, Realtime.IPA, Realtime.Temperature, reset_cause_get_name(reset_cause),
					  adc_values[0], adc_values[0], (float)((3000.0/65535)*adc_values[0]),
					  adc_values[1], adc_values[1], (float)((3000.0/65535)*adc_values[1]),
					  adc_values[2], adc_values[2], (float)((3000.0/65535)*adc_values[2]),
					  adc_values[3], adc_values[3], (float)((3000.0/65535)*adc_values[3]),
					  adc_values[4], adc_values[4], (float)((3000.0/65535)*adc_values[4]),
					  adc_ext[0], adc_ext[1], (float)((3000.0/4095.0)*adc_ext[1]),
					  adc_ext[1], adc_ext[2], (float)((3000.0/4095.0)*adc_ext[2]),
					  adc_ext[2], adc_ext[3], (float)((3000.0/4095.0)*adc_ext[3]),
					  adc_ext[3], adc_ext[4], (float)((3000.0/4095.0)*adc_ext[4]),
					  adc_ext[4], adc_ext[5], (float)((3000.0/4095.0)*adc_ext[5]),
					  adc_ext[5], adc_ext[6], (float)((3000.0/4095.0)*adc_ext[6]),
        	          adc_ext[6], adc_ext[7], (float)((3000.0/4095.0)*adc_ext[7]),
					  adc_ext[7], adc_ext[8], (float)((3000.0/4095.0)*adc_ext[8]) );

        	  netconn_write(conn, buf_html, strlen(buf_html), NETCONN_COPY);
          }
    	  //
    	  else
    	  {
    		  /* Load Error page */
    		  fs_open(&file, "/404.html");
    		  netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
    		  fs_close(&file);
    	  }
      }
    }
  }
  //
  /* Close the connection (server closes in HTTP) */
  netconn_close(conn);

  /* Delete the buffer (netconn_recv gives us ownership,
   	 so we have to make sure to deallocate the buffer) */
  netbuf_delete(inbuf);
}


/**
  * @brief  http server thread
  * @param arg: pointer on argument(not used here)
  * @retval None
  */
static void http_server_netconn_thread(void *arg)
{
	struct netconn *conn = NULL, *newconn = NULL;
	err_t err = ERR_OK, accept_err = ERR_OK;

	http_client = 0;
//	if(flag_lvgl == 1) {
//http_restart:
		/* Create a new TCP connection handle */
		conn = netconn_new(NETCONN_TCP);

		if (conn != NULL) {
			/* Bind to port 80 (HTTP) with default IP address */
			err = netconn_bind(conn, NULL, cfg.PortWEB);

			if (err == ERR_OK) {
				/* Put the connection into LISTEN state */
				netconn_listen(conn);

				while(1){
					/* accept any icoming connection */
					accept_err = netconn_accept(conn, &newconn);
					if(accept_err == ERR_OK && http_client == 0) {
						http_client++;
						// Reset Timer LogOut
						timer_http_access = 0;
						/* Set Timeout 10s. */
						netconn_set_recvtimeout(newconn, 10000);
						// Task Semaphore
						xSemaphoreTake(MutexHTTPDHandle, portMAX_DELAY);
						/* */
						httpd_error = 0;

						/* serve connection */
						http_server_serve(newconn);
						// Task Semaphore
						xSemaphoreGive(MutexHTTPDHandle);
						/* */
						if(http_client > 0) http_client--;
						/* delete connection */
						netconn_delete(newconn);
					}
					else {
						httpd_error = 255;
						netconn_close(newconn);
						netconn_delete(newconn);
						//goto http_restart;
					}
					// Desloga HTTP 2 Min
					if( timer_http_access >= (1000 * 60 * 2) ) {
						timer_http_access = 0;
						http_access = 0;
					}
					//
					osDelay(10);
				}
			}
		}
}

void http_server_netconn_init(void)
{
	ThreadHTTPDPHandle = sys_thread_new("HTTP", http_server_netconn_thread, NULL, 8192, WEBSERVER_THREAD_PRIO);
}


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
