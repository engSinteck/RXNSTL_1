/**
  ******************************************************************************
  * @file    LwIP/LwIP_MQTT Client
  * @author  MCD Application Team
  * @brief   Basic MQTT Client implementation using LwIP API
  ******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "stdio.h"
#include "string.h"
#include "lwip/api.h"
#include "lwip/ip4_addr.h"
#include "lwip/altcp_tls.h"
#include "lwip/apps/mqtt.h"
#include "lwip/apps/mqtt_opts.h"
#include "lwip/apps/mqtt_priv.h"
#include "../cJSON/lwjson.h"
#include "../Sinteck/tcp/tcp_client.h"
#include "../Sinteck/tcp/json_util.h"
#include "../Sinteck/src/defines.h"

#include "mqtt_client.h"

extern void strstr_substring(const char *src, const char *delim1, const char *delim2, int pos);
extern char out[];

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define MQTT_CLIENT_THREAD_PRIO    ( osPriorityLow )

/* Private macro -------------------------------------------------------------*/
extern uint8_t token_model[];
extern uint8_t uuid_model[];
extern uint8_t str_uuid[];
extern uint8_t Telemetry_State;
extern char buff[];
extern uint32_t timer_reflesh;
extern uint8_t flag_telemetry;
char rasc_mqtt[100] = { 0 };

/* Private variables ---------------------------------------------------------*/
//static uint32_t nPubCounter = 0;
//static char pub_payload[40];
static uint8_t mqtt_is_prime = 0;
static uint32_t timer_mqtt = 0;
static mqtt_client_t* mqtt_client;
static char topic_in[50] = { 0 };
static uint32_t len_topic_in = 0;

/* LwJSON instance and tokens */
static lwjson_token_t tokens[128];
static lwjson_t lwjson;

/* Private function prototypes -----------------------------------------------*/
static void example_connect(mqtt_client_t *client, const char *topic);
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);
static void mqtt_sub_request_cb(void *arg, err_t result);
static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len);
static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags);
static void mqtt_pub_request_cb(void *arg, err_t result);
static void example_publish(mqtt_client_t *client, void *arg);
static void example_publish_config(mqtt_client_t *client, void *arg);
static void example_publish_config(mqtt_client_t *client, void *arg);
//static void example_publish_teste(mqtt_client_t *client, void *arg);
static void example_disconnect(mqtt_client_t *client);
static void process_mqtt_data(const uint8_t *data, uint16_t len);
static void mqtt_client_thread(void *arg);

/* Private functions ---------------------------------------------------------*/

/**
 * Establish connection to MQTT sever
 */
static void example_connect(mqtt_client_t *client, const char *topic)
{
  struct mqtt_connect_client_info_t ci;
  err_t err;

  /* Setup an empty client info structure */
  memset(&ci, 0, sizeof(ci));

  /* Minimal amount of information required is client identifier, so set it here */
  /* Set client information */
  sprintf( (char *)str_uuid, "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X%c",
  		  uuid_model[0], uuid_model[1],uuid_model[2],uuid_model[3],uuid_model[4],uuid_model[5],uuid_model[6],uuid_model[7],
  		  uuid_model[8], uuid_model[9],uuid_model[10],uuid_model[11],uuid_model[12],uuid_model[13],uuid_model[14],uuid_model[15],
 		  '\0');
  memcpy((char *)ci.client_id, str_uuid, strlen((char *)str_uuid)+1);
  ci.client_user = "";
  ci.client_pass = "";
  ci.keep_alive = 60;
  ci.will_topic = NULL;
  ci.will_msg = NULL;
  ci.will_retain = 0;
  ci.will_qos = 0;
  ci.tls_config = NULL;

  ip4_addr_t mqttServerIP;
  IP4_ADDR(&mqttServerIP, 3, 23, 178, 219);

  /* Initiate client and connect to server, if this fails immediately an error code is returned
     otherwise mqtt_connection_cb will be called with connection result after attempting
     to establish a connection with the server.
     For now MQTT version 3.1.1 is always used */
  err = mqtt_client_connect(client, &mqttServerIP, MQTT_PORT, mqtt_connection_cb, (uint8_t *)topic, &ci);

  /* For now just print the result code if something goes wrong */
  if(err != ERR_OK) {
   // logI("mqtt_connect return %d\n", err);
  }
}

/**
 * Connection callback -- simply report status
 */
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
	err_t err;
	char topico[20] = { 0 };

	if(status == MQTT_CONNECT_ACCEPTED) {
		Telemetry_State = 1;
		mqtt_is_prime = 0;

		/* Setup callback for incoming publish requests */
		mqtt_set_inpub_callback(client, mqtt_incoming_publish_cb, mqtt_incoming_data_cb, arg);

	    sprintf(topico, "cmd/%c%c%c%c%c%c%c%c%c/#", token_model[0], token_model[1], token_model[2], token_model[3],
			  			  	  	  	  	  	  	  '-',
			                                      token_model[4], token_model[5], token_model[6], token_model[7]);

		/* Subscribe to a topic named "sub_topic" with QoS level 1, call mqtt_sub_request_cb with result */
		err = mqtt_subscribe(client, topico, 1, mqtt_sub_request_cb, arg);

		if(err != ERR_OK) {
		}
	}
	else {
		Telemetry_State = 0;
		mqtt_is_prime = 0;

		/* Its more nice to be connected, so try to reconnect */
		example_connect(client, "Sinteck");
	}
}

/**
 * Subscription Callback
 */
static void mqtt_sub_request_cb(void *arg, err_t result)
{
  /* Just print the result code here for simplicity,
     normal behavior would be to take some action if subscribe fails like
     notifying user, retry subscribe or disconnect from server */
}


/* The idea is to demultiplex topic and create some reference to be used in data callbacks
   Example here uses a global variable, better would be to use a member in arg
   If RAM and CPU budget allows it, the easiest implementation might be to just take a copy of
   the topic string and use it in mqtt_incoming_data_cb
*/
static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len)
{
//  logI("Incoming publish arg: %s at topic %s with total length %u\n", arg, topic, (unsigned int)tot_len);
	len_topic_in = strlen(topic);
	memcpy((char *)topic_in, (char *)topic, len_topic_in);
}

static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
  //logI("Incoming publish payload Data: %s with length %d, flags %u\n", data, len, (unsigned int)flags);

  if(flags & MQTT_DATA_FLAG_LAST) {
    /* Last fragment of payload received (or whole part if payload fits receive buffer
       See MQTT_VAR_HEADER_BUFFER_LEN)  */
	  	process_mqtt_data(data, len);
    /* Call function or do action depending on reference, in this case inpub_id */

	}
}

/**
 * Publish complete Callback
 */
static void mqtt_pub_request_cb(void *arg, err_t result)
{
  if(result != ERR_OK) {
  }
}

/**
 * Publish Data
 */
static void example_publish(mqtt_client_t *client, void *arg)
{
	  err_t err;
	  char topico[20] = { 0 };
	  u8_t qos = 0; /* 0 1 or 2, see MQTT specification */
	  u8_t retain = 0; /* No don't retain the payload... */

	  sprintf(topico, "reading/%c%c%c%c%c%c%c%c%c", token_model[0], token_model[1], token_model[2], token_model[3],
			  	  	  	  	  	  	  	  	  	    '-',
                                                    token_model[4], token_model[5], token_model[6], token_model[7]);

	  err = mqtt_publish(client, topico, buff, strlen(buff), qos, retain, mqtt_pub_request_cb, arg);

	  if(err != ERR_OK) {
	    example_disconnect(client);
	  }
}

/**
 * Publish Config
 */
static void example_publish_config(mqtt_client_t *client, void *arg)
{
	  err_t err;
	  char topico[20] = { 0 };
	  u8_t qos = 1; /* 0 1 or 2, see MQTT specification */
	  u8_t retain = 0; /* No don't retain the payload... */

	  sprintf(topico, "config/%c%c%c%c%c%c%c%c%c", token_model[0], token_model[1], token_model[2], token_model[3],
	  			  	  	  	  	  	  	  	  	  	'-',
	                                                token_model[4], token_model[5], token_model[6], token_model[7]);

	  err = mqtt_publish(client, topico, buff, strlen(buff), qos, retain, mqtt_pub_request_cb, arg);

	  if(err != ERR_OK) {
	    example_disconnect(client);
	  }
}

/**
 * Publish Preset
 */
static void example_publish_preset(mqtt_client_t *client, void *arg)
{
	  err_t err;
	  char topico[20] = { 0 };
	  u8_t qos = 1; /* 0 1 or 2, see MQTT specification */
	  u8_t retain = 0; /* No don't retain the payload... */

	  sprintf(topico, "preset/%c%c%c%c%c%c%c%c%c", token_model[0], token_model[1], token_model[2], token_model[3],
		  			  	  	  	  	  	  	  	   '-',
		                                           token_model[4], token_model[5], token_model[6], token_model[7]);

	  err = mqtt_publish(client, topico, buff, strlen(buff), qos, retain, mqtt_pub_request_cb, arg);

	  if(err != ERR_OK) {
	    example_disconnect(client);
	  }
}

/**
 * Disconnect from server
 */
static void example_disconnect(mqtt_client_t *client)
{
	Telemetry_State = 0;
	mqtt_is_prime = 0;
	mqtt_disconnect(client);
}

/**
 * Process MQTT Data
 */
uint8_t var[50] = { 0 };
lwjsonr_t mqtt_res;
const lwjson_token_t* t;
static void process_mqtt_data(const uint8_t *data, uint16_t len)
{
	uint8_t token[10] = { 0 };

	memset(var, 0, 50);
	memcpy(var, data, len);
	//logI("Process MQTT Topic: %s, Size: %ld Data: %s with length %d\n", topic_in, len_topic_in, var, len);

	token[0] = topic_in[4];
	token[1] = topic_in[5];
	token[2] = topic_in[6];
	token[3] = topic_in[7];
	token[4] = topic_in[8];
	token[5] = topic_in[9];
	token[6] = topic_in[10];
	token[7] = topic_in[11];
	token[8] = topic_in[12];


	if(token[0] == token_model[0] && token[1] == token_model[1] && token[2] == token_model[2] && token[3] == token_model[3] &&
	   token[4] == '-' &&
	   token[5] == token_model[4] && token[6] == token_model[5] && token[7] == token_model[6] && token[8] == token_model[7] ) {
		//
		if (strncmp((char const *)&topic_in[14], "rfstate", 7) == 0) {
			// decodifica JSON
			lwjson_init(&lwjson, tokens, LWJSON_ARRAYSIZE(tokens));
			mqtt_res = lwjson_parse(&lwjson, (const char *)var);
			if (mqtt_res == lwjsonOK) {
		        /* Find custom key in JSON */
		        if ((t = lwjson_find(&lwjson, "state")) != NULL) {
		        	// RFENABLE
		        	if(t->u.num_int == 0) {

		        	}
		        	if(t->u.num_int == 1) {

		        	}
		        	if(t->u.num_int == 2) {

		        	}
		        }
		        /* Call this when not used anymore */
		        lwjson_free(&lwjson);
			}
		}
		// Teste JSON String
		else if (strncmp((char const *)&topic_in[14], "preset", 6) == 0) {
			// decodifica JSON
			lwjson_init(&lwjson, tokens, LWJSON_ARRAYSIZE(tokens));
			mqtt_res = lwjson_parse(&lwjson, (const char *)var);
			if (mqtt_res == lwjsonOK) {
		        /* Find custom key in JSON */
		        if ((t = lwjson_find(&lwjson, "Name")) != NULL) {
		        	memset(rasc_mqtt, 0, 100);
		        	size_t len_str = lwjson_get_val_string_length(t);
		        	lwjson_get_val_string(t, &len_str);
		        	memcpy(rasc_mqtt, lwjson_get_val_string(t, &len_str), lwjson_get_val_string_length(t));
		        }
		        //
		        if ((t = lwjson_find(&lwjson, "FWD")) != NULL) {
		        }
				/* Call this when not used anymore */
		        lwjson_free(&lwjson);
			}
		}
	}
	else {
	}
}


/**
  * @brief  mqtt client thread
  * @param arg: pointer on argument(not used here)
  * @retval None
  */
static void mqtt_client_thread(void *arg)
{
	/* Connect to server */
	mqtt_client = mqtt_client_new();
	if(mqtt_client != NULL) {
		example_connect(mqtt_client, "Sinteck");
	}

	while(1) {
		// while connected, publish every second
		if(HAL_GetTick() - timer_mqtt > 1000) {
			timer_mqtt = HAL_GetTick();
			if(mqtt_client_is_connected(mqtt_client)) {
				if(mqtt_is_prime == 0) {
					mqtt_is_prime = 1;
					json_config( buff );
					example_publish_config(mqtt_client, NULL);
				}
				else if(mqtt_is_prime == 1) {
					mqtt_is_prime = 2;
					json_preset( buff );
					example_publish_preset(mqtt_client, NULL);
				}
				else {
					mqtt_is_prime++;
					if(mqtt_is_prime >= 60) mqtt_is_prime = 0;
					json_central( buff );
					example_publish(mqtt_client, NULL);
				}
			}
			else {
				// Connect to server
				example_connect(mqtt_client, "Sinteck");
			}
		}
		osDelay(1000);
	}
}

/* Public functions ---------------------------------------------------------*/

/**
  * @brief  Initialize the MQTT client (start its thread)
  * @param  none
  * @retval None
  */
void mqtt_client_init()
{
  sys_thread_new("MQTT", mqtt_client_thread, NULL, 3072, osPriorityLow);
}
