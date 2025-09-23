/*
 * WebSocket_Interface.c
 *
 *  Created on: 7 de abr de 2021
 *      Author: Rinaldo Dos Santos
 *      Sinteck Next
 */

#include "main.h"
#include "stdbool.h"
#include "../Sinteck/tcp/WebSocket_Interface.h"

bool WebSocketClient_getMessage(char* message)
{
	// 1. read type and fin
//	unsigned int msgtype = message[0];
//	if (!client->connected()) {
//		logI("Step 1");
//		return false;
//	}

	// 2. read length and check if masked
	int length = message[1];
	bool hasMask = false;
	if (length & WS_MASK) {
		hasMask = true;
		length = length & ~WS_MASK;
	}

	if (length == WS_SIZE16) {
		length = message[2] << 8;
		length |= message[3];
	}

	// 3. read mask
	if (hasMask) {
		uint8_t mask[4];
		mask[0] = message[4];
		mask[1] = message[5];
		mask[2] = message[6];
		mask[3] = message[7];

		// 4. read message (masked)
		message = "";
		for (int i = 0; i < length; ++i) {
			message += (char) (message[8+i] ^ mask[i % 4]);
		}
	} else {
		// 4. read message (unmasked)
		message = "";
		for (int i = 0; i < length; ++i) {
			message += (char) message[8+i];
		}
	}

    return true;
}
