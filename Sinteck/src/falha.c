/*
 * falha.c
 *
 *  Created on: Sep 10, 2025
 *      Author: rinaldo.santos
 */

#include "main.h"
#include "cmsis_os.h"
#include "queue.h"
#include "../Sinteck/src/falha.h"

extern osMessageQueueId_t QueueMqttHandle;
extern uint8_t debug;

Cod_Alarme alarme[QTD_MAX_ALARME];

char *retfalha = "ABCDEFGHIJKLMNOPQRSTUVXZ";
uint8_t alarm_mqtt_tx[32] = { 0 };

uint64_t falha = 0;

void ClearAlarme(void)
{
	uint16_t x;

	ContAlarme = 0;

	for(x = 0; x < QTD_MAX_ALARME; x++) {
		alarme[x].enable = 0;
		alarme[x].alarme = 0;
		alarme[x].flag = 0;
		alarme[x].subtipo = 0;
		alarme[x].alm1 = 0;
		alarme[x].alm2 = 0;
		alarme[x].hour = 0;
		alarme[x].minute = 0;
		alarme[x].second = 0;
		alarme[x].day = 0;
		alarme[x].month = 0;
		alarme[x].year = 0;
		alarme[x].text[0] = 0;
		for(uint8_t z=0; z < 19; z++) {
			alarme[x].text[z] = 0;
		}
	}
}

void SetAlarmeTeste(void)
{
	uint16_t x;

	for(x = 0; x < QTD_MAX_ALARME; x++) {
		alarme[x].enable = 1;
		alarme[x].alarme = 21;
		alarme[x].flag = 0;
		alarme[x].hour = 10;
		alarme[x].minute = 0;
		alarme[x].second = 0;
		alarme[x].day = 2;
		alarme[x].month = 9;
		alarme[x].year = 2020;
	}
	ContAlarme = QTD_MAX_ALARME;
}



void status_falha(void)
{
	uint64_t opt;

	// 0xFFFF FFFF FFFF FFFF
	// 0xF F F F - F F F F - F F F F - F F F F
	//      60  56  52  48  44  40  36  32  28  24  20  16  12  8  4  0
	//		63  59 	55  51  47  43	39  35	31  27  23  19  15 11  7  3
//	if(flag_pll_power) {
		opt = 0x000000FFCBFC0FFF;
//	}
//	else {
//		opt = 0x000000FFCBFC0FFE;
//	}

	if( falha & opt ) {
		HAL_GPIO_WritePin(LED_FAIL_GPIO_Port, LED_FAIL_Pin, LED_ON);
	}
	else {
		HAL_GPIO_WritePin(LED_FAIL_GPIO_Port, LED_FAIL_Pin, LED_OFF);
	}
#ifdef GIGA
	HAL_GPIO_WritePin(LED_FAIL_GPIO_Port, LED_FAIL_Pin, LED_ON);
#endif
}

char* decode_falha(uint64_t fail)
{
	if( (falha & (1ULL << FAIL_PLLLOCK)) /*&& flag_pll_power*/ ) {
		retfalha = "F-PLL  ";
	}
	else if( (falha & (1ULL << FAIL_10MHZ)) ) {
		retfalha = "F-10MHz";
	}
	else if( (falha & (1ULL << FAIL_FAN1)) ) {
		retfalha = "F-FAN1 ";
	}
	else if( (falha & (1ULL << FAIL_FAN2)) ) {
		retfalha = "F-FAN2 ";
	}
	else if( (falha & (1ULL << FAIL_FAN3)) ) {
		retfalha = "F-FAN3 ";
	}
	else if( (falha & (1ULL << FAIL_SWR)) ) {
		retfalha = "F-SWR  ";
	}
	else if( (falha & (1ULL << FAIL_TEMPERATURE)) ) {
		retfalha = "F-TEMP ";
	}
	else if( (falha & (1ULL << FAIL_VPA)) ) {
		retfalha = "F-VPA  ";
	}
	else if( (falha & (1ULL << FAIL_IPA)) ) {
		retfalha = "F-IPA  ";
	}
	else if( (falha & (1ULL << FAIL_FOLD)) ) {
		retfalha = "F-FOLD ";
	}
	else if( (falha & (1ULL << FAIL_FOLDIPA)) ) {
		retfalha = "F-FoIPA";
	}
	else if( (falha & (1ULL << FAIL_RFL)) ) {
		retfalha = "F-HiREF";
	}
	else if( (falha & (1ULL << FAIL_REMOTE)) ) {
		retfalha = "W-REMOT";
	}
	else if( (falha & (1ULL << FAIL_USB_CONNECT)) ) {
		retfalha = "F-USBC ";
	}
	else if( (falha & (1ULL << FAIL_USB_MOUNT)) ) {
		retfalha = "F-USBM ";
	}
	else if( (falha & (1ULL << FAIL_SYNCRDS)) ) {
		retfalha = "F-SiRDS";
	}
	else if( (falha & (1ULL << FAIL_RDSCARD)) ) {
		retfalha = "F-RDSC ";
	}
	else if((falha & (1ULL << FAIL_VINPUT)) ) {
		retfalha = "F-Vac  ";
	}
	else if( (falha & (1ULL << FAIL_PSTEMP)) ) {
		retfalha = "F-PSTem";
	}
	else if( (falha & (1ULL << FAIL_RFDRIVER)) ) {
		retfalha = "F-RFDri";
	}
	else if( (falha & (1ULL << FAIL_IPADRIVER)) ) {
		retfalha = "F-IPDri";
	}
	else if( (falha & (1ULL << FAIL_NOANTENNA)) ) {
		retfalha = "F-NoAnt";
	}
	else if( (falha & (1ULL << FAIL_ALLFAN)) ) {
		retfalha = "F-ALFAN";
	}
	else if( (falha & (1ULL << FAIL_MECSENSE)) ) {
		retfalha = "F-MECS ";
	}
	else if( (falha & (1ULL << FAIL_BATTLOW)) ) {
		retfalha = "F-BATLo";
	}
	else if( (falha & (1ULL << FAIL_PSU)) ) {
		retfalha = "F-PSU  ";
	}
	else if( (falha & (1ULL << FAIL_PSUWALK)) ) {
		retfalha = "W-PSUWa";
	}
	else if( (falha & (1ULL << FAIL_LICENSE)) ) {
		retfalha = "F-Oper ";
	}
	else if( (falha & (1ULL << FAIL_UPLINK)) ) {
		retfalha = "W-Uplnk";
	}
	else if( (falha & (1ULL << FAIL_BATCHARGE)) ) {
		retfalha = "W-BATCh";
	}
	else if( (falha & (1ULL << FAIL_BATMAX)) ) {
		retfalha = "F-BATMa";
	}
	else if( (falha & (1ULL << FAIL_PWR)) ) {
		retfalha = "F-FWD  ";
	}
	else if( (falha & (1ULL << FAIL_TEMPDRV)) ) {
		retfalha = "F-DrvT ";
	}
	else if( (falha & (1ULL << FAIL_IPA_BURN)) ) {
		retfalha = "F-IPAov";
	}
	else if( (falha & (1ULL << FAIL_PALLET)) ) {
		retfalha = "F-Palle";
	}
	else if( (falha & (1ULL << FAIL_ELTEK)) ) {
		retfalha = "F-PsSup";
	}
	else if( (falha & (1ULL << FAIL_FAULTIPA)) ) {
		retfalha = "F-MRF  ";
	}
	else if( (falha & (1ULL << FAIL_AUDIOMPX)) ) {
		retfalha = "F-MPX  ";
	}
	else {
		retfalha = "OK     ";
	}

	return retfalha;
}

char* decode_falha_2(uint64_t fail)
{
	switch(fail) {
		case 0:
			//if(flag_pll_power) {
				retfalha = "F-PLL  ";
			//}
			break;
		case 1:
			retfalha = "F-10MHz";
			break;
		case 2:
			retfalha = "F-FAN1 ";
			break;
		case 3:
			retfalha = "F-FAN2 ";
			break;
		case 4:
			retfalha = "F-FAN3 ";
			break;
		case 5:
			retfalha = "F-SWR  ";
			break;
		case 6:
			retfalha = "F-TEMP ";
			break;
		case 7:
			retfalha = "F-VPA  ";
			break;
		case 8:
			retfalha = "F-IPA  ";
			break;
		case 9:
			retfalha = "F-FOLD ";
			break;
		case 10:
			retfalha = "F-FoIPA";
			break;
		case 11:
			retfalha = "F-HiREF";
			break;
		case 12:
			retfalha = "W-REMOT";
			break;
		case 13:
			retfalha = "W-USBC ";
			break;
		case 14:
			retfalha = "W-USBM ";
			break;
		case 15:
			retfalha = "W-SiRDS";
			break;
		case 16:
			retfalha = "W-RDSC ";
			break;
		case 17:
			retfalha = "F-Vac  ";
			break;
		case 18:
			retfalha = "F-PSTem";
			break;
		case 19:
			retfalha = "F-RFDri";
			break;
		case 20:
			retfalha = "F-IPDri";
			break;
		case 21:
			retfalha = "F-NoAnt";
			break;
		case 22:
			retfalha = "F-ALFAN";
			break;
		case 23:
			retfalha = "F-MECS ";
			break;
		case 24:
			retfalha = "F-BATLo";
			break;
		case 25:
			retfalha = "F-PSU  ";
			break;
		case 26:
			retfalha = "W-PSUWa";
			break;
		case 27:
			retfalha = "F-Oper ";
			break;
		case 28:
			retfalha = "W-Uplnk";
			break;
		case 29:
			retfalha = "W-BATCh";
			break;
		case 30:
			retfalha = "F-BATMa";
			break;
		case 31:
			retfalha = "F-FWD  ";
			break;
		case 32:
			retfalha = "F-DrvT ";
			break;
		case 33:
			retfalha = "F-IPAov ";
			break;
		case 34:
			retfalha = "F-Pallet";
			break;
		case 35:
			retfalha = "F-PsSup ";
			break;
		case 36:
			retfalha = "F-RFM   ";
			break;
		case 37:
			retfalha = "F-STK   ";;
			break;
		case 38:
			retfalha = "F-MPX   ";;
			break;
		default:
			retfalha = "       ";
			break;
	}
	return retfalha;
}

char* decode_falha_remote(uint8_t fail)
{
	switch(fail) {
		case 0:
			//if(flag_pll_power) {
				retfalha = "F-PLL  ";
			//}
			break;
		case 1:
			retfalha = "F-10MHz";
			break;
		case 2:
			retfalha = "F-FAN1 ";
			break;
		case 3:
			retfalha = "F-FAN2 ";
			break;
		case 4:
			retfalha = "F-FAN3 ";
			break;
		case 5:
			retfalha = "F-SWR  ";
			break;
		case 6:
			retfalha = "F-TEMP ";
			break;
		case 7:
			retfalha = "F-VPA  ";
			break;
		case 8:
			retfalha = "F-IPA  ";
			break;
		case 9:
			retfalha = "F-FOLD ";
			break;
		case 10:
			retfalha = "F-FoIPA";
			break;
		case 11:
			retfalha = "F-HiREF";
			break;
		case 12:
			retfalha = "W-REMOT";
			break;
		case 13:
			retfalha = "W-USBC ";
			break;
		case 14:
			retfalha = "W-USBM ";
			break;
		case 15:
			retfalha = "W-SiRDS";
			break;
		case 16:
			retfalha = "W-RDSC ";
			break;
		case 17:
			retfalha = "F-Vac  ";
			break;
		case 18:
			retfalha = "F-PSTem";
			break;
		case 19:
			retfalha = "F-RFDri";
			break;
		case 20:
			retfalha = "F-IPDri";
			break;
		case 21:
			retfalha = "F-NoAnt";
			break;
		case 22:
			retfalha = "F-ALFAN";
			break;
		case 23:
			retfalha = "F-MECS ";
			break;
		case 24:
			retfalha = "F-BATLo";
			break;
		case 25:
			retfalha = "F-PSU  ";
			break;
		case 26:
			retfalha = "W-PSUWa";
			break;
		case 27:
			retfalha = "F-Oper ";
			break;
		case 28:
			retfalha = "W-Uplnk";
			break;
		case 29:
			retfalha = "W-BATCh";
			break;
		case 30:
			retfalha = "F-BATMa";
			break;
		case 31:
			retfalha = "F-FWD  ";
			break;
		case 32:
			retfalha = "F-DrvT ";
			break;
		case 33:
			retfalha = "F-IPAov ";
			break;
		case 34:
			retfalha = "F-Pallet";
			break;
		case 35:
			retfalha = "F-PsSup ";
			break;
		case 36:
			retfalha = "F-RFM   ";
			break;
		case 37:
			retfalha = "F-STK   ";
			break;
		case 38:
			retfalha = "F-MPX   ";
			break;
		default:
			retfalha = "       ";
			break;
	}
	return retfalha;
}


char* decode_falha_snmp(uint64_t fail)
{
	if( (falha & (1ULL << FAIL_PLLLOCK)) ) {
		//if(flag_pll_power) {
			retfalha = "F-PLL  ";
		//}
	}
	else if( (falha & (1ULL << FAIL_10MHZ)) ) {
		retfalha = "F-10MHz";
	}
	else if( (falha & (1ULL << FAIL_FAN1)) ) {
		retfalha = "F-FAN1 ";
	}
	else if( (falha & (1ULL << FAIL_FAN2)) ) {
		retfalha = "F-FAN2 ";
	}
	else if( (falha & (1ULL << FAIL_FAN3)) ) {
		retfalha = "F-FAN3";
	}
	else if( (falha & (1ULL << FAIL_SWR)) ) {
		retfalha = "F-SWR  ";
	}
	else if( (falha & (1ULL << FAIL_TEMPERATURE)) ) {
		retfalha = "F-TEMP ";
	}
	else if( (falha & (1ULL << FAIL_VPA)) ) {
		retfalha = "F-VPA  ";
	}
	else if( (falha & (1ULL << FAIL_IPA)) ) {
		retfalha = "F-IPA  ";
	}
	else if( (falha & (1ULL << FAIL_FOLD)) ) {
		retfalha = "F-FOLD ";
	}
	else if( (falha & (1ULL << FAIL_FOLDIPA)) ) {
		retfalha = "F-FoIPA";
	}
	else if( (falha & (1ULL << FAIL_RFL)) ) {
		retfalha = "F-HiREF";
	}
	else if( (falha & (1ULL << FAIL_REMOTE)) ) {
		retfalha = "W-REMOT";
	}
	else if( (falha & (1ULL << FAIL_USB_CONNECT)) ) {
		retfalha = "W-USBC ";
	}
	else if( (falha & (1ULL << FAIL_USB_MOUNT)) ) {
		retfalha = "W-USBM ";
	}
	else if( (falha & (1ULL << FAIL_SYNCRDS)) ) {
		retfalha = "W-SiRDS";
	}
	else if( (falha & (1ULL << FAIL_RDSCARD)) ) {
		retfalha = "W-RDSC ";
	}
	else if( (falha & (1ULL << FAIL_VINPUT)) ) {
		retfalha = "F-Vac  ";
	}
	else if( (falha & (1ULL << FAIL_PSTEMP)) ) {
		retfalha = "F-PSTem";
	}
	else if( (falha & (1ULL << FAIL_RFDRIVER)) ) {
		retfalha = "F-RFDri";
	}
	else if( (falha & (1ULL << FAIL_IPADRIVER)) ) {
		retfalha = "F-IPDri";
	}
	else if( (falha & (1ULL << FAIL_NOANTENNA)) ) {
		retfalha = "F-NoAnt";
	}
	else if( (falha & (1ULL << FAIL_ALLFAN)) ) {
		retfalha = "F-ALFAN";
	}
	else if( (falha & (1ULL << FAIL_MECSENSE)) ) {
		retfalha = "F-MECS ";
	}
	else if( (falha & (1ULL << FAIL_BATTLOW)) ) {
		retfalha = "F-BATLo";
	}
	else if( (falha & (1ULL << FAIL_PSU)) ) {
		retfalha = "F-PSU  ";
	}
	else if( (falha & (1ULL << FAIL_PSUWALK)) ) {
		retfalha = "W-PSUWa";
	}
	else if( (falha & (1ULL << FAIL_LICENSE)) ) {
		retfalha = "F-Oper ";
	}
	else if( (falha & (1ULL << FAIL_UPLINK)) ) {
		retfalha = "W-Uplnk";
	}
	else if( (falha & (1ULL << FAIL_BATCHARGE)) ) {
		retfalha = "W-BATCh";
	}
	else if( (falha & (1ULL << FAIL_BATMAX)) ) {
		retfalha = "F-BATMa";
	}
	else if( (falha & (1ULL << FAIL_PWR)) ) {
		retfalha = "F-FWD  ";
	}
	else if( (falha & (1ULL << FAIL_TEMPDRV)) ) {
		retfalha = "F-DrvT ";
	}
	else if( (falha & (1ULL << FAIL_IPA_BURN)) ) {
		retfalha = "F-IPAov ";
	}
	else if( (falha & (1ULL << FAIL_PALLET)) ) {
		retfalha = "F-Pallet";
	}
	else if( (falha & (1ULL << FAIL_ELTEK)) ) {
		retfalha = "F-PsSup ";
	}
	else if( (falha & (1ULL << FAIL_FAULTIPA)) ) {
		retfalha = "F-RFM   ";
	}
	else if( (falha & (1ULL << FAIL_STACK)) ) {
		retfalha = "F-STK   ";
	}
	else if( (falha & (1ULL << FAIL_AUDIOMPX)) ) {
		retfalha = "F-MPX   ";
	}
	else {
		retfalha = "       ";
	}

	return retfalha;
}

char* decode_falha_Eltek(uint8_t alm1, uint8_t alm2)
{
	return "OK";
}

//sets the bit pos to the value. if value is == 0 it zeroes the bit if value !=0 it sets the bit
void setbitfalha(uint64_t *val, int bit, int value)
{
    if(value)
    {
        *val |= ((uint64_t)1 << bit);
    }
    else
    {
        *val &= ~((uint64_t)1 << bit);
    }
}



