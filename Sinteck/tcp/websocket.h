/*
 * websocket.h
 *
 *  Created on: Mar 3, 2021
 *      Author: Rinaldo Dos Santos
 *      Sinteck Next
 */

#ifndef TCP_WEBSOCKET_H_
#define TCP_WEBSOCKET_H_

#define SERVER_NAME_W	"177.71.230.18"
#define SERVER_PORT_W 	"443"

void close_ws(void);
void Websocket_Start(void *arg);
int send_ws(char * message);
int ws_handshake(void);
int websocket_send(char *strdata);
void websocket_init(void);
void Websocket_TX(void *arg);

#endif /* TCP_WEBSOCKET_H_ */
