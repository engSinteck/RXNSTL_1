/*
 * file_handling.h
 *
 *  Created on: 8 de abr. de 2026
 *      Author: rinaldo.santos
 */

#ifndef SRC_FILE_HANDLING_H_
#define SRC_FILE_HANDLING_H_

void Mount_USB (void);
void Unmount_USB (void);
void Check_USB_Details(void);
void Read_MP3_File(void);
void Fill_I2S_From_PCM(short *pcm, int samples);
void Send_I2S_buffer_MP3(int samples);
void FillNextBuffer(int32_t *i2s_buffer);
void Start_MP3_Playback(void);
void fill_i2s_dma(int32_t *buf, int samples);

#endif /* SRC_FILE_HANDLING_H_ */
