/*
 * mp3_id.h
 *
 *  Created on: 9 de abr. de 2026
 *      Author: rinaldo.santos
 */

#ifndef SRC_MP3_ID_H_
#define SRC_MP3_ID_H_

#include "main.h"

typedef struct {
    char tag[3];     // "TAG"
    char title[30];
    char artist[30];
    char album[30];
    char year[4];
    char comment[30];
    unsigned char genre;
    // V2
    char title_v2[64];
} ID3v1Tag;

void read_mp3_id3v1(void);
void read_mp3_id3v2(void);

#endif /* SRC_MP3_ID_H_ */
