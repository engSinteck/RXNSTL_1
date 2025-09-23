/*
 * json_util.h
 *
 *  Created on: 10 de dez de 2020
 *      Author: Rinaldo Dos Santos
 *      Sinteck Next
 */

#ifndef TCP_JSON_UTIL_H_
#define TCP_JSON_UTIL_H_

void Teste_cJSON_config(void);
void Teste_cJSON_telemetry(void);
size_t json_config( char* dest);
size_t json_central( char* dest);
size_t json_preset( char* dest);
size_t json_teste( char* dest);
size_t json_errors( char* dest);
size_t json_alarm( char* dest,  uint8_t* alarm);

#endif /* TCP_JSON_UTIL_H_ */
