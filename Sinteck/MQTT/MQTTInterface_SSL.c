#include "MQTTInterface.h"

#ifdef STM32H743xx
   #include "stm32h7xx_hal.h"
#else
	#include "stm32f7xx_hal.h"
#endif

#include MBEDTLS_CONFIG_FILE
#include "mbedtls/platform.h"

#include <string.h>
#include "lwip.h"
#include "lwip/api.h"
#include "lwip/sockets.h"

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
#include "mbedtls/memory_buffer_alloc.h"
#endif
#include "mbedtls/net_sockets.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"

#include <string.h>
#include "../Sinteck/src/log_cdc.h"
//#include "../Sinteck/tcp/certificado.h"

extern uint8_t mbedtls_memory_mqtt_buf[];

static mbedtls_net_context server_mqtt_fd;
static const uint8_t *persmqtt = (uint8_t *)("ssl_mqtt");
static int ret;

mbedtls_entropy_context entropy_mqtt;
mbedtls_ctr_drbg_context ctr_drbg_mqtt;
mbedtls_ssl_context ssl_mqtt;
mbedtls_ssl_config conf_mqtt;
mbedtls_x509_crt cacert_mqtt;

int net_init(Network *n) {
	 logI( "\nStart Get MQTT TLS Routine...\n" );

	//initialize mbedTLS realted variables
#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
	mbedtls_memory_buffer_alloc_init(mbedtls_memory_mqtt_buf, sizeof(mbedtls_memory_mqtt_buf));
#endif

	mbedtls_ssl_init(&ssl_mqtt);
	mbedtls_ssl_config_init(&conf_mqtt);
	mbedtls_x509_crt_init(&cacert_mqtt);
	mbedtls_ctr_drbg_init(&ctr_drbg_mqtt);

	logI( "Seeding the random number generator...\n" );

	mbedtls_entropy_init(&entropy_mqtt);
	if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg_mqtt, mbedtls_entropy_func, &entropy_mqtt, (const unsigned char*) persmqtt,
			strlen((char *)persmqtt))) != 0) {
		logI( " failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret );
		return -1;
	}

	//register functions
	n->mqttread = net_read; //receive function
	n->mqttwrite = net_write; //send function
	n->disconnect = net_disconnect; //disconnection function

	return 0;
}

int net_connect(Network *n, char *ip, char *port) {
	int ret;

	logI( "  . Loading the CA root certificate ...\n" );
	// SSL/TLS connection process. refer to ssl client1 example
	ret = mbedtls_x509_crt_parse( &cacert_mqtt, (const unsigned char *) mbedtls_root2_certificate, mbedtls_root_certificate_len );

	if (ret < 0) {
		logI("mbedtls_x509_crt_parse failed.\n");
		return -1;
	}
	logI( "  . Connecting to %s:%s ...\n", ip, port );

	ret = mbedtls_net_connect(&server_mqtt_fd, ip, port,	MBEDTLS_NET_PROTO_TCP);
	if (ret < 0) {
		logI("mbedtls_net_connect failed. 0x%X\n", ret);
		return -1;
	}

	ret = mbedtls_ssl_config_defaults(&conf_mqtt, MBEDTLS_SSL_IS_CLIENT,
	MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
	if (ret < 0) {
		logI("mbedtls_ssl_config_defaults failed.\n");
		return -1;
	}

	mbedtls_ssl_conf_authmode(&conf_mqtt, MBEDTLS_SSL_VERIFY_OPTIONAL);
	mbedtls_ssl_conf_ca_chain(&conf_mqtt, &cacert_mqtt, NULL);
	mbedtls_ssl_conf_rng(&conf_mqtt, mbedtls_ctr_drbg_random, &ctr_drbg_mqtt);
	//mbedtls_ssl_conf_dbg(&conf, my_debug, stdout);

	ret = mbedtls_ssl_setup(&ssl_mqtt, &conf_mqtt);
	if (ret < 0) {
		logI("mbedtls_ssl_setup failed.\n");
		return -1;
	}

	ret = mbedtls_ssl_set_hostname(&ssl_mqtt, ip);
	if (ret < 0) {
		logI("mbedtls_ssl_set_hostname failed.\n");
		return -1;
	}

	mbedtls_ssl_set_bio(&ssl_mqtt, &server_mqtt_fd, mbedtls_net_send, mbedtls_net_recv,
	NULL);

	while ((ret = mbedtls_ssl_handshake(&ssl_mqtt)) != 0) {
		if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			logI("mbedtls_ssl_handshake failed.\n");
			return -1;
		}
	}

	ret = mbedtls_ssl_get_verify_result(&ssl_mqtt);
	if (ret < 0) {
		logI("mbedtls_ssl_get_verify_result failed.\n");
		return -1;
	}

	return 0;
}

int net_read(Network *n, unsigned char *buffer, int len, int timeout_ms) {
	int ret;
	int received = 0;
	int error = 0;
	int complete = 0;

	//set timeout
	if (timeout_ms != 0) {
		mbedtls_ssl_conf_read_timeout(&conf_mqtt, timeout_ms);
	}

	//read until received length is bigger than variable len
	do {
		ret = mbedtls_ssl_read(&ssl_mqtt, buffer, len);
		if (ret > 0) {
			received += ret;
		} else if (ret != MBEDTLS_ERR_SSL_WANT_READ) {
			error = 1;
		}
		if (received >= len) {
			complete = 1;
		}
	} while (!error && !complete);

	return received;
}

int net_write(Network *n, unsigned char *buffer, int len, int timeout_ms) {
	int ret;
	int written;

	//check all bytes are written
	for (written = 0; written < len; written += ret) {
		while ((ret = mbedtls_ssl_write(&ssl_mqtt, buffer + written, len - written)) <= 0) {
			if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
				return ret;
			}
		}
	}

	return written;
}

void net_disconnect(Network *n) {
	int ret;

	do {
		ret = mbedtls_ssl_close_notify(&ssl_mqtt);
	} while (ret == MBEDTLS_ERR_SSL_WANT_WRITE);

	mbedtls_ssl_session_reset(&ssl_mqtt);
	mbedtls_net_free(&server_mqtt_fd);
}

void net_clear() {
	mbedtls_net_free(&server_mqtt_fd);
	mbedtls_x509_crt_free(&cacert_mqtt);
	mbedtls_ssl_free(&ssl_mqtt);
	mbedtls_ssl_config_free(&conf_mqtt);
	mbedtls_ctr_drbg_free(&ctr_drbg_mqtt);
	mbedtls_entropy_free(&entropy_mqtt);

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
	mbedtls_memory_buffer_alloc_free();
#endif
}

uint32_t MilliTimer;

//Timer functions
char TimerIsExpired(Timer *timer) {
	long left = timer->end_time - MilliTimer;
	return (left < 0);
}

void TimerCountdownMS(Timer *timer, unsigned int timeout) {
	timer->end_time = MilliTimer + timeout;
}

void TimerCountdown(Timer *timer, unsigned int timeout) {
	timer->end_time = MilliTimer + timeout;
}

int TimerLeftMS(Timer *timer) {
	long left = timer->end_time - MilliTimer;
	return (left < 0) ? 0 : left;
}

void TimerInit(Timer *timer) {
	timer->end_time = 0;
}

