/*
 * mp3_id.c
 *
 *  Created on: 9 de abr. de 2026
 *      Author: rinaldo.santos
 */


#include "mp3_id.h"
#include "fatfs.h"
#include "stdio.h"

extern FIL USBHFile;

ID3v1Tag tag;
FRESULT fr_id;


void read_mp3_id3v1(void)
{
	UINT bytesRead;

	fr_id = f_open(&USBHFile, "1:test_48.mp3", FA_READ);

	if (fr_id == FR_OK) {
		f_lseek(&USBHFile, f_size(&USBHFile) - 128);
		f_read(&USBHFile, &tag, sizeof(ID3v1Tag), &bytesRead);

		if (strncmp(tag.tag, "TAG", 3) == 0) {
			printf("Título: %.30s\n", tag.title);
			printf("Artista: %.30s\n", tag.artist);
			printf("Álbum: %.30s\n", tag.album);
		}
	}
	f_close(&USBHFile);
}

void read_mp3_id3v2(void)
{
	unsigned char header[10];
	UINT bytesRead;

	fr_id = f_open(&USBHFile, "1:test_48.mp3", FA_READ);

	f_lseek(&USBHFile, 0);
	f_read(&USBHFile, header, 10, &bytesRead);

	if (memcmp(header, "ID3", 3) == 0) {
		// tamanho da tag (bytes 6–9, codificação especial)
//		int size = ((header[6] & 0x7F) << 21) |
//                   ((header[7] & 0x7F) << 14) |
//                   ((header[8] & 0x7F) << 7)  |
//                    (header[9] & 0x7F);

		// ler frames dentro desse tamanho
		unsigned char frameHeader[10];
		f_read(&USBHFile, frameHeader, 10, &bytesRead);
		if (memcmp(frameHeader, "TIT2", 4) == 0) {
			int frameSize = (frameHeader[4] << 24) |
                            (frameHeader[5] << 16) |
                            (frameHeader[6] << 8)  |
                             frameHeader[7];
			f_read(&USBHFile, tag.title_v2, frameSize, &bytesRead);
			printf("Título: %s\n", tag.title_v2+1);						 // +1 para pular byte de codificação
		}
	}
	f_close(&USBHFile);
}
