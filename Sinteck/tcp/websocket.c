/*
 * websocket.c
 *
 *  Created on: Mar 3, 2021
 *      Author: Rinaldo Dos Santos
 *      Sinteck Next
 */

#include "main.h"
#include "rng.h"
#include "cmsis_os.h"
#include <string.h>
#include "lwip/sys.h"
#include "lwip/api.h"

#include <mbedtls_config.h>
#include "mbedtls.h"
#include "mbedtls/platform.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include "mbedtls/memory_buffer_alloc.h"
#include "mbedtls/base64.h"
#include "mbedtls/sha1.h"

#include "../Sinteck/tcp/websocket.h"
#include "../Sinteck/tcp/certificado.h"
#include "../Sinteck/tcp/json_util.h"
#include "../Sinteck/tcp/WebSocket_Interface.h"

static mbedtls_net_context server_fd;
static uint32_t flags;
static uint8_t buf_websocket[4096] = {0};
static uint8_t buf_rxwebsocket[4096] = {0};
static uint8_t buf_rinaldo[4096] = {0};
int len_rxwebsocket;
static const uint8_t *pers = (uint8_t *)("ssl_client");
static uint8_t vrfy_buf_websocket[1024] = {0};
extern char buf_json[4096];
size_t sz;
uint8_t str_uuid[64];
uint8_t base64[64];
char ws_key[96];
char ws_ori[32];
char ws_token[32];
char ws_version[48];
char ws_uuid[112];
uint32_t tmr_send_websocket = 0;

osThreadId_t TaskWebSocketHandle;

static int ret;
static int status_ws = -1;
int tx_ssl;
size_t size_ssl;

extern uint8_t token_model[];
extern uint8_t uuid_model[];
extern const char* versao;
extern uint8_t IP_ADDRESS[];
extern char buff[];
extern uint8_t Telemetry_State;

extern mbedtls_entropy_context entropy;
extern mbedtls_ctr_drbg_context ctr_drbg;
extern mbedtls_ssl_context ssl;
extern mbedtls_ssl_config conf;
extern mbedtls_x509_crt cacert;

extern uint8_t mbedtls_memory_buf[];

typedef struct _wsclient_frame {
	unsigned int fin;
	unsigned int opcode;
	unsigned int mask_offset;
	unsigned int payload_offset;
	unsigned int rawdata_idx;
	unsigned int rawdata_sz;
	unsigned long long payload_len;
	char *rawdata;
	struct _wsclient_frame *next_frame;
	struct _wsclient_frame *prev_frame;
	unsigned char mask[4];
} wsclient_frame;

uint8_t teste_websocket[10] = {0};

void close_ws(void)
{
	  mbedtls_ssl_close_notify( &ssl );
	  mbedtls_net_free( &server_fd );
	  mbedtls_x509_crt_free( &cacert );
	  mbedtls_ssl_free( &ssl );
	  mbedtls_ssl_config_free( &conf );
	  mbedtls_ctr_drbg_free( &ctr_drbg );
	  mbedtls_entropy_free( &entropy );
}

void Websocket_TX(void *arg)
{
	while(1) {
		if(HAL_GetTick() - tmr_send_websocket > 1000) {
			tmr_send_websocket = HAL_GetTick();
			if(status_ws == 0 ) {
				websocket_send("{ temperature: 20.5 }");
			}
		}
		osDelay(10);
	}
}

void Websocket_Start(void *arg)
{
	int len;
	/*
	 * 0. Initialize the RNG and the session data
	 */
	TaskWebSocketHandle = xTaskGetCurrentTaskHandle();

#ifdef MBEDTLS_MEMORY_BUFFER_ALLOC_C
	mbedtls_memory_buffer_alloc_init(mbedtls_memory_buf, sizeof(mbedtls_memory_buf));
#endif
	mbedtls_ssl_init(&ssl);
	mbedtls_ssl_config_init(&conf);
	mbedtls_x509_crt_init(&cacert);
	mbedtls_ctr_drbg_init(&ctr_drbg);

	mbedtls_entropy_init( &entropy );
	len = strlen((char *)pers);
	if( ( ret = mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *) pers, len ) ) != 0 ) {
	}

	/*
	 * 1. Initialize certificates
	 */
	ret = mbedtls_x509_crt_parse( &cacert, (const unsigned char *) mbedtls_root2_certificate, mbedtls_root_certificate_len );
	if( ret < 0 ) {
		close_ws();
	}

	/*
	 * 2. Start the connection
	 */
	if( ( ret = mbedtls_net_connect( &server_fd, SERVER_NAME_W, SERVER_PORT_W, MBEDTLS_NET_PROTO_TCP ) ) != 0 ) {
	}

	/*
	 * 3. Setup stuff
	 */
	if( ( ret = mbedtls_ssl_config_defaults( &conf,
					MBEDTLS_SSL_IS_CLIENT,
                    MBEDTLS_SSL_TRANSPORT_STREAM,
					MBEDTLS_SSL_PRESET_DEFAULT ) ) != 0 )
	{
	}

	/* OPTIONAL is not optimal for security,
	 * but makes interop easier in this simplified example */
	//mbedtls_ssl_conf_authmode( &conf, MBEDTLS_SSL_VERIFY_OPTIONAL );
	mbedtls_ssl_conf_authmode( &conf, MBEDTLS_SSL_VERIFY_NONE );
	mbedtls_ssl_conf_ca_chain( &conf, &cacert, NULL );
	mbedtls_ssl_conf_rng( &conf, mbedtls_ctr_drbg_random, &ctr_drbg );

	if( ( ret = mbedtls_ssl_setup( &ssl, &conf ) ) != 0 ) {
	}

	if( ( ret = mbedtls_ssl_set_hostname( &ssl, SERVER_NAME_W ) ) != 0 ) {
	}

	mbedtls_ssl_set_bio( &ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL );

	/*
	 * 4. Handshake
	 */
	while( ( ret = mbedtls_ssl_handshake( &ssl ) ) != 0 ) {
		if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE ) {
		}
	}

	//
	// 5. Verify the server certificate
	//
	if( ( flags = mbedtls_ssl_get_verify_result( &ssl ) ) != 0 ) {
		mbedtls_x509_crt_verify_info( (char *)vrfy_buf_websocket, sizeof( vrfy_buf_websocket ), "  ! ", flags );
	}

	// Connection WebSocket
	ret = ws_handshake();
	if(ret != 0) {
		status_ws = 0;
	}
	else {
		status_ws = -1;
		mbedtls_ssl_close_notify( &ssl );

		mbedtls_net_free( &server_fd );

		mbedtls_x509_crt_free( &cacert );
		mbedtls_ssl_free( &ssl );
		mbedtls_ssl_config_free( &conf );
		mbedtls_ctr_drbg_free( &ctr_drbg );
		mbedtls_entropy_free( &entropy );
	}

	if ((ret < 0) && (ret != MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)) {
		vTaskDelete(TaskWebSocketHandle); //Delete the start task
	}
	else {
	}

	//
	while(1){
		  do {
			  len = sizeof( buf_rxwebsocket ) - 1;
			  memset( buf_rxwebsocket, 0, sizeof( buf_rxwebsocket ) );
			  ret = mbedtls_ssl_read( &ssl, buf_rxwebsocket, len );

			  if( ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE ) {
				  continue;
			  }

			  if( ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY ) {
				  break;
			  }

			  if( ret < 0 ) {
				  ret = -1;
				  break;
			  }

			  if( ret == 0 ) {
				  ret = -1;
				  break;
			  }
			  //
			  len = ret;
			  memset(buf_rinaldo, 0, 4096);
			  memcpy(buf_rinaldo, buf_rxwebsocket, len);

			  memcpy(buf_rinaldo, &buf_rxwebsocket[2], buf_rxwebsocket[1]);

		   }  while( 1 );

		osDelay(10);
	}
}

int send_ws(char * message)
{
	int len;

	sprintf( (char *) buf_websocket, message );
	len = strlen((char *) buf_websocket);

	while( ( ret = mbedtls_ssl_write( &ssl, buf_websocket, len ) ) <= 0 )
	{
		if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
		{
		  return -1;
		}
	}

	len = ret;
	return 0;
}

int ws_handshake(void)
{
	int len;

	sprintf( (char *)str_uuid, "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
	 		  uuid_model[0], uuid_model[1],uuid_model[2],uuid_model[3],uuid_model[4],uuid_model[5],uuid_model[6],uuid_model[7],
	 		  uuid_model[8], uuid_model[9],uuid_model[10],uuid_model[11],uuid_model[12],uuid_model[13],uuid_model[14],uuid_model[15]
			  );
	mbedtls_base64_encode(base64, 64, &sz, str_uuid, 36 );

	sprintf(ws_key, "Sec-WebSocket-Key: %s\r\n", base64);
	sprintf(ws_ori, "Origin: %d.%d.%d.%d\r\n", IP_ADDRESS[0], IP_ADDRESS[1], IP_ADDRESS[2], IP_ADDRESS[3]);
	sprintf(ws_token, "X-Access-Token: %c%c%c%c%c%c%c%c%c\r\n", token_model[0], token_model[1], token_model[2],token_model[3],
			  	  	  	  	  	  	  	  	  	  	  	  	  	'-',
																token_model[4], token_model[5], token_model[6], token_model[7]);

	sprintf(ws_version, "X-Xt-Version: %s\r\n", versao);
	sprintf(ws_uuid, "X-Uuid: %s\r\n", str_uuid);

	sprintf( (char *) buf_websocket, "%s%s%s%s%s%s%s%s%s%s\n\r",
			          "GET /api/v1/ws HTTP/1.1\r\n",
					  "Host: xt.sinteck.com.br\r\n",
					  "Upgrade: WebSocket\r\n",
					  "Connection: Upgrade\r\n",
					  ws_key,
					  ws_ori,
					  "User-Agent: XT Sinteck\r\n",
					  ws_token,
					  ws_version,
					  ws_uuid
					  );

	  len = strlen((char *) buf_websocket);

	  while( ( ret = mbedtls_ssl_write( &ssl, buf_websocket, len ) ) <= 0 ) {
		  if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE ) {
			  return -1;
		  }
	  }

	  len = ret;
	  return ret;
}

int websocket_send(char *strdata)
{
	unsigned char mask[4];
	uint32_t mask_int;
	unsigned long long payload_len;
	unsigned char finNopcode;
	unsigned int payload_len_small;
	unsigned int payload_offset = 6;
	unsigned int len_size;
	unsigned int sent = 0;
	int i;
	unsigned int frame_size;
	char *data;


	finNopcode = 0x81; //FIN and text opcode.
	HAL_RNG_GenerateRandomNumber(&hrng, &mask_int);
	memcpy(mask, &mask_int, 4);
	payload_len = strlen(strdata);
	if(payload_len <= 125) {
		frame_size = 6 + payload_len;
		payload_len_small = payload_len;

	}
	else if(payload_len > 125 && payload_len <= 0xffff) {
		frame_size = 8 + payload_len;
		payload_len_small = 126;
		payload_offset += 2;
	}
	else if(payload_len > 0xffff && payload_len <= 0xffffffffffffffffLL) {
		frame_size = 14 + payload_len;
		payload_len_small = 127;
		payload_offset += 8;
	}
	else {
		return -1; //too large payload
	}
	data = (char *)malloc(frame_size);
	memset(data, 0, frame_size);
	*data = finNopcode;
	*(data+1) = payload_len_small | 0x80; //payload length with mask bit on
	if(payload_len_small == 126) {
		payload_len &= 0xffff;
		len_size = 2;
		for(i = 0; i < len_size; i++) {
			*(data+2+i) = *((char *)&payload_len+(len_size-i-1));
		}
	}
	if(payload_len_small == 127) {
		payload_len &= 0xffffffffffffffffLL;
		len_size = 8;
		for(i = 0; i < len_size; i++) {
			*(data+2+i) = *((char *)&payload_len+(len_size-i-1));
		}
	}
	for(i=0;i<4;i++)
		*(data+(payload_offset-4)+i) = mask[i];

	memcpy(data+payload_offset, strdata, strlen(strdata));
	for(i=0;i<strlen(strdata);i++)
		*(data+payload_offset+i) ^= mask[i % 4] & 0xff;
	sent = 0;
	i = 0;

	while(sent < frame_size && i >= 0) {
		i = mbedtls_ssl_write( &ssl, (const unsigned char *)data+sent, frame_size-sent );
		sent += i;
	}
	return 0;
}

void websocket_init(void)
{
	sys_thread_new("Websocket_Irx", Websocket_Start, NULL, 3072, osPriorityLow);
	//sys_thread_new("Websocket_TX",  Websocket_TX, NULL, 3072, osPriorityLow);
}
