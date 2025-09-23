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
	return strlen(dest);
}


/* Add JSON Central-Telemetry */
size_t json_central( char* dest)
{
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
