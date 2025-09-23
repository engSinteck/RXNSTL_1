 /**
  *
  *  Portions COPYRIGHT 2016 STMicroelectronics
  *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
  *
  ******************************************************************************
  * @file    ssl_client.c
  * @author  MCD Application Team
  * @brief   SSL client application
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include <mbedtls_config.h>
#endif

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdio.h>
#include <stdlib.h>
#define mbedtls_time       time 
#define mbedtls_time_t     time_t
#define mbedtls_fprintf    fprintf
#define logI     printf
#endif


#if !defined(MBEDTLS_BIGNUM_C) || !defined(MBEDTLS_ENTROPY_C) ||  \
    !defined(MBEDTLS_SSL_TLS_C) || !defined(MBEDTLS_SSL_CLI_C) || \
    !defined(MBEDTLS_NET_C) || !defined(MBEDTLS_RSA_C) ||         \
    !defined(MBEDTLS_CERTS_C) || !defined(MBEDTLS_PEM_PARSE_C) || \
    !defined(MBEDTLS_CTR_DRBG_C) || !defined(MBEDTLS_X509_CRT_PARSE_C)
#endif
#if 0
int main( void )
{

    logI("MBEDTLS_BIGNUM_C and/or MBEDTLS_ENTROPY_C and/or "
           "MBEDTLS_SSL_TLS_C and/or MBEDTLS_SSL_CLI_C and/or "
           "MBEDTLS_NET_C and/or MBEDTLS_RSA_C and/or "
           "MBEDTLS_CTR_DRBG_C and/or MBEDTLS_X509_CRT_PARSE_C "
           "not defined.\n");

    return( 0 );
}
#else

#include "mbedtls/net_sockets.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include "mbedtls/memory_buffer_alloc.h"

#include "main.h"
#include "cmsis_os.h"

#include <string.h>
#include "mbedtls.h"
#include "lwip/sys.h"
#include "lwip/api.h"

#include "../Sinteck/tcp/ssl_client.h"
#include "../Sinteck/tcp/certificado.h"

extern Cfg_var cfg;

static mbedtls_net_context server_fd;
static uint32_t flags;
static uint8_t buf[2048];
static const uint8_t *pers = (uint8_t *)("ssl_token");
static uint8_t vrfy_buf[1024];

static int ret;

extern mbedtls_entropy_context entropy;
extern mbedtls_ctr_drbg_context ctr_drbg;
extern mbedtls_ssl_context ssl;
extern mbedtls_ssl_config conf;
mbedtls_x509_crt cacert;

extern uint8_t mbedtls_memory_buf[];

int Get_Token(void)
{
  int len;

  /*
   * 0. Initialize the RNG and the session data
   */

#ifdef MBEDTLS_MEMORY_BUFFER_ALLOC_C
  mbedtls_memory_buffer_alloc_init(mbedtls_memory_buf, sizeof(mbedtls_memory_buf));
#endif
  mbedtls_ssl_init(&ssl);
  mbedtls_ssl_config_init(&conf);
  mbedtls_x509_crt_init(&cacert);
  mbedtls_ctr_drbg_init(&ctr_drbg);


  mbedtls_entropy_init( &entropy );
  len = strlen((char *)pers);
  if( ( ret = mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func, &entropy,
                             (const unsigned char *) pers, len ) ) != 0 )
  {
    goto exit;
  }

  /*
   * 0. Initialize certificates
   */
  ret = mbedtls_x509_crt_parse( &cacert, (const unsigned char *) mbedtls_root2_certificate,
  		  	  	  	  	  mbedtls_root_certificate_len );
  if( ret < 0 )
  {
    goto exit;
  }

  /*
   * 1. Start the connection
   */
  if( ( ret = mbedtls_net_connect( &server_fd, SERVER_NAME,
                                       SERVER_PORT, MBEDTLS_NET_PROTO_TCP ) ) != 0 )
  {
    goto exit;
  }

  /*
   * 2. Setup stuff
   */
  if( ( ret = mbedtls_ssl_config_defaults( &conf,
                  MBEDTLS_SSL_IS_CLIENT,
                  MBEDTLS_SSL_TRANSPORT_STREAM,
                  MBEDTLS_SSL_PRESET_DEFAULT ) ) != 0 )
  {
    goto exit;
  }

  /* OPTIONAL is not optimal for security,
   * but makes interop easier in this simplified example */
  //mbedtls_ssl_conf_authmode( &conf, MBEDTLS_SSL_VERIFY_OPTIONAL );
  mbedtls_ssl_conf_authmode( &conf, MBEDTLS_SSL_VERIFY_NONE );
  mbedtls_ssl_conf_ca_chain( &conf, &cacert, NULL );
  mbedtls_ssl_conf_rng( &conf, mbedtls_ctr_drbg_random, &ctr_drbg );

  if( ( ret = mbedtls_ssl_setup( &ssl, &conf ) ) != 0 )
  {
    goto exit;
  }

  if( ( ret = mbedtls_ssl_set_hostname( &ssl, "xt.sinteck.com.br" ) ) != 0 )
  {
    goto exit;
  }

  mbedtls_ssl_set_bio( &ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL );

  /*
   * 4. Handshake
   */
  while( ( ret = mbedtls_ssl_handshake( &ssl ) ) != 0 )
  {
    if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
    {
      goto exit;
    }
  }

  /*
   * 5. Verify the server certificate
   */
  if( ( flags = mbedtls_ssl_get_verify_result( &ssl ) ) != 0 )
  {

    mbedtls_x509_crt_verify_info( (char *)vrfy_buf, sizeof( vrfy_buf ), "  ! ", flags );
  }
  else
  {
  }

  /*
   * 6. Write the GET request
   */

  sprintf( (char *) buf, "%s%s%s%s%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X\r\n\r\n",
 		  "GET /api/v1/get_token HTTP/1.1\r\n",
 		  "Host: xt.sinteck.com.br\r\n",
 		  "User-Agent: XT Sinteck\r\n",
 	      "X-Uuid: ",
 		  cfg.uuid[0], cfg.uuid[1], cfg.uuid[2], cfg.uuid[3], cfg.uuid[4], cfg.uuid[5], cfg.uuid[6], cfg.uuid[7],
		  cfg.uuid[8], cfg.uuid[9], cfg.uuid[10],cfg.uuid[11],cfg.uuid[12],cfg.uuid[13],cfg.uuid[14],cfg.uuid[15]
 		);
  len = strlen((char *) buf);

  while( ( ret = mbedtls_ssl_write( &ssl, buf, len ) ) <= 0 )
  {
    if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
    {
      goto exit;
    }
  }

  len = ret;

  /*
   * 7. Read the HTTP response
   */
  do {
    len = sizeof( buf ) - 1;
    memset( buf, 0, sizeof( buf ) );
    ret = mbedtls_ssl_read( &ssl, buf, len );

    if( ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE )
    {
      continue;
    }

    if( ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY )
    {
      break;
    }

    if( ret < 0 )
    {
      break;
    }

    if( ret == 0 )
    {
      break;
    }

    len = ret;
    if((strncmp((char *) buf, "HTTP/1.1 200", 12) == 0)) {
    	cfg.Token[0] = buf[len-9];
    	cfg.Token[1] = buf[len-8];
    	cfg.Token[2] = buf[len-7];
    	cfg.Token[3] = buf[len-6];
    	cfg.Token[4] = buf[len-4];
    	cfg.Token[5] = buf[len-3];
    	cfg.Token[6] = buf[len-2];
    	cfg.Token[7] = buf[len-1];
    	cfg.Token[8] = 0;
    	cfg.Token[12] = 1;
    	break;
    }
    else if((strncmp((char *) buf, "HTTP/1.1 304 Not Modified", 25) == 0)) {
    	cfg.Token[12] = 2;
    	break;
    }
    else if((strncmp((char *) buf, "HTTP/1.1 502 Bad Gateway", 24) == 0)) {
    	cfg.Token[12] = 2;
    	break;
    }
  }  while( 1 );

  mbedtls_ssl_close_notify( &ssl );

exit:
  mbedtls_net_free( &server_fd );

  mbedtls_x509_crt_free( &cacert );
  mbedtls_ssl_free( &ssl );
  mbedtls_ssl_config_free( &conf );
  mbedtls_ctr_drbg_free( &ctr_drbg );
  mbedtls_entropy_free( &entropy );
#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
	mbedtls_memory_buffer_alloc_free();
#endif

  if ((ret < 0) && (ret != MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY))
  {
  }
  else
  {
  }
  return ret;
}

#endif /* MBEDTLS_BIGNUM_C && MBEDTLS_ENTROPY_C && MBEDTLS_SSL_TLS_C &&
          MBEDTLS_SSL_CLI_C && MBEDTLS_NET_C && MBEDTLS_RSA_C &&
          MBEDTLS_CERTS_C && MBEDTLS_PEM_PARSE_C && MBEDTLS_CTR_DRBG_C &&
          MBEDTLS_X509_CRT_PARSE_C */
