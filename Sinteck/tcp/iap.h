/*
 * iap.h
 *
 *  Created on: Feb 24, 2020
 *      Author: Rinaldo Dos Santos
 *      Sinteck Next
 */

#ifndef TCP_IAP_H_
#define TCP_IAP_H_

#define USER_FLASH_FIRST_PAGE_ADDRESS 0x08020000 /* Only as example see comment */
#define USER_FLASH_LAST_PAGE_ADDRESS  0x081E0000
#define USER_FLASH_END_ADDRESS        0x081FFFFF

/* UserID and Password definition *********************************************/
#define USERID       "user"
#define PASSWORD     "stm32"
#define LOGIN_SIZE   (15+ sizeof(USERID) + sizeof(PASSWORD))

#endif /* TCP_IAP_H_ */
