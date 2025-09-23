/**
  ******************************************************************************
  * @file    LwIP/LwIP_HTTP_Server_Raw/Src/httpd_cg_ssi.c
  * @author  MCD Application Team
  * @brief   Webserver SSI and CGI handlers
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics International N.V. 
  * All rights reserved.</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "lwip/debug.h"
#include "lwip/tcp.h"
#include "lwip/apps/httpd.h"
#include "http_cgi_ssi.h"


#include <string.h>
#include <stdlib.h>

#include "../Sinteck/src/eeprom.h"
#include "../Sinteck/src/defines.h"

extern volatile float forward, reflected, rfin;
extern uint8_t RFEnable;
extern uint16_t PresetAtivo;

tSSIHandler ADC_Page_SSI_Handler;
uint32_t ADC_not_configured=1;

/* we will use character "t" as tag for CGI */
char const* TAGCHAR="t";
char const** TAGS=&TAGCHAR;

/* CGI handler for LED control */ 
const char * LEDS_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
u16_t ADC_Handler(int iIndex, char *pcInsert, int iInsertLen);
u16_t ADC_Reflected(int iIndex, char *pcInsert, int iInsertLen);
void httpd_ssi_init(void);
void httpd_cgi_init(void);

/* Html request for "/leds.cgi" will start LEDS_CGI_Handler */
const tCGI LEDS_CGI={"/leds.cgi", LEDS_CGI_Handler};

/* Cgi call table, only one CGI used */
tCGI CGI_TAB[1];

/**
  * @brief  ADC_Handler : SSI handler for ADC page 
  */
u16_t ADC_Handler(int iIndex, char *pcInsert, int iInsertLen)
{
  /* We have only one SSI handler iIndex = 0 */
  if (iIndex ==0)
  {  
    char Digit1=0, Digit2=0, Digit3=0, Digit4=0; 
    uint32_t ADCVal = 0;        

     /* convert to Voltage,  step = 0.8 mV */
     ADCVal = (uint32_t)(ADCVal * 0.8);
     
     /* get digits to display */
     ADCVal = (uint32_t)forward;

     Digit1= ADCVal/1000;
     Digit2= (ADCVal-(Digit1*1000))/100;
     Digit3= (ADCVal-((Digit1*1000)+(Digit2*100)))/10;
     Digit4= ADCVal -((Digit1*1000)+(Digit2*100)+ (Digit3*10));

     /* prepare data to be inserted in html */
     *pcInsert       = (char)(Digit1+0x30);
     *(pcInsert + 1) = (char)(Digit2+0x30);
     *(pcInsert + 2) = (char)(Digit3+0x30);
     *(pcInsert + 3) = (char)(Digit4+0x30);

    /* 4 characters need to be inserted in html*/
    return 4;
  }
  return 0;
}


/**
  * @brief  ADC_Handler : SSI handler for ADC page
  */
u16_t ADC_Reflected(int iIndex, char *pcInsert, int iInsertLen)
{
  /* We have only one SSI handler iIndex = 0 */
  if (iIndex == 1)
  {
    char Digit1=0, Digit2=0, Digit3=0, Digit4=0;
    uint32_t ADCVal = 0;

     /* convert to Voltage,  step = 0.8 mV */
     ADCVal = (uint32_t)(ADCVal * 0.8);

     /* get digits to display */
     ADCVal = (uint32_t)forward;
     
     Digit1= ADCVal/1000;
     Digit2= (ADCVal-(Digit1*1000))/100;
     Digit3= (ADCVal-((Digit1*1000)+(Digit2*100)))/10;
     Digit4= ADCVal -((Digit1*1000)+(Digit2*100)+ (Digit3*10));
        
     /* prepare data to be inserted in html */
     *pcInsert       = (char)(Digit1+0x30);
     *(pcInsert + 1) = (char)(Digit2+0x30);
     *(pcInsert + 2) = (char)(Digit3+0x30);
     *(pcInsert + 3) = (char)(Digit4+0x30);
    
    /* 4 characters need to be inserted in html*/
    return 4;
  }
  return 0;
}

/**
  * @brief  CGI handler for LEDs control 
  */
const char * LEDS_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
  uint32_t i=0;
  
  /* We have only one SSI handler iIndex = 0 */
  if (iIndex==0)
  {
    /* All LEDs off */
    
    /* Check cgi parameter : example GET /leds.cgi?led=2&led=4 */
    for (i=0; i<iNumParams; i++)
    {
      /* check parameter "led" */
      if (strcmp(pcParam[i] , "led")==0)   
      {
        /* Switch LED1 ON if 1 */
        if(strcmp(pcValue[i], "1") ==0) {
//			Presets[PresetAtivo & 0x000F].SafeMode = 0;
//			PresetAtivo &= ~((uint8_t)0x01 << 5);
//			RFEnable = 1;
//			Write_Eeprom(ADDR_RFENABLE, (uint8_t*)&RFEnable, 1);
//			Write_Eeprom(ADDR_PRESET_ATIVO, (uint8_t*)&PresetAtivo, 2);
//			Write_Eeprom(ADDR_PRE0_MODE + ((PresetAtivo & 0x000F) * 36), (uint8_t*)&Presets[PresetAtivo & 0x000F].SafeMode, 1);
//			if(!CheckRFAlarme()) {
//				//HAL_GPIO_WritePin(LED_RF_ON_GPIO_Port, LED_RF_ON_Pin, LED_ON);		// LED RF ON
//				RF_Enable();
         // BSP_LED_On(LED1);
        }
        /* Switch LED2 ON if 2 */
        else if(strcmp(pcValue[i], "2") ==0) {
//			Presets[PresetAtivo & 0x000F].SafeMode = 1;
//			PresetAtivo |= 0x01 << 5;
//			RFEnable = 2;
//			Write_Eeprom(ADDR_RFENABLE, (uint8_t*)&RFEnable, 1);
//			Write_Eeprom(ADDR_PRESET_ATIVO, (uint8_t*)&PresetAtivo, 2);
//			Write_Eeprom(ADDR_PRE0_MODE + ((PresetAtivo & 0x000F) * 36), (uint8_t*)&Presets[PresetAtivo & 0x000F].SafeMode, 1);
//			if(!CheckRFAlarme()) {
//				//HAL_GPIO_WritePin(LED_RF_ON_GPIO_Port, LED_RF_ON_Pin, LED_ON);		// LED RF ON
//				RF_Enable();
//			}
          //BSP_LED_On(LED2);
        }
        /* Switch LED3 ON if 3 */
        else if(strcmp(pcValue[i], "3") ==0) {
//			Presets[PresetAtivo & 0x000F].SafeMode = 0;
//			PresetAtivo &= ~((uint8_t)0x01 << 5);
//			RFEnable = 0;
//			Write_Eeprom(ADDR_RFENABLE, (uint8_t*)&RFEnable, 1);
//			Write_Eeprom(ADDR_PRESET_ATIVO, (uint8_t*)&PresetAtivo, 2);
//			Write_Eeprom(ADDR_PRE0_MODE + ((PresetAtivo & 0x000F) * 36), (uint8_t*)&Presets[PresetAtivo & 0x000F].SafeMode, 1);
//			HAL_GPIO_WritePin(LED_RF_ON_GPIO_Port, LED_RF_ON_Pin, LED_OFF);		// LED RF OFF
//			// Desliga Falhas de SWR And REFLECTED
//			falha &= ~((uint64_t)1ULL << FAIL_SWR);
//			falha &= ~((uint64_t)1ULL << FAIL_RFL);
//			RF_Disable();
          //BSP_LED_On(LED3);
        }
        /* Switch LED4 ON if 4 */
        else if(strcmp(pcValue[i], "4") ==0) {
          //BSP_LED_On(LED4);
        }
      }
    }
  }
  /* uri to send after cgi call*/
  return "/STM32F7xxLED.html";  
}

/**
  * @brief  Http webserver Init
  */
void http_server_init(void)
{
  /* Httpd Init */
  httpd_init();
  
  /* configure SSI handlers (ADC page SSI) */
  http_set_ssi_handler(ADC_Handler, (char const **)TAGS, 1);
  
  /* configure CGI handlers (LEDs control CGI) */
  CGI_TAB[0] = LEDS_CGI;
  http_set_cgi_handlers(CGI_TAB, 1);  
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
