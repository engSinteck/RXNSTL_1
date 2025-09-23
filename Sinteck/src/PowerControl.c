/*
 * PowerControl.c
 *
 *  Created on: Aug 25, 2025
 *      Author: rinaldo.santos
 */


#include "main.h"

#include "../Sinteck/src/PowerControl.h"
#include "../Sinteck/src/pwm.h"

void foldback(void)
{

}

void Power_Control(void)
{

}

void Check_IPA_MAX(void)
{

}

void Check_IPA(void)
{

}

void Check_VPA(void)
{

}

void Check_VBAT(void)
{

}

void Control_Temperature(void)
{

}

void Check_VSWR(void)
{

}


void Check_Reflected(void)
{

}

void RF_Enable(void)
{

}

void RF_Disable(void)
{

}

void RF_Disable_IPA(void)
{

}

void status_Remote(void)
{

}

void Check_License(void)
{
//	if( (license[0] == 0 || license[0] == 0xFF || LicSeq == 99) && (falha & (1ULL << FAIL_LICENSE)) ) {
//		// Licensa FULL
//		falha &= ~((uint64_t)1ULL << FAIL_LICENSE);
//		Alarme_Ins(FAIL_LICENSE, 0, 1);
//	}
//	else if( (LicSeq > 0 && LicSeq != 99) && (license[0] == 0 || license[0] == 0xFF) ) {
//		if(licenseTimer == 0 && !(falha & (1ULL << FAIL_LICENSE))) {
//			falha |= 1ULL << FAIL_LICENSE;
//			Alarme_Ins(FAIL_LICENSE, 0, 0);
//			RFEnable = 0;
//			RF_Disable();
//		}
//		else if( licenseTimer >= 1 && (falha & (1ULL << FAIL_LICENSE)) ) {
//			falha &= ~((uint64_t)1ULL << FAIL_LICENSE);
//			Alarme_Ins(FAIL_LICENSE, 0, 1);
//		}
//	}
}
