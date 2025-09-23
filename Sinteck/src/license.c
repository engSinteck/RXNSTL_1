/*
 * license.c
 *
 *  Created on: Sep 11, 2025
 *      Author: rinaldo.santos
 */

#include "main.h"
#include "../Sinteck/src/eeprom.h"

extern Cfg_var cfg;
license_var lic;

void cryptLic(void)
{
	for(uint8_t x = 0; x < 8; x++) {
		lic.MyLic[x] = (char)(  ((cfg.License[x]-'A') ^ (cfg.SerialNumber[x] + '0')) );
	}
}

uint32_t CheckLicense(void)
{
	ReadLicSeq();
	cryptLic();

	lic.myseq = (((lic.MyLic[0] - '0') * 10) + (lic.MyLic[1] - '0'));

	if(lic.LicSeq >= lic.myseq) {
		return 0;
	}

	lic.ln1 = (lic.MyLic[2] - '0');
	lic.ln2 = (lic.MyLic[3] - '0');
	lic.ln3 = (lic.MyLic[4] - '0');

	lic.mydays = (lic.ln1 * 100) + (lic.ln2 * 10) + (lic.ln3 * 1);

	lic.myverify = (((lic.MyLic[5] - '0') * 100) + ((lic.MyLic[6] - '0')*10) + lic.MyLic[7] - '0');
	lic.verify = ((lic.MyLic[0] - '0') + (lic.MyLic[1] - '0') + (lic.MyLic[2] - '0') + (lic.MyLic[3] - '0') + (lic.MyLic[4] - '0'));

	if (lic.verify == lic.myverify) {
		return lic.mydays;
	}
	else {
		return 0;
	}
}
