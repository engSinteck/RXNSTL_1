/*
 * uuid.h
 *
 *  Created on: 25 de mar de 2020
 *      Author: rinal
 */

#ifndef SRC_UUID_H_
#define SRC_UUID_H_

/**
* A simple header for reading the STM32 device UUID
* Tested with STM32F4 and STM32F0 families
*
* Version 1.0
* Written by Uli Koehler
* Published on http://techoverflow.net
* Licensed under CC0 (public domain):
* https://creativecommons.org/publicdomain/zero/1.0/
*
* Posted by "Electrical engineering and programming notepad"
* URL: https://ee-programming-notepad.blogspot.com/2017/06/reading-stm32f4-unique-device-id-from.html
*/
#include <stdint.h>
/**
* The STM32 factory-programmed UUID memory.
* Three values of 32 bits each starting at this address
* Use like this: STM32_UUID[0], STM32_UUID[1], STM32_UUID[2]
*/
#define STM32_UUID ((uint32_t *)0x1FFF7A10)

void uuid_generator(void);
void Get_UUID(void);

#endif /* SRC_UUID_H_ */
