/*
 * mqtt_paho.c
 *
 *  Created on: 15 de mar de 2021
 *      Author: Rinaldo Dos Santos
 *      Sinteck Next
 */
#include "main.h"
#include "cmsis_os.h"
#include "queue.h"
#include "stdio.h"
#include "string.h"
#include "lwip.h"
#include "lwip/api.h"
#include "lwip/sys.h"
#include "lwip/inet.h"
#include "lwip/dns.h"
#include "../Sinteck/tcp/mqtt_paho.h"
#include "../Sinteck/tcp/json_util.h"
#include "../cJSON/lwjson.h"
#include "../Sinteck/src/audio.h"
#include "../Sinteck/src/pwm.h"
#include "../Sinteck/src/defines.h"
//
extern uint32_t timer_reflesh;
extern struct netif gnetif;
extern ip4_addr_t ipaddr;
extern ip4_addr_t netmask;
extern ip4_addr_t gw;
extern ip4_addr_t dnsaddr;
extern uint8_t flag_telemetry;
// Profile
extern char station_profile[];
extern char station_city[];
extern char station_state[];
extern char station_country[];
extern char station_temp[];

extern osMessageQueueId_t QueueMqttHandle;
extern uint8_t uuid_model[];
extern uint8_t str_uuid[];
extern char buff[];
extern uint8_t Telemetry_State;
static uint8_t mqtt_is_prime1;
static char token_mqtt[10] = { 0 };
static char topic[200] = { 0 };
static char cmd_mqtt[50] = { 0 };

uint32_t timer_mqtt_pub = 0;
uint8_t alarm_mqtt_rx[32] = { 0 };
Timer timer_mqtt;

/* LwJSON instance and tokens */
static lwjson_token_t tokens[128];
static lwjson_t lwjson;
static lwjsonr_t mqtt_res;
static const lwjson_token_t* t;

MQTTPacket_connectData data_mqtt_paho = MQTTPacket_connectData_initializer;

static void MqttClientSubTask(void *arg);
static void MqttClientPubTask(void *arg);
void MqttMessageArrived(MessageData* msg);

MQTTClient mqttClient;	//mqtt client
Network net; 			//mqtt network

uint8_t sndBuffer[MQTT_BUFSIZE]; //mqtt send buffer
uint8_t rcvBuffer[MQTT_BUFSIZE]; //mqtt receive buffer
uint8_t msgBuffer[MQTT_BUFSIZE]; //mqtt message buffer

extern Cfg_var cfg;

//mqtt subscribe task
static void MqttClientSubTask(void *arg)
{
	//int rc = MQTT_SUCCESS;
	TimerInit(&timer_mqtt);
	mqttClient.isconnected = 0;
	Telemetry_State = 0;

	while(1)
	{
		if(!mqttClient.isconnected) {
			MQTTCloseSession(&mqttClient);
			net_disconnect(&net);
			MQTTDisconnect(&mqttClient);
			MqttConnectBroker();
			osDelay(1000);
		}
		else {
			MQTTYield(&mqttClient, 1000); 		//handle timer
			osDelay(100);
		}
	}
}

//mqtt publish task
static void MqttClientPubTask(void *arg)
{
	int rc = MQTT_SUCCESS;
	char topico[50] = { 0 };
	MQTTMessage send_mqtt;

	while(1)
	{
		if(mqttClient.isconnected)
		{
			if(HAL_GetTick() - timer_mqtt_pub > 1000) {
				timer_mqtt_pub =  HAL_GetTick();
				if(mqtt_is_prime1 == 0) {
					memset(topico, 0, 50);
					sprintf(topico, "RXNSTL/config/%c%c%c%c%c%c%c%c%c", cfg.Token[0], cfg.Token[1], cfg.Token[2], cfg.Token[3],
				  			  	  	  	  	  	  	  	  	        '-',
																	cfg.Token[4], cfg.Token[5], cfg.Token[6], cfg.Token[7]);
					json_config( buff );
					send_mqtt.payload = (void*)buff;
					send_mqtt.payloadlen = strlen(buff);
					send_mqtt.qos = QOS2;
					send_mqtt.retained = 1;
					send_mqtt.dup = 0;

					rc = MQTTPublish(&mqttClient, topico, &send_mqtt);
					if( rc != MQTT_SUCCESS) {
						MQTTCloseSession(&mqttClient);
				        net_disconnect(&net);
					}
					mqtt_is_prime1++;
				}
				else if(mqtt_is_prime1 == 1) {
					mqtt_is_prime1++;
				}
				else {
					mqtt_is_prime1++;
					if(mqtt_is_prime1 >= 60) mqtt_is_prime1 = 0;
					json_central( buff );
					memset(topico, 0, 50);
					sprintf(topico, "RXNSTL/reading/%c%c%c%c%c%c%c%c%c", cfg.Token[0], cfg.Token[1], cfg.Token[2], cfg.Token[3],
						  	  	  	  	  	  	  	  	  	         '-',
																	 cfg.Token[4], cfg.Token[5], cfg.Token[6], cfg.Token[7]);

					send_mqtt.payload = (void*)buff;
					send_mqtt.payloadlen = strlen(buff);
					send_mqtt.qos = QOS2;
					send_mqtt.retained = 1;
					send_mqtt.dup = 0;
					rc = MQTTPublish(&mqttClient, topico, &send_mqtt);
					if( rc != MQTT_SUCCESS) {
				        MQTTCloseSession(&mqttClient);
				        net_disconnect(&net);
					}
					// Se Tiver Alarme Envia
					if (xQueueReceive(QueueMqttHandle, &alarm_mqtt_rx, 0) == pdPASS) {
						memset(topico, 0, 50);
						sprintf(topico, "RXNSTL/alarm/%c%c%c%c%c%c%c%c%c", cfg.Token[0], cfg.Token[1], cfg.Token[2], cfg.Token[3],
																	   '-',
																	   cfg.Token[4], cfg.Token[5], cfg.Token[6], cfg.Token[7]);
						json_alarm(buff, alarm_mqtt_rx);
						send_mqtt.payload = (void*)buff;
						send_mqtt.payloadlen = strlen(buff);
						send_mqtt.qos = QOS2;
						send_mqtt.retained = 1;
						send_mqtt.dup = 0;
						rc = MQTTPublish(&mqttClient, topico, &send_mqtt);
						if( rc != MQTT_SUCCESS) {
					        MQTTCloseSession(&mqttClient);
					        net_disconnect(&net);
						}
					}
				}
			}
		}
		else {
			MQTTCloseSession(&mqttClient);
			net_disconnect(&net);
			MQTTDisconnect(&mqttClient);
			MqttConnectBroker();
		}
		osDelay(500);
	}
}

int MqttConnectBroker(void)
{
	int ret;
	char rasc[50] = { 0 };

	// Token
	if(cfg.SerialNumber[0] == 0xFF || cfg.SerialNumber[1] == 0xFF || cfg.SerialNumber[2] == 0xFF || cfg.SerialNumber[3] == 0xFF ||
	   cfg.SerialNumber[4] == 0xFF || cfg.SerialNumber[5] == 0xFF || cfg.SerialNumber[6] == 0xFF || cfg.SerialNumber[7] == 0xFF ) {

		cfg.Token[0] = '0';
		cfg.Token[1] = '0';
		cfg.Token[2] = '0';
		cfg.Token[3] = '0';
		cfg.Token[4] = '0';
		cfg.Token[5] = '0';
		cfg.Token[6] = '0';
		cfg.Token[7] = '0';
		cfg.Token[8] = 0;
		cfg.Token[12] = 1;
	}
	else {
		cfg.Token[0] = cfg.SerialNumber[0];
		cfg.Token[1] = cfg.SerialNumber[1];
		cfg.Token[2] = cfg.SerialNumber[2];
		cfg.Token[3] = cfg.SerialNumber[3];
		cfg.Token[4] = cfg.SerialNumber[4];
		cfg.Token[5] = cfg.SerialNumber[5];
		cfg.Token[6] = cfg.SerialNumber[6];
		cfg.Token[7] = cfg.SerialNumber[7];
		cfg.Token[8] = 0;
		cfg.Token[12] = 1;
	}

	if(cfg.Token[0] == 0 || cfg.Token[1] == 0 || cfg.Token[2] == 0 || cfg.Token[3] == 0) {
		return -1;
	}

	if(cfg.Token[0] == ' ' || cfg.Token[1] == ' ' || cfg.Token[2] == ' ' || cfg.Token[3] == ' ') {
		return -1;
	}

	NewNetwork(&net);
	ret = ConnectNetwork(&net, BROKER_IP, MQTT_PORT);
	if(ret != MQTT_SUCCESS)
	{
		return -1;
	}

	MQTTClientInit(&mqttClient, &net, 1000, sndBuffer, sizeof(sndBuffer), rcvBuffer, sizeof(rcvBuffer));

	MQTTPacket_connectData data = MQTTPacket_connectData_initializer;

	//
	uint32_t idPart1 = HAL_GetUIDw0();
	uint32_t idPart2 = HAL_GetUIDw1();
	uint32_t idPart3 = HAL_GetUIDw2();
	uint32_t idPart4 = HAL_GetDEVID();
	sprintf(rasc, "%08lX-%02X%02X-%02X%02X-%02X%02X-%02X%02X%08lX", idPart1,
	                                  (uint8_t)((idPart4 & 0xFF000000) >> 24),
				                      (uint8_t)((idPart4 & 0x00FF0000) >> 16),
				                      (uint8_t)((idPart4 & 0x0000FF00) >> 8),
				                      (uint8_t)((idPart4 & 0x000000FF)),
				                      (uint8_t)((idPart2 & 0xFF000000) >> 24),
				                      (uint8_t)((idPart2 & 0x00FF0000) >> 16),
				                      (uint8_t)((idPart2 & 0x0000FF00) >> 8),
				                      (uint8_t)((idPart2 & 0x000000FF)),
				                      idPart3 );
	//

	data.willFlag = 0;
	data.MQTTVersion = 4;
	data.clientID.cstring = rasc;
	data.username.cstring = "XTTransmitter";
	data.password.cstring = "Bolivianos578@";
	data.keepAliveInterval = 60;
	data.cleansession = 1;

	ret = MQTTConnect(&mqttClient, &data);
	if(ret != MQTT_SUCCESS)
	{
		Telemetry_State = 0;
		return ret;
	}
	else {
		Telemetry_State = 1;
		return MQTT_SUCCESS;
	}
}

void MqttMessageArrived(MessageData* msg)
{
	uint32_t x;

	// Topico
	memset(topic, 0, 200);
	memset(cmd_mqtt, 0, 50);
	memcpy(topic, msg->topicName->lenstring.data, strlen(msg->topicName->lenstring.data));
	// Mensagem
	MQTTMessage* message = msg->message;
	memset(msgBuffer, 0, sizeof(msgBuffer));
	memcpy(msgBuffer, message->payload, message->payloadlen);

	if (strncmp((char const *)&topic,"cmd/", 4) == 0) {
		token_mqtt[0] = topic[4];
		token_mqtt[1] = topic[5];
		token_mqtt[2] = topic[6];
		token_mqtt[3] = topic[7];
		token_mqtt[4] = topic[8];
		token_mqtt[5] = topic[9];
		token_mqtt[6] = topic[10];
		token_mqtt[7] = topic[11];
		token_mqtt[8] = topic[12];
		token_mqtt[9] = '\0';
		//
		size_t len_cmd = (strlen(topic) - 14 - (int)message->payloadlen);
		for(x = 0; x < len_cmd; x++) {
			cmd_mqtt[x] = topic[14+x];
		}
	    cmd_mqtt[x] = '\0';
	}

	Process_MQTT_InTopic(token_mqtt, cmd_mqtt, msgBuffer, (int)message->payloadlen);

	// Free Buffer TopicName
	memset(msg->topicName->lenstring.data, 0, strlen(msg->topicName->lenstring.data));
}

void Process_MQTT_InTopic(char *token, char *cmd, uint8_t * msg, int size)
{
	uint8_t v1, v2, v3, v4, v5, v6, v7, v8, v9;

	lwjson_init(&lwjson, tokens, LWJSON_ARRAYSIZE(tokens));
	mqtt_res = lwjson_parse(&lwjson, (const char *)msg);

	if (strncmp(cmd, "Audio", 5) == 0) {
		v1 = v2 = v3 = v4 = v5 = v6 = v7 = v8 = v9 = 0;

		if (mqtt_res == lwjsonOK) {
			/* Find custom key in JSON */
			if ((t = lwjson_find(&lwjson, "Stereo")) != NULL) {
				if( t->type == LWJSON_TYPE_TRUE ) {
					v1 = 1;
				}
				else {
					v1 = 0;
				}
				if ((t = lwjson_find(&lwjson, "Source")) != NULL) {
					v2 = lwjson_get_val_int(t);
					if ((t = lwjson_find(&lwjson, "Emphase")) != NULL) {
						if( t->type == LWJSON_TYPE_TRUE ) {
							v3 = 1;
						}
						else {
							v3 = 0;
						}
						if ((t = lwjson_find(&lwjson, "Processor")) != NULL) {
							if( t->type == LWJSON_TYPE_TRUE ) {
								v4 = 1;
							}
							else {
								v4 = 0;
							}
							if ((t = lwjson_find(&lwjson, "Clipper")) != NULL) {
								if( t->type == LWJSON_TYPE_TRUE ) {
									v5 = 1;
								}
								else {
									v5 = 0;
								}
								if ((t = lwjson_find(&lwjson, "VolMpx")) != NULL) {
									v6 = lwjson_get_val_int(t);
									if ((t = lwjson_find(&lwjson, "VolSca")) != NULL) {
										v7 = lwjson_get_val_int(t);
							        	// Salva na EEPROM
							        	flag_telemetry = 2;
									}
								}
							}
						}
					}
				}
			}
	        /* Call this when not used anymore */
	        lwjson_free(&lwjson);
		}
	}
	else if ( (strncmp(cmd, "PresetAtivo", 11) == 0) && (strlen(cmd) == 11) ) {
		if (mqtt_res == lwjsonOK) {
			/* Find custom key in JSON */
			if ((t = lwjson_find(&lwjson, "PresetAtivo")) != NULL) {
			}
	        /* Call this when not used anymore */
	        lwjson_free(&lwjson);
		}
	}
	else if (strncmp(cmd, "rds", 3) == 0) {
		if (mqtt_res == lwjsonOK) {
			/* Find custom key in JSON */
			if ((t = lwjson_find(&lwjson, "Enable")) != NULL) {
				if( t->type == LWJSON_TYPE_TRUE ) {
					v1 = 1;
				}
				else {
					v1 = 0;
				}
				if ((t = lwjson_find(&lwjson, "Date-Time")) != NULL) {
					if( t->type == LWJSON_TYPE_TRUE ) {
						v2 = 1;
					}
					else {
						v2 = 0;
					}
					if ((t = lwjson_find(&lwjson, "Type")) != NULL) {
						v3 = lwjson_get_val_int(t);
						if ((t = lwjson_find(&lwjson, "PS")) != NULL) {
							char strps[10] = {0};
							size_t len_str = lwjson_get_val_string_length(t);
							memset(strps, 0, 10);
							memcpy(strps, lwjson_get_val_string(t, &len_str), len_str);
							if ((t = lwjson_find(&lwjson, "PI")) != NULL) {
								char strpi[5] = {0};
								size_t len_str = lwjson_get_val_string_length(t);
								memset(strpi, 0, 5);
								memcpy(strpi, lwjson_get_val_string(t, &len_str), len_str);
								if ((t = lwjson_find(&lwjson, "PTY")) != NULL) {
									v4 = lwjson_get_val_int(t);
									if ((t = lwjson_find(&lwjson, "PTYN")) != NULL) {
										char strptyn[10] = {0};
										size_t len_str = lwjson_get_val_string_length(t);
										memset(strptyn, 0, 10);
										memcpy(strptyn, lwjson_get_val_string(t, &len_str), len_str);
										if ((t = lwjson_find(&lwjson, "MS")) != NULL) {
											v5 = lwjson_get_val_int(t);
											if ((t = lwjson_find(&lwjson, "AF1")) != NULL) {
												v6 = lwjson_get_val_int(t);
												if ((t = lwjson_find(&lwjson, "AF2")) != NULL) {
													v7 = lwjson_get_val_int(t);
													if ((t = lwjson_find(&lwjson, "AF3")) != NULL) {
														v8 = lwjson_get_val_int(t);
														if ((t = lwjson_find(&lwjson, "AF4")) != NULL) {
															v9 = lwjson_get_val_int(t);
															if ((t = lwjson_find(&lwjson, "AF5")) != NULL) {
																if ((t = lwjson_find(&lwjson, "RadioText")) != NULL) {
																	char strrt[65] = {0};
																	size_t len_str = lwjson_get_val_string_length(t);
																	memset(strrt, 0, 65);
																	memcpy(strrt, lwjson_get_val_string(t, &len_str), len_str);
																	if ((t = lwjson_find(&lwjson, "DinamicText")) != NULL) {
																		char strdn[65] = {0};
																		size_t len_str = lwjson_get_val_string_length(t);
																		memset(strdn, 0, 65);
																		memcpy(strdn, lwjson_get_val_string(t, &len_str), len_str);
																		if ((t = lwjson_find(&lwjson, "Remote")) != NULL) {
																			if ((t = lwjson_find(&lwjson, "Port")) != NULL) {
																				//
																			}
																		}
																	}
																}
															}
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
	        /* Call this when not used anymore */
	        lwjson_free(&lwjson);
		}
	}
	else if (strncmp(cmd, "rfstate", 7) == 0) {
		if (mqtt_res == lwjsonOK) {
			/* Find custom key in JSON */
			if ((t = lwjson_find(&lwjson, "state")) != NULL) {
				v1 = lwjson_get_val_int(t);
				if(v1 == 0) {
					// Power OFF
					timer_reflesh = 0;
				}
				if(v1 == 1) {
	            	// Power ON
	            	flag_telemetry = 1;
				}
				if(v1 == 2) {
					// Safe-Mode
		 			timer_reflesh = 0;
				}
			}
	        /* Call this when not used anymore */
	        lwjson_free(&lwjson);
		}
	}
	else if (strncmp(cmd,"Frequency", 9) == 0) {
		if (mqtt_res == lwjsonOK) {
			/* Find custom key in JSON */
			if ((t = lwjson_find(&lwjson, "Frequency")) != NULL) {
			}
	        /* Call this when not used anymore */
	        lwjson_free(&lwjson);
		}
	}
	else if (strncmp(cmd,"PowerLock", 9) == 0) {
		if (mqtt_res == lwjsonOK) {
			/* Find custom key in JSON */
			if ((t = lwjson_find(&lwjson, "PowerLock")) != NULL) {
			}
	        /* Call this when not used anymore */
	        lwjson_free(&lwjson);
		}
	}
	else if (strncmp(cmd,"SendAlert", 9) == 0) {
		if (mqtt_res == lwjsonOK) {
			/* Find custom key in JSON */
			if ((t = lwjson_find(&lwjson, "Email1")) != NULL) {
			}
	        /* Call this when not used anymore */
	        lwjson_free(&lwjson);
		}
	}
	else if (strncmp(cmd,"License", 7) == 0) {
		if (mqtt_res == lwjsonOK) {
			/* Find custom key in JSON */
	        /* Call this when not used anymore */
	        lwjson_free(&lwjson);
		}
	}
	else if (strncmp(cmd,"Password", 8) == 0) {
		if (mqtt_res == lwjsonOK) {
			/* Find custom key in JSON */
			if ((t = lwjson_find(&lwjson, "Admin")) != NULL) {
			}
			flag_telemetry = 9;
	        /* Call this when not used anymore */
	        lwjson_free(&lwjson);
		}
	}
	else if (strncmp(cmd,"AdvSettings", 11) == 0) {
		if (mqtt_res == lwjsonOK) {
			/* Find custom key in JSON */
		}
	    /* Call this when not used anymore */
	    lwjson_free(&lwjson);
	}
	else if (strncmp(cmd,"Network", 7) == 0) {
		if (mqtt_res == lwjsonOK) {
			/* Find custom key in JSON */
			if ((t = lwjson_find(&lwjson, "IP")) != NULL) {
				size_t len_str = lwjson_get_val_string_length(t);
				uint8_t ip_m[16] = {0};
				memcpy(ip_m, lwjson_get_val_string(t, &len_str), len_str);
				if ((t = lwjson_find(&lwjson, "MASK")) != NULL) {
					size_t len_str = lwjson_get_val_string_length(t);
					uint8_t m_m[16] = {0};
					memcpy(m_m, lwjson_get_val_string(t, &len_str), len_str);
					if ((t = lwjson_find(&lwjson, "GW")) != NULL) {
						size_t len_str = lwjson_get_val_string_length(t);
						uint8_t gw_m[16] = {0};
						memcpy(gw_m, lwjson_get_val_string(t, &len_str), len_str);
						if ((t = lwjson_find(&lwjson, "DNS")) != NULL) {
							size_t len_str = lwjson_get_val_string_length(t);
							uint8_t d_m[16] = {0};
							memcpy(d_m, lwjson_get_val_string(t, &len_str), len_str);
							if ((t = lwjson_find(&lwjson, "Port")) != NULL) {
								int mqtt_pport = lwjson_get_val_int(t);
								//
								u32_t mqtt_ip = inet_addr((const char*)ip_m);
								u32_t mqtt_mask = inet_addr((const char*)m_m);
								u32_t mqtt_gw = inet_addr((const char*)gw_m);
								u32_t mqtt_dns = inet_addr((const char*)d_m);
								//
								cfg.IP_ADDR[3] = (mqtt_ip>>24) & 0xFF;
								cfg.IP_ADDR[2] = (mqtt_ip>>16) & 0xFF;
								cfg.IP_ADDR[1] = (mqtt_ip>>8) & 0xFF;
								cfg.IP_ADDR[0] = mqtt_ip & 0xFF;
								//
								cfg.MASK_ADDR[3] = (mqtt_mask>>24) & 0xFF;
								cfg.MASK_ADDR[2] = (mqtt_mask>>16) & 0xFF;
								cfg.MASK_ADDR[1] = (mqtt_mask>>8) & 0xFF;
								cfg.MASK_ADDR[0] = mqtt_mask & 0xFF;
								//
								cfg.GW_ADDR[3] = (mqtt_gw>>24) & 0xFF;
								cfg.GW_ADDR[2] = (mqtt_gw>>16) & 0xFF;
								cfg.GW_ADDR[1] = (mqtt_gw>>8) & 0xFF;
								cfg.GW_ADDR[0] = mqtt_gw & 0xFF;
								//
								cfg.DNS_ADDR[3] = (mqtt_dns>>24) & 0xFF;
								cfg.DNS_ADDR[2] = (mqtt_dns>>16) & 0xFF;
								cfg.DNS_ADDR[1] = (mqtt_dns>>8) & 0xFF;
								cfg.DNS_ADDR[0] = mqtt_dns & 0xFF;
								//
								// Atualiza IPS
								cfg.PortWEB = mqtt_pport;
								dns_setserver(0, (const ip_addr_t *)&mqtt_dns);
								netif_set_addr(&gnetif, (const ip4_addr_t *)&mqtt_ip, (const ip4_addr_t *)&mqtt_mask, (const ip4_addr_t *)&mqtt_gw);
								flag_telemetry = 12;
							}
						}
					}
				}
			}
	        /* Call this when not used anymore */
	        lwjson_free(&lwjson);
		}
	}
	else if (strncmp(cmd,"LocalTimes", 10) == 0) {
		if (mqtt_res == lwjsonOK) {
			/* Find custom key in JSON */
			if ((t = lwjson_find(&lwjson, "time")) != NULL) {
				size_t len_str = lwjson_get_val_string_length(t);
				uint8_t mqtt_epoch[20] = {0};
				memset(mqtt_epoch, 0, 20);
				memcpy(mqtt_epoch, lwjson_get_val_string(t, &len_str), len_str);
				if ((t = lwjson_find(&lwjson, "NTP")) != NULL) {
					if( t->type == LWJSON_TYPE_TRUE ) {
						v1 = 1;
					}
					else {
						v1 = 0;
					}
					if ((t = lwjson_find(&lwjson, "Fuso")) != NULL) {
						if ((t = lwjson_find(&lwjson, "Language")) != NULL) {
							//
							// Acerta Relogio
				        	flag_telemetry = 4;
						}
					}
				}
			}
	        /* Call this when not used anymore */
	        lwjson_free(&lwjson);
		}
	}
	else if (strncmp(cmd,"ConfigHold", 10) == 0) {
		if (mqtt_res == lwjsonOK) {
			/* Find custom key in JSON */
			if ((t = lwjson_find(&lwjson, "10MHZ")) != NULL) {
				if ((t = lwjson_find(&lwjson, "ConfigInHold")) != NULL) {
					if( t->type == LWJSON_TYPE_TRUE ) {
						v1 = 1;
					}
					else {
						v1 = 0;
					}
					if ((t = lwjson_find(&lwjson, "EnableVswrNull")) != NULL) {
						if( t->type == LWJSON_TYPE_TRUE ) {
							v2 = 1;
						}
						else {
							v2 = 0;
						}
						if ((t = lwjson_find(&lwjson, "VSWRNULL")) != NULL) {
							flag_telemetry = 10;
						}
					}
				}
			}
	        /* Call this when not used anymore */
	        lwjson_free(&lwjson);
		}
	}
	else if (strncmp(cmd,"Service", 7) == 0) {
		if (mqtt_res == lwjsonOK) {
			/* Find custom key in JSON */
			if ((t = lwjson_find(&lwjson, "RFInput")) != NULL) {
				if ((t = lwjson_find(&lwjson, "EltekOut")) != NULL) {
					if ((t = lwjson_find(&lwjson, "ServiceMode")) != NULL) {
						if( t->type == LWJSON_TYPE_TRUE ) {
							v1 = 1;
						}
						else {
							v1 = 0;
						}
						if ((t = lwjson_find(&lwjson, "PWMBias")) != NULL) {
							if ((t = lwjson_find(&lwjson, "PWMDriver")) != NULL) {
								if ((t = lwjson_find(&lwjson, "ResetOffsetBattery")) != NULL) {
									if( t->type == LWJSON_TYPE_TRUE ) {
										v2 = 1;
									}
									else {
										v2 = 0;
									}
									if ((t = lwjson_find(&lwjson, "ResetOffsetIPA")) != NULL) {
										if( t->type == LWJSON_TYPE_TRUE ) {
											v3 = 1;
										}
										else {
											v3 = 0;
										}
										//
										if( v2 == 1 ) {
											flag_telemetry = 15;
										}
										if( v2 == 1 ) {
											flag_telemetry = 16;
										}
									}
								}
							}
						}
					}
				}
			}
	        /* Call this when not used anymore */
	        lwjson_free(&lwjson);
		}
	}
	else if (strncmp(cmd,"Weather", 7) == 0) {
		if (mqtt_res == lwjsonOK) {
			/* Find custom key in JSON */
			if ((t = lwjson_find(&lwjson, "Name")) != NULL) {
	        	size_t len_str = lwjson_get_val_string_length(t);
				memset(station_profile, 0 , 50);
				if(len_str < 25) {
					memcpy(station_profile, lwjson_get_val_string(t, &len_str), len_str);
				}
				if ((t = lwjson_find(&lwjson, "City")) != NULL) {
					size_t len_str = lwjson_get_val_string_length(t);
					memset(station_city, 0 , 50);
					if(len_str < 25) {
						memcpy(station_city, lwjson_get_val_string(t, &len_str), len_str);
					}
					if ((t = lwjson_find(&lwjson, "State")) != NULL) {
						size_t len_str = lwjson_get_val_string_length(t);
						memset(station_state, 0 , 50);
						if(len_str < 25) {
							memcpy(station_state, lwjson_get_val_string(t, &len_str), len_str);
						}
						if ((t = lwjson_find(&lwjson, "Country")) != NULL) {
							size_t len_str = lwjson_get_val_string_length(t);
							memset(station_country, 0 , 50);
							if(len_str < 25) {
								memcpy(station_country, lwjson_get_val_string(t, &len_str), len_str);
							}
							if ((t = lwjson_find(&lwjson, "ExtTemperature")) != NULL) {
								float value = lwjson_get_val_real(t);
								sprintf(station_temp, "%0.1f°C", value);
					        	// Marca para Salvar em EEPROM
					        	flag_telemetry = 27;
							}
						}
					}
				}
			}
		}
        /* Call this when not used anymore */
        lwjson_free(&lwjson);
	}
}

void mqtt_init(void)
{
  sys_thread_new("MQTT_pub", MqttClientPubTask, NULL, 4096, osPriorityLow4);
  sys_thread_new("MQTT_sub", MqttClientSubTask, NULL, 4096, osPriorityLow3);
}
