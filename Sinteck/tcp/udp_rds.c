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

#define UDP_BIND_PORT      8888	   // substitua com sua porta
#define RECV_TIMEOUT_MS    3000    // timeout para netconn_recv
#define MAX_UDP_PAYLOAD    512     // ajuste conforme necessidade
#define RDS_FIELD_SIZE     64

uint8_t flag_udp_rds = 0;

extern rds_var rds;
extern uint16_t port_udp;

extern uint8_t flagtxtrds;

/*-----------------------------------------------------------------------------------*/
void udpecho_thread(void *arg)
{
	err_t err;
	struct netconn *conn = NULL;
    struct netbuf *buf = NULL;
    void *data_ptr = NULL;
    u16_t data_len = 0;
    ip_addr_t *from_addr = NULL;
    u16_t from_port = 0;

    LWIP_UNUSED_ARG(arg);

    /* Criar conex?o UDP */
    conn = netconn_new(NETCONN_UDP);
    if (conn == NULL) {
        // Falha cr?tica ao alocar netconn
        // log erro e encerra a task
        return;
    }

    /* Bind na porta local (usando vari?vel local para deixar claro) */
    err = netconn_bind(conn, IP_ADDR_ANY, UDP_BIND_PORT);
    if (err != ERR_OK) {
        netconn_delete(conn);
        return;
    }

    /* Setar timeout para recv (evita bloqueio infinito) */
    netconn_set_recvtimeout(conn, RECV_TIMEOUT_MS);

    for (;;) {
        err = netconn_recv(conn, &buf);

        if (err == ERR_TIMEOUT) {
            /* Timeout ? loop novamente (poderia checar watchdog, sinais, etc.) */
            continue;
        }
        if (err != ERR_OK || buf == NULL) {
            /* erro fatal: log e continue/recuperar */
            osDelay(100);					// evitar tight-loop
            continue;
        }

        /* Obt?m dados e origem */
        if (netbuf_data(buf, &data_ptr, &data_len) != ERR_OK) {
            netbuf_delete(buf);
            continue;
        }
        from_addr = netbuf_fromaddr(buf);
        from_port = netbuf_fromport(buf);

        /* Seguran?a: limitar tamanho */
        if (data_len == 0 || data_len > MAX_UDP_PAYLOAD) {
            netbuf_delete(buf);
            continue;
        }

        /* Copiar para buffer local e garantir termina??o para parsing seguro */
        char local_buf[MAX_UDP_PAYLOAD + 1];
        size_t copy_len = (data_len < MAX_UDP_PAYLOAD) ? data_len : MAX_UDP_PAYLOAD;
        memcpy(local_buf, data_ptr, copy_len);
        local_buf[copy_len] = '\0'; /* agora ? string segura at? copy_len */

        /* Exemplo de valida??o simples: apenas ASCII imprim?vel e \n\r permitidos */
        size_t i;
        int ascii_ok = 1;
        for (i = 0; i < copy_len; ++i) {
            unsigned char c = (unsigned char)local_buf[i];
            if ( (c < 32 || c > 126) && c != '\r' && c != '\n' ) {
                ascii_ok = 0;
                break;
            }
        }
        if (!ascii_ok) {
            netbuf_delete(buf);
            continue;
        }

        /* Parse com base em prefixos ? usar data_len/ copy_len, n?o strlen */
        if (copy_len >= 4 && strncmp(local_buf, "DPS=", 4) == 0) {
            size_t payload_len = copy_len - 4;
            if (payload_len >= RDS_FIELD_SIZE) payload_len = RDS_FIELD_SIZE - 1;

            /* Recomendo: enviar para outra task via fila em vez de setar flags diretas */
            /* Exemplo direto (com prote??o m?nima) */
            memcpy(rds.dps1, &local_buf[4], payload_len);
            rds.dps1[payload_len] = '\0';

            /* Sinalizar com atomic/volatile */
            flagtxtrds = 1;
            flag_udp_rds = 1;

            /* Exemplo: resposta de ACK para o remetente usando sendto */
            const char ack[] = "ACK:DPS\n";
            size_t ack_len = strlen(ack); /* n?mero exato de bytes a enviar */
            struct netbuf *reply = netbuf_new();
            if (reply != NULL) {
            	void *data = netbuf_alloc(reply, (uint16_t)ack_len); 					// aloca espa?o */
                if(data != NULL)  {
                	memcpy(data, ack, ack_len);
                	err_t err_send = netconn_sendto(conn, reply, from_addr, from_port);
                	if (err_send != ERR_OK) {
                		//logI("UDP send failed: %d\n", err_send);
                	}
                } else {
                		//logI("netbuf_alloc failed!\n");
                }
                netbuf_delete(reply);
            }
        }
        else if (copy_len >= 3 && strncmp(local_buf, "RT=", 3) == 0) {
            size_t payload_len = copy_len - 3;
            if (payload_len >= RDS_FIELD_SIZE) payload_len = RDS_FIELD_SIZE - 1;
            memcpy(rds.rt1, &local_buf[3], payload_len);
            rds.rt1[payload_len] = '\0';

            flagtxtrds = 1;
            flag_udp_rds = 2;

            const char ack[] = "ACK:RT\n";
            size_t ack_len = strlen(ack); /* n?mero exato de bytes a enviar */
            struct netbuf *reply = netbuf_new();
            if (reply != NULL) {
            	void *data = netbuf_alloc(reply, (uint16_t)ack_len); 					// aloca espa?o */
                if(data != NULL)  {
                	memcpy(data, ack, ack_len);
                	err_t err_send = netconn_sendto(conn, reply, from_addr, from_port);
                	if (err_send != ERR_OK) {
                		//logI("UDP send failed: %d\n", err_send);
                	}
                } else {
                		//logI("netbuf_alloc failed!\n");
                }
                netbuf_delete(reply);
            }
        } else {
            /* Mensagem desconhecida - opcional: responder com erro ou simplesmente ignorar */
        }

        /* Limpeza sempre */
        netbuf_delete(buf);
    }

    /* Se sair do loop: cleanup */
    // netconn_disconnect(conn); // n?o estritamente necess?rio para UDP
    netconn_delete(conn);
}

/*-----------------------------------------------------------------------------------*/
void udpecho_init(void)
{
	sys_thread_new("udp_thread", udpecho_thread, NULL, 2048, osPriorityLow );
}
