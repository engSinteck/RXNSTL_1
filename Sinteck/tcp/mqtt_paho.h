/*
 * mqtt_paho.h
 *
 *  Created on: 15 de mar de 2021
 *      Author: Rinaldo Dos Santos
 *      Sinteck Next
 */

#ifndef TCP_MQTT_PAHO_H_
#define TCP_MQTT_PAHO_H_

#include "../MQTT/MQTTClient.h"
#include "../MQTT/MQTTInterface.h"
#include "cmsis_os.h"

#include <string.h>

#define BROKER_IP		"3.23.178.219"
//#define BROKER_IP		"ec2-3-23-178-219.us-east-2.compute.amazonaws.com"
//#define BROKER_IP		"xt.sinteck.com.br"

#define MQTT_PORT		1883
#define MQTT_BUFSIZE	4096

int  MqttConnectBroker(void);
void MqttMessageArrived(MessageData* msg);
void mqtt_init(void);
void Process_MQTT_InTopic(char *token, char *cmd, uint8_t * msg, int size);

#endif /* TCP_MQTT_PAHO_H_ */
