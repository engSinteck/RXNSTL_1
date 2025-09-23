/*
 * udp_rds.c
 *
 *  Created on: 10 de mar de 2021
 *      Author: Rinaldo Dos Santos
 *      Sinteck Next
 */

#include "lwip/opt.h"

#include "lwip/sys.h"
#include "lwip/api.h"

#include "main.h"
#include "string.h"
#include "stdio.h"
#include "../Sinteck/tcp/udp_rds.h"

static struct netconn *conn_udp;
static struct netbuf *buf_udp;
static ip_addr_t *addr_udp;
static unsigned short port_udp;
static uint8_t *buf_socket_udp = NULL;
static uint16_t len_udp_in;
static char string_udp[80];
uint8_t flag_udp_rds = 0;

extern rds_var rds;
extern uint16_t port_udp;

extern uint8_t flagtxtrds;

/*-----------------------------------------------------------------------------------*/
void udpecho_thread(void *arg)
{
	err_t err_udp, recv_err_udp;

	LWIP_UNUSED_ARG(arg);

	flag_udp_rds = 0;
	conn_udp = netconn_new(NETCONN_UDP);
	if (conn_udp!= NULL) {
		err_udp = netconn_bind(conn_udp, NULL, port_udp);
		if (err_udp == ERR_OK) {
			while (1) {
				recv_err_udp = netconn_recv(conn_udp, &buf_udp);

				if (recv_err_udp == ERR_OK) {
					netbuf_data(buf_udp, (void**)&buf_socket_udp, &len_udp_in);
					addr_udp = netbuf_fromaddr(buf_udp);
					port_udp = netbuf_fromport(buf_udp);
					netconn_connect(conn_udp, addr_udp, port_udp);
					buf_udp->addr.addr = 0;
					netconn_send(conn_udp, buf_udp);
					// Debug
					memcpy(string_udp, buf_socket_udp, len_udp_in);
					string_udp[len_udp_in] = '\0';		// Fim String
					if (string_udp[0] == 'D' && string_udp[1] == 'P' && string_udp[2] == 'S' && string_udp[3] == '=') {
						if( (strlen(string_udp)-3) < 64 ) {
							flagtxtrds = 1;
							memcpy(rds.dps1, &string_udp[4], strlen(string_udp)-4);
							rds.dps1[strlen(string_udp)-4] = '\0';		// Fim String
							flag_udp_rds = 1;
						}
					}
					else if (string_udp[0] == 'R' && string_udp[1] == 'T' && string_udp[2] == '=') {
						if( (strlen(string_udp)-3) < 64 ) {
							flagtxtrds = 1;
							memcpy(rds.rt1, &string_udp[3], strlen(string_udp)-3);
							rds.rt1[strlen(string_udp)-3] = '\0';		// Fim String
							flag_udp_rds = 2;
						}
					}
					len_udp_in = 0;
					string_udp[0] = '\0';
					netbuf_delete(buf_udp);
				}
			}
		}
		else {
			netconn_delete(conn_udp);
		}
	}
}

/*-----------------------------------------------------------------------------------*/
void udpecho_init(void)
{
	sys_thread_new("udp_thread", udpecho_thread, NULL, 2048, osPriorityLow );
}
