/* USER CODE BEGIN Header */
/**
 ******************************************************************************
  * @file    user_diskio.c
  * @brief   This file includes a diskio driver skeleton to be completed by the user.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
 /* USER CODE END Header */

#ifdef USE_OBSOLETE_USER_CODE_SECTION_0
/*
 * Warning: the user section 0 is no more in use (starting from CubeMx version 4.16.0)
 * To be suppressed in the future.
 * Kept to ensure backward compatibility with previous CubeMx versions when
 * migrating projects.
 * User code previously added there should be copied in the new user sections before
 * the section contents can be deleted.
 */
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */
#endif

/* USER CODE BEGIN DECL */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "ff_gen_drv.h"
//#include "W25QXX.h"
#include <stdio.h>
#include "../Sinteck/src/stm32_qspi_512.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
/* Disk status */
static volatile DSTATUS Stat = STA_NOINIT;

/* USER CODE END DECL */

/* Private function prototypes -----------------------------------------------*/
DSTATUS USER_initialize (BYTE pdrv);
DSTATUS USER_status (BYTE pdrv);
DRESULT USER_read (BYTE pdrv, BYTE *buff, DWORD sector, UINT count);
#if FF_FS_READONLY == 0
  DRESULT USER_write (BYTE pdrv, const BYTE *buff, DWORD sector, UINT count);  
#endif /* FF_FS_READONLY == 0 */
#if FF_MAX_SS != FF_MIN_SS
  DRESULT USER_ioctl (BYTE pdrv, BYTE cmd, void *buff);
#endif /* FF_MAX_SS != FF_MIN_SS */

Diskio_drvTypeDef  USER_Driver =
{
  USER_initialize,
  USER_status,
  USER_read, 
#if  FF_USE_WRITE
  USER_write,
#endif  /* _USE_WRITE == 1 */  
#if  FF_USE_IOCTL == 1
  USER_ioctl,
#endif /* _USE_IOCTL == 1 */
};

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Initializes a Drive
  * @param  pdrv: Physical drive number (0..)
  * @retval DSTATUS: Operation status
  */
DSTATUS USER_initialize (
	BYTE pdrv           /* Physical drive nmuber to identify the drive */
)
{
  /* USER CODE BEGIN INIT */
    Stat = STA_NOINIT;
    Stat = ~STA_NOINIT;
    return Stat;
  /* USER CODE END INIT */
}
 
/**
  * @brief  Gets Disk Status 
  * @param  pdrv: Physical drive number (0..)
  * @retval DSTATUS: Operation status
  */
DSTATUS USER_status (
	BYTE pdrv       /* Physical drive number to identify the drive */
)
{
  /* USER CODE BEGIN STATUS */
    //Stat = STA_NOINIT;
    return FR_OK;
  /* USER CODE END STATUS */
}

/**
  * @brief  Reads Sector(s) 
  * @param  pdrv: Physical drive number (0..)
  * @param  *buff: Data buffer to store read data
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to read (1..128)
  * @retval DRESULT: Operation result
  */
DRESULT USER_read (
	BYTE pdrv,      /* Physical drive nmuber to identify the drive */
	BYTE *buff,     /* Data buffer to store read data */
	DWORD sector,   /* Sector address in LBA */
	UINT count      /* Number of sectors to read */
)
{
  /* USER CODE BEGIN READ */
		//logI("User_Read: pdrv: %d  Sector: %ld  Count: %d\n\r", pdrv, sector, count);
		for(;count>0;count--) {
			if(BSP_QSPI_Read(buff, sector*FF_MIN_SS, FF_MIN_SS) != QSPI_OK) {
				return RES_ERROR;
			}
			sector++;
			buff += FF_MIN_SS;			// 512;
		}
		/* wait until the read operation is finished */

	    return RES_OK;
  /* USER CODE END READ */
}

/**
  * @brief  Writes Sector(s)
  * @param  pdrv: Physical drive number (0..)
  * @param  *buff: Data to be written
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to write (1..128)
  * @retval DRESULT: Operation result
  */
#if FF_FS_READONLY == 0
DRESULT USER_write (
	BYTE pdrv,          /* Physical drive nmuber to identify the drive */
	const BYTE *buff,   /* Data to be written */
	DWORD sector,       /* Sector address in LBA */
	UINT count          /* Number of sectors to write */
)
{ 
  /* USER CODE BEGIN WRITE */
	  /* USER CODE HERE */
		for(;count>0;count--) {
//			if(BSP_QSPI_Write(buff, sector*FF_MIN_SS, FF_MIN_SS) != QSPI_OK) {
//				return RES_ERROR;
//			}
//			sector++;
//			buff += FF_MIN_SS;
		}

	    return RES_OK;
  /* USER CODE END WRITE */
}
#endif /* FF_FS_READONLY == 0 */

/**
  * @brief  I/O control operation
  * @param  pdrv: Physical drive number (0..)
  * @param  cmd: Control code
  * @param  *buff: Buffer to send/receive control data
  * @retval DRESULT: Operation result
  */
#if FF_MAX_SS != FF_MIN_SS
DRESULT USER_ioctl (
	BYTE pdrv,      /* Physical drive nmuber (0..) */
	BYTE cmd,       /* Control code */
	void *buff      /* Buffer to send/receive control data */
)
{
  /* USER CODE BEGIN IOCTL */
	DRESULT res = RES_ERROR;

	if (Stat & STA_NOINIT) return RES_NOTRDY;

	switch(cmd) {
		case CTRL_SYNC:
			res = RES_OK;
			break;
		case GET_SECTOR_SIZE:
			*(DWORD*)buff = W25Q128FV_SECTOR_COUNT;			// 512
			res = RES_OK;
			break;
		case GET_BLOCK_SIZE:
			*(DWORD*)buff = 1024*1024*64;
			//*(DWORD*)buff = W25Q128FV_SUBSECTOR_SIZE;		// 65536
			res = RES_OK;
			break;
		case GET_SECTOR_COUNT:
			*(DWORD*)buff = W25Q128FV_SUBSECTOR_SIZE;
			//*(DWORD*)buff = 1024*1024*64;					// 4096
			res = RES_OK;
			break;
	}
    return res;
  /* USER CODE END IOCTL */
}
#endif /* FF_MAX_SS != FF_MIN_SS */

