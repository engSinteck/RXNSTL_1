/*
 * w25q512fv.h
 *
 *  Created on: 26 de fev de 2021
 *      Author: Rinaldo Dos Santos
 *      Sinteck Next
 */

#ifndef SRC_W25Q512FV_H_
#define SRC_W25Q512FV_H_

#ifdef __cplusplus
 extern "C" {
#endif


/**
  * @brief  W25Q512FV Configuration
  */
#define W25Q512FV_FLASH_SIZE                  	0x4000000 /* 512 MBits => 64MBytes */
#define W25Q512FV_SECTOR_SIZE                 	0x10000   /* 1024 sectors of 64KBytes */
#define W25Q512FV_SUBSECTOR_SIZE              	0x1000    /* 16384 subsectors of 4kBytes */
#define W25Q512FV_PAGE_SIZE                   	0x100     /* 262144 pages of 256 bytes */
#define W25Q512FV_SECTOR_COUNT				  	512

#define W25Q512FV_DUMMY_CYCLES_READ           	4
#define W25Q512FV_DUMMY_CYCLES_READ_QUAD      	10
#define W25Q512FV_DUMMY_CYCLES_READ_DTR       	6
#define W25Q512FV_DUMMY_CYCLES_READ_QUAD_DTR  	8

#define W25Q512FV_BULK_ERASE_MAX_TIME         	480000
#define W25Q512FV_SECTOR_ERASE_MAX_TIME       	3000
#define W25Q512FV_SUBSECTOR_ERASE_MAX_TIME    	800

/**
  * @brief  W25Q512FV Commands
  */
/* Reset Operations */
#define RESET_ENABLE_CMD						0x66
#define RESET_MEMORY_CMD                     	0x99

#define ENTER_QPI_MODE_CMD                   	0x38
#define EXIT_QPI_MODE_CMD                    	0xFF

/* Identification Operations */
#define READ_ID_CMD                          	0x90
#define DUAL_READ_ID_CMD                    	0x92
#define QUAD_READ_ID_CMD                     	0x94
#define READ_JEDEC_ID_CMD                    	0x9F

/* Read Operations */
#define READ_CMD                             	0x03
#define FAST_READ_CMD                        	0x0B
#define DUAL_OUT_FAST_READ_CMD               	0x3B
#define DUAL_INOUT_FAST_READ_CMD             	0xBB
#define QUAD_OUT_FAST_READ_CMD               	0x6B
#define QUAD_INOUT_FAST_READ_CMD             	0xEB

/* Write Operations */
#define WRITE_ENABLE_CMD                     	0x06
#define WRITE_DISABLE_CMD                    	0x04

/* Register Operations */
#define READ_STATUS_REG1_CMD                  	0x05
#define READ_STATUS_REG2_CMD                  	0x35
#define READ_STATUS_REG3_CMD                  	0x15

#define WRITE_STATUS_REG1_CMD                 	0x01
#define WRITE_STATUS_REG2_CMD                 	0x31
#define WRITE_STATUS_REG3_CMD                 	0x11

/* Program Operations */
#define PAGE_PROG_CMD                        	0x02
#define QUAD_INPUT_PAGE_PROG_CMD             	0x32

/* Erase Operations */
#define SECTOR_ERASE_CMD                     	0x20
#define CHIP_ERASE_CMD                       	0xC7

#define PROG_ERASE_RESUME_CMD                	0x7A
#define PROG_ERASE_SUSPEND_CMD               	0x75

 /* 4-byte Address Mode Operations */
 #define ENTER_4_BYTE_ADDR_MODE_CMD           	0xB7
 #define EXIT_4_BYTE_ADDR_MODE_CMD            	0xE9

/* Flag Status Register */
#define W25Q512FV_FSR_BUSY                    	((uint8_t)0x01)    /*!< busy */
#define W25Q512FV_FSR_WREN                    	((uint8_t)0x02)    /*!< write enable */
#define W25Q512FV_FSR_QE                      	((uint8_t)0x02)    /*!< quad enable */

#ifdef __cplusplus
}
#endif

#endif /* SRC_W25Q512FV_H_ */
