/*
 * tcp_client_netconn.c
 *
 *  Created on: Nov 25, 2020
 *      Author: Rinaldo Dos Santos
 *      Sinteck Next
 */

#include "lwip/opt.h"

#include "lwip/sys.h"
#include "lwip/api.h"

#include "main.h"
#include "string.h"
#include "stdio.h"
#include "../Sinteck/tcp/tcp_client_netconn.h"

char sendData[1000] = {0};

extern uint8_t Telemetry_State;

static void Task_TCP_Client(void *arg)
{
    err_t  err;
    ip4_addr_t serverIPAddress;
    struct netbuf *recvData;
    char *dataBuff;

    IP4_ADDR(&serverIPAddress, SERVER_IP_0, SERVER_IP_1, SERVER_IP_2, SERVER_IP_3);

	uint32_t idPart1 = HAL_GetUIDw0();
	uint32_t idPart2 = HAL_GetUIDw1();
	uint32_t idPart3 = HAL_GetUIDw2();
	uint32_t idPart4 = HAL_GetDEVID();

    while(1)
    {
    	struct netconn* tcpClientConn = netconn_new(NETCONN_TCP);

    	if(tcpClientConn != NULL)
        {
    		err = netconn_connect(tcpClientConn, &serverIPAddress, 443);
            if(err == ERR_OK) {
            	Telemetry_State = 1;
            	sprintf((char*)sendData, "GET /api/v1/get_token HTTP/1.1\r\nHost: %s\r\nX-Uuid: %08lX-%02X%02X-%02X%02X-%02X%02X-%02X%02X%08lX\r\n\r\n",
            	       "xt.sinteck.com.br",
            	  	   idPart1,
            	 	   (uint8_t)((idPart4 & 0xFF000000) >> 24),
            		   (uint8_t)((idPart4 & 0x00FF0000) >> 16),
            		   (uint8_t)((idPart4 & 0x0000FF00) >> 8),
					   (uint8_t)((idPart4 & 0x000000FF)),
            		   (uint8_t)((idPart2 & 0xFF000000) >> 24),
            		   (uint8_t)((idPart2 & 0x00FF0000) >> 16),
            		   (uint8_t)((idPart2 & 0x0000FF00) >> 8),
            		   (uint8_t)((idPart2 & 0x000000FF)),
            		   idPart3 );

            	err = netconn_write(tcpClientConn, sendData, strlen(sendData), NETCONN_COPY);
            	if(err == ERR_OK) {
            		err = netconn_recv(tcpClientConn, &recvData);
                    if(err == ERR_OK) {
                    	netbuf_data(recvData, (void *)&dataBuff, &netbuf_len(recvData));
    					//netbuf_delete(recvData);
    					//err = netconn_close(tcpClientConn);
    					//err = netconn_delete(tcpClientConn);
    				}
    				else {
    					// Error netconn_recv
    					Telemetry_State = 0;
    					netconn_close(tcpClientConn);
    					err = netconn_delete(tcpClientConn);
    				}
    			}
                else {
                	// Error netconn_write
                	Telemetry_State = 0;
                	err = netconn_close(tcpClientConn);
                    err = netconn_delete(tcpClientConn);
    			}
    		}
    		else {
    			// Error netconn_connect
    			Telemetry_State = 0;
    			err = netconn_delete(tcpClientConn);
    		}
    	}
    	else {
    		// tcpClientConn
    		Telemetry_State = 0;
    		netconn_delete(tcpClientConn);
    	}
    }
}

void tcp_client_init(void)
{
  sys_thread_new("tcp_client", Task_TCP_Client, NULL, 3072, osPriorityLow);
}
