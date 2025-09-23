/*
 * tcp_client.h
 *
 *  Created on: 19 de nov de 2020
 *      Author: Rinaldo Dos Santos
 *      Sinteck Next
 */

#ifndef TCP_TCP_CLIENT_H_
#define TCP_TCP_CLIENT_H_

#define DEST_IP_ADDR0   (uint8_t)177
#define DEST_IP_ADDR1   (uint8_t)71
#define DEST_IP_ADDR2   (uint8_t)230
#define DEST_IP_ADDR3   (uint8_t)18

#define DEST_PORT       (uint32_t)443

void tcp_client_connect(void);
void get_dns_TelemetriaAtiva(void);

#endif /* TCP_TCP_CLIENT_H_ */
