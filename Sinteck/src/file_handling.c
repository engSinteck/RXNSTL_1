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
#include "i2s.h"
#include "mp3dec.h"

#include "file_handling.h"
#include "mp3_id.h"

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
size_t  bytesReadMP3;

void Mount_USB(void)
{
	fresult = f_mount(&USBHFatFS, USBHPath, 1);
}

void Unmount_USB(void)
{
	fresult = f_mount(NULL, USBHPath, 1);
}

void Check_USB_Details(void)
{
    /* Check free space */
    f_getfree("1:", &fre_clust_usb, &pfs_usb);

    totalSpace_usb = (uint32_t)((pfs_usb->n_fatent - 2) * pfs_usb->csize * 0.5);
    freeSpace_usb = (uint32_t)(fre_clust_usb * pfs_usb->csize * 0.5);
}

#endif /* SRC_FILE_HANDLING_C_ */
