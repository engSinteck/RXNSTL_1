/*
 * json_util.c
 *
 *  Created on: 10 de dez de 2020
 *      Author: Rinaldo Dos Santos
 *      Sinteck Next
 */
#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../cJSON/json-maker.h"
#include "../cJSON/tiny-json.h"
#include "../Sinteck/src/falha.h"
#include "../Sinteck/tcp/json_util.h"


extern Cfg_var cfg;
extern rds_var rds;
extern AdvancedSettings adv;
extern RTC_DateTypeDef gDate;
extern RTC_TimeTypeDef gTime;
extern Profile_var Profile;
extern license_var lic;
extern SYS_UPTime uptime;
extern SYS_Realtime Realtime;
extern Cod_Alarme alarme[];

extern char str_uptime[];
extern const char* versao;
extern char version_flash[];
extern uint64_t falha;
extern uint32_t ContAlarme;
extern reset_cause_t reset_cause;
extern const char * reset_cause_get_name(reset_cause_t reset_cause);

char buff[4096];

struct weather {
    int temp;
    int hum;
};

struct time {
    int hour;
    int minute;
};

struct measure {
    struct weather weather;
    struct time time;
};

struct data {
    char const* city;
    char const* street;
    struct measure measure;
    int samples[ 4 ];
};

// Add JSON Config
size_t json_config( char* dest)
{
	char rasc[50] = { 0 };

	dest = json_objOpen( dest, NULL );

	// Audio
	dest = json_objOpen( dest, "Audio" );
	dest = json_bool( dest, "Stereo",  cfg.MonoStereo);
	dest = json_int( dest, "Source", cfg.AudioSource );
	dest = json_bool( dest, "Emphase", cfg.Emphase );
	dest = json_bool( dest, "Processor", cfg.Processor );
	dest = json_bool( dest, "AES192", cfg.AES192 );
	dest = json_int( dest, "VolMPX1", cfg.Vol_MPX1);
	dest = json_int( dest, "VolMPX2", cfg.Vol_MPX2 );
	dest = json_objClose( dest );
	// RDS
	dest = json_objOpen( dest, "rds" );
	dest = json_bool( dest, "Enable", rds.enable );
	dest = json_bool( dest, "Date-Time", rds.ct );
	dest = json_int( dest, "Type", rds.type );
	dest = json_str( dest, "ps", rds.ps);
	dest = json_str( dest, "pi", rds.pi);
	dest = json_str( dest, "pty", rds.ptyn);
	dest = json_int( dest, "ptyn", rds.pty);
	dest = json_int( dest, "ms", rds.ms);
	dest = json_int( dest, "af1", rds.af[0]);
	dest = json_int( dest, "af2", rds.af[1]);
	dest = json_int( dest, "af3", rds.af[2]);
	dest = json_int( dest, "af4", rds.af[3]);
	dest = json_int( dest, "af5", rds.af[4]);
	dest = json_str( dest, "RadioText", rds.rt1);
	dest = json_str( dest, "DinamicText", rds.dps1);
	dest = json_bool( dest, "Remote", cfg.RDSRemote);
	dest = json_int( dest, "Port", cfg.RDSUDPPort);
	dest = json_objClose( dest );
	// Advanced Settings
	dest = json_objOpen( dest, "AdvancedSettings" );
	dest = json_float( dest, "MaxIPA", adv.MaxIpa);
	dest = json_float( dest, "MaxVSWR", adv.MaxVswr );
	dest = json_float( dest, "MaxTemp", adv.MaxTemp );
	dest = json_float( dest, "OffsetForward", adv.GainFWD );
	dest = json_float( dest, "OffsetReflected", adv.GainSWR );
	dest = json_float( dest, "OffsetIPA", adv.GainIPA );
	dest = json_float( dest, "OffsetVPA", adv.GainVPA );
	dest = json_float( dest, "OffsetTemperature", adv.GainTemp );
	dest = json_objClose( dest );
	// Network
	dest = json_objOpen( dest, "Connectivity" );
	sprintf(rasc, "%03d.%03d.%03d.%03d", cfg.IP_ADDR[0], cfg.IP_ADDR[1], cfg.IP_ADDR[2], cfg.IP_ADDR[3]);
	dest = json_str( dest, "IP", rasc );
	sprintf(rasc, "%03d.%03d.%03d.%03d", cfg.MASK_ADDR[0], cfg.MASK_ADDR[1], cfg.MASK_ADDR[2], cfg.MASK_ADDR[3]);
	dest = json_str( dest, "NetMask", rasc );
	sprintf(rasc, "%03d.%03d.%03d.%03d", cfg.GW_ADDR[0], cfg.GW_ADDR[1], cfg.GW_ADDR[2], cfg.GW_ADDR[3]);
	dest = json_str( dest, "Gateway", rasc );
	sprintf(rasc, "%03d.%03d.%03d.%03d", cfg.DNS_ADDR[0], cfg.DNS_ADDR[1], cfg.DNS_ADDR[2], cfg.DNS_ADDR[3]);
	dest = json_str( dest, "DNS", rasc );
	dest = json_int( dest, "Port", cfg.PortWEB );
	dest = json_bool( dest, "SNMP", cfg.EnableSNMP );
	dest = json_objClose( dest );
	// Password
	dest = json_objOpen( dest, "Password" );
	sprintf(rasc, "%d%d%d%d", cfg.PassAdmin[0], cfg.PassAdmin[1], cfg.PassAdmin[2], cfg.PassAdmin[3]);
	dest = json_str( dest, "Admin", rasc );
	sprintf(rasc, "%d%d%d%d", cfg.PassUser[0], cfg.PassUser[1], cfg.PassUser[2], cfg.PassUser[3]);
	dest = json_str( dest, "User", rasc );
	dest = json_objClose( dest );
	// Timer Server
	dest = json_objOpen( dest, "LocalTimes" );
	sprintf(rasc, "%02d-%02d-%04d", gDate.Month, gDate.Date, 2000+gDate.Year);
	dest = json_str( dest, "Date", rasc );
	sprintf(rasc, "%02d:%02d:%02d", gTime.Hours, gTime.Minutes, gTime.Seconds);
	dest = json_str( dest, "Time", rasc );
	dest = json_bool( dest, "TimeCfg", cfg.NTP );
	dest = json_int( dest, "TimeFuso", cfg.Timezone );
	dest = json_objClose( dest );
	//
	dest = json_objOpen( dest, "Profile" );
	dest = json_str( dest, "Station",  Profile.Station);
	dest = json_str( dest, "City",  Profile.City);
	dest = json_str( dest, "State",  Profile.State);
	dest = json_str( dest, "Country",  Profile.Country);
	dest = json_str( dest, "TempExt",  Profile.Temp);
	dest = json_objClose( dest );
	//
	dest = json_int( dest, "Frequency", cfg.Frequencia );
	dest = json_bool( dest, "ConfigInHold", cfg.ConfigHold );
	dest = json_bool( dest, "VSWRNULL", cfg.VSWR_Null );
	dest = json_float( dest, "VSWR_Value", cfg.VSWR_Null_Value );
	dest = json_float( dest, "FWD_NULL_VALUE", cfg.FWD_Null_Value );

	if( (cfg.License[0] == 0 || cfg.License[0] == 0xFF) ||
		(cfg.License[1] == 0 || cfg.License[1] == 0xFF) ||
		(cfg.License[2] == 0 || cfg.License[2] == 0xFF) ||
		(cfg.License[3] == 0 || cfg.License[3] == 0xFF) ||
		(cfg.License[4] == 0 || cfg.License[4] == 0xFF) ||
		(cfg.License[5] == 0 || cfg.License[5] == 0xFF) ||
		(cfg.License[6] == 0 || cfg.License[6] == 0xFF) ||
		(cfg.License[7] == 0 || cfg.License[7] == 0xFF) ) {

		dest = json_str( dest, "License", "AAAAAAAA" );
	}
	else {
		dest = json_str( dest, "License", cfg.License );
	}

	dest = json_int( dest, "LicSeq", lic.LicSeq );
	dest = json_int( dest, "LicenseTimer", lic.licenseTimer );

	uint32_t idPart1 = HAL_GetUIDw0();
	uint32_t idPart2 = HAL_GetUIDw1();
	uint32_t idPart3 = HAL_GetUIDw2();
	uint32_t idPart4 = HAL_GetDEVID();
	sprintf(rasc, "%08lX-%02X%02X-%02X%02X-%02X%02X-%02X%02X%08lX", idPart1,
	                                  (uint8_t)((idPart4 & 0xFF000000) >> 24),
				                      (uint8_t)((idPart4 & 0x00FF0000) >> 16),
				                      (uint8_t)((idPart4 & 0x0000FF00) >> 8),
				                      (uint8_t)((idPart4 & 0x000000FF)),
				                      (uint8_t)((idPart2 & 0xFF000000) >> 24),
				                      (uint8_t)((idPart2 & 0x00FF0000) >> 16),
				                      (uint8_t)((idPart2 & 0x0000FF00) >> 8),
				                      (uint8_t)((idPart2 & 0x000000FF)),
				                      idPart3 );
	dest = json_str( dest, "UUID", rasc );
	sprintf(rasc, "%c%c%c%c-%c%c%c%c", cfg.Token[0], cfg.Token[1], cfg.Token[2], cfg.Token[3],
			cfg.Token[4], cfg.Token[5], cfg.Token[6], cfg.Token[7]);
	dest = json_str( dest, "Token", rasc );
	sprintf(rasc, "%d%d%d%d%d%d%d%d", cfg.SerialNumber[0], cfg.SerialNumber[1], cfg.SerialNumber[2], cfg.SerialNumber[3],
			                          cfg.SerialNumber[4], cfg.SerialNumber[5], cfg.SerialNumber[6], cfg.SerialNumber[7]);
	dest = json_str( dest, "SerialNumber", rasc);
	dest = json_str( dest, "Model", "RXNSTL 900MHz" );
	dest = json_str( dest, "Version", versao);
	dest = json_str( dest, "GUI", version_flash);
	memset(str_uptime, 0, 50);
	sprintf(str_uptime, "%ldd - %02ld:%02ld:%02ld", uptime.dia, (uptime.total/3600)%24, (uptime.total/60)%60, uptime.total%60);
	dest = json_str( dest, "UpTime", str_uptime);

	dest = json_objClose( dest );
	dest = json_end( dest );

		return strlen(dest);
}


/* Add JSON Central-Telemetry */
size_t json_central( char* dest)
{
	dest = json_objOpen( dest, NULL );
	dest = json_float( dest, "FWD", Realtime.Forward );
	dest = json_float( dest, "REF", Realtime.Reflected );
	dest = json_float( dest, "Temp", Realtime.Temperature );
	dest = json_float( dest, "VPA", Realtime.VPA );
	dest = json_float( dest, "IPA1", Realtime.IPA );
	dest = json_int( dest, "PWMBias", Realtime.pwm_bias);
	dest = json_int( dest, "PWMFan", Realtime.pwm_fan);
	dest = json_int( dest, "Fail", falha );
	dest = json_str( dest, "ResetCause:", reset_cause_get_name(reset_cause));

	dest = json_objClose( dest );
	dest = json_end( dest );

	return strlen(dest);
}

/* Add JSON Central-Telemetry */
size_t json_errors( char* dest)
{
	uint8_t x;
	char txtalarm[64] = { 0 };

	dest = json_objOpen( dest, NULL );
	// Open Array
	for(x = 0; x < ContAlarme; x++) {
		if(alarme[x].enable != 0) {
			sprintf(txtalarm, "%s", decode_falha_2(alarme[x].alarme));

			dest = json_arrOpen( dest, NULL);
			dest = json_int( dest, NULL, 0);
			dest = json_int( dest, NULL, alarme[x].alarme);
			dest = json_str( dest, NULL, txtalarm);
			dest = json_arrClose( dest );
		}
	}
	dest = json_objClose( dest );
	dest = json_end( dest );

	return strlen(dest);
}

size_t json_alarm( char* dest,  uint8_t* alarm)
{
	char txtalarm[64] = { 0 };
	char txttime[32] = { 0 };

	sprintf( txttime, "%02d-%02d-%04d %02d:%02d:%02d", alarm[3], alarm[4], (alarm[5]*256 | alarm[6]), alarm[7], alarm[8], alarm[9] );

	if(alarm[2] == 0) {
		sprintf( txtalarm, "%s", decode_falha_remote(alarm[0]) );
	}
	else {
		sprintf( txtalarm, "%s - End", decode_falha_remote(alarm[0]) );
	}

	dest = json_objOpen( dest, NULL );

	dest = json_int( dest, "id", alarm[0]);
	dest = json_str( dest, "time", txttime);
	dest = json_str( dest, "text", txtalarm);

	dest = json_objClose( dest );
	dest = json_end( dest );

	return strlen(dest);
}

/* Add a time object property in a JSON string.
  "name":{"temp":-5,"hum":48}, */
char* json_weather( char* dest, char const* name, struct weather const* weather ) {
    dest = json_objOpen( dest, name );              // --> "name":{\0
    dest = json_int( dest, "temp", weather->temp ); // --> "name":{"temp":22,\0
    dest = json_int( dest, "hum", weather->hum );   // --> "name":{"temp":22,"hum":45,\0
    dest = json_objClose( dest );                   // --> "name":{"temp":22,"hum":45},\0
    return dest;
}

/* Add a time object property in a JSON string.
  "name":{"hour":18,"minute":32}, */
char* json_time( char* dest, char const* name, struct time const* time ) {
    dest = json_objOpen( dest, name );
    dest = json_int( dest, "hour",   time->hour   );
    dest = json_int( dest, "minute", time->minute );
    dest = json_objClose( dest );
    return dest;
}

/* Add a measure object property in a JSON string.
 "name":{"weather":{"temp":-5,"hum":48},"time":{"hour":18,"minute":32}}, */
char* json_measure( char* dest, char const* name, struct measure const* measure ) {
    dest = json_objOpen( dest, name );
    dest = json_weather( dest, "weather", &measure->weather );
    dest = json_time( dest, "time", &measure->time );
    dest = json_objClose( dest );
    return dest;
}

/* Add a data object property in a JSON string. */
char* json_data( char* dest, char const* name, struct data const* data ) {
    dest = json_objOpen( dest, NULL );
    dest = json_str( dest, "city",   data->city );
    dest = json_str( dest, "street", data->street );
    dest = json_measure( dest, "measure", &data->measure );
    dest = json_arrOpen( dest, "samples" );
    for( int i = 0; i < 4; ++i )
        dest = json_int( dest, NULL, data->samples[i] );
    dest = json_arrClose( dest );
    dest = json_objClose( dest );
    return dest;
}

/** Convert a data structure to a root JSON object.
  * @param dest Destination memory block.
  * @param data Source data structure.
  * @return  The JSON string length. */
int data_to_json( char* dest, struct data const* data ) {
    char* p = json_data( dest, NULL, data );
    p = json_end( p );
    return p - dest;
}

void Teste_cJSON_config(void)
{
	memset(buff, 0, 4096);

	json_config( buff );
}

void Teste_cJSON_telemetry(void)
{
	memset(buff, 0, 4096);

	json_central( buff );
	memset(buff, 0, 4096);
	json_errors( buff );
}
