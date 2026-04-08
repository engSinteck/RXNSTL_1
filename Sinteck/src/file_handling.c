/*
 * file_handling.c
 *
 *  Created on: 8 de abr. de 2026
 *      Author: rinaldo.santos
 */

#ifndef SRC_FILE_HANDLING_C_
#define SRC_FILE_HANDLING_C_

#include "main.h"
#include "stdio.h"
#include "fatfs.h"

// USB HOST
extern uint8_t retUSBH;    /* Return value for USBH */
extern FATFS USBHFatFS;    /* File system object for USBH logical drive */
extern FIL USBHFile;       /* File object for USBH */
extern char USBHPath[4];

FRESULT fr_mp3;     /* FatFs return code */
FATFS *pfs_usb;
FRESULT fresult;
DWORD fre_clust_usb;
uint32_t totalSpace_usb = 0, freeSpace_usb = 0, SpaceUsed_usb = 0;
unsigned int bytesReadMP3;
__attribute__((section(".DTCMRAM")))
__ALIGN_BEGIN static BYTE buffer_mp3[2048] __ALIGN_END;

void Mount_USB(void)
{
	//fresult = f_mount(&USBHFatFS, "", 1);
	fresult = f_mount(&USBHFatFS, USBHPath, 1);
//	if (fresult != FR_OK) printf ("ERROR!!! in mounting USB ...\n\n");
//	else printf("USB mounted successfully...\n");
	HAL_GPIO_WritePin(LED_FAIL_GPIO_Port, LED_FAIL_Pin, LED_ON);
}

void Unmount_USB(void)
{
	fresult = f_mount(NULL, USBHPath, 1);
//	if (fresult == FR_OK) printf ("USB UNMOUNTED successfully...\n\n\n");
//	else printf("ERROR!!! in UNMOUNTING USB \n\n\n");
	HAL_GPIO_WritePin(LED_FAIL_GPIO_Port, LED_FAIL_Pin, LED_OFF);
}

void Check_USB_Details(void)
{
    /* Check free space */
    f_getfree("1:", &fre_clust_usb, &pfs_usb);

    totalSpace_usb = (uint32_t)((pfs_usb->n_fatent - 2) * pfs_usb->csize * 0.5);
//    printf ("USB  Total Size: \t%lu\n",totalSpace_usb);
    freeSpace_usb = (uint32_t)(fre_clust_usb * pfs_usb->csize * 0.5);
//    printf ("USB Free Space: \t%lu\n", freeSpace_usb);
}

void Read_MP3_File(void)
{
	fr_mp3 = f_open(&USBHFile, "1:test.mp3", FA_READ);

    if( fr_mp3 == FR_OK ) {
        do {
            f_read(&USBHFile, buffer_mp3, sizeof(buffer_mp3), &bytesReadMP3);
            if (bytesReadMP3 > 0) {
                // Aqui você envia os dados para a task de decodificação Helix
                //Process_MP3_Data(buffer, bytesRead);
            }
        } while (bytesReadMP3 > 0);
        f_close(&USBHFile);
    } else {
        printf("Erro ao abrir arquivo MP3.\n");
    }
}

#endif /* SRC_FILE_HANDLING_C_ */
