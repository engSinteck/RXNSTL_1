/*
 * SAA6579.c
 *
 *  Created on: Sep 25, 2025
 *      Author: rinaldo.santos
 */
#include "main.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../Sinteck/src/SAA6579.h"

#define RDS_BUFFER_SIZE 1024
#define MAX_SYNC_ERRORS 10
#define MAX_PS 8
#define MAX_RT 64

enum {
    SYNC_SEARCH = 0,
    SYNC_A,
    SYNC_B,
    SYNC_C,
    SYNC_D
};

typedef struct {
    uint16_t PI;
    uint8_t PTY;
    uint8_t TP;
    uint8_t MS;
    char PS[MAX_PS+1];       // 8 chars + null
    char RT[MAX_RT+1];       // 64 chars + null
    char PTYN[9];            // 8 chars + null
} rds_info_t;

typedef struct {
    uint8_t buf_rds[RDS_BUFFER_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
} ringbuffer_t;

typedef struct {
    uint32_t total_bits;       // bits processados
    uint32_t total_blocks;     // blocos de 26 bits processados
    uint32_t good_blocks;      // blocos validados com CRC
    uint32_t bad_blocks;       // blocos inválidos
    uint32_t total_groups;     // grupos completos decodificados
    uint32_t ps_updates;       // quantas vezes o PS mudou
    uint32_t rt_updates;       // quantas vezes o RT mudou
    uint32_t last_ps_ms;       // tempo (ms) desde último PS válido
    uint32_t last_rt_ms;       // tempo (ms) desde último RT válido
} rds_metrics_t;

static ringbuffer_t rds_rb = {0};
rds_info_t rds_info;
rds_metrics_t rds_stats;

static uint16_t group[4];
static int sync_state = SYNC_SEARCH;
static uint16_t group[4];   			// buffer para 4 blocos de 16 bits
static int expecting_cprime = 0; 		// flag: se aceitou C
static int sync_errors = 0;

// Buffers internos temporários
static char ps_prev[MAX_PS+1] = "        ";
static char rt_prev[MAX_RT+1] = {0};
static char ptyn_prev[9] = "        ";

// RDS Offset Words (10-bit syndromes)
static const uint16_t rds_offset_words[5] = {
    0x0FC, // A
    0x198, // B
    0x168, // C
    0x1B4, // C′
    0x1E2  // D
};

static inline int rds_push(ringbuffer_t *rb, uint8_t val) {
    uint16_t next = (rb->head +1) % RDS_BUFFER_SIZE;
    if(next == rb->tail) return 0; // cheio
    rb->buf_rds[rb->head] = val;
    rb->head = next;
    return 1;
}

static inline int rds_pop(ringbuffer_t *rb, uint8_t *val) {
    if(rb->head == rb->tail) return 0; // vazio
    *val = rb->buf_rds[rb->tail];
    rb->tail = (rb->tail+1) % RDS_BUFFER_SIZE;
    return 1;
}

// Calcula CRC10 dos 16 bits de dados
uint16_t rds_crc10(uint16_t data) {
    uint32_t d = data;
    uint16_t crc = 0;
    for(int i=15;i>=0;i--) {
        uint8_t bit = (d>>i)&1;
        uint8_t crc_msb = (crc >> 9) & 1;
        crc <<= 1;
        if(bit ^ crc_msb) crc ^= 0x3FF; // polinômio 0x3FF = x^10 + x^8 + x^7 + x^5 + x^4 + x^3 + 1
    }
    return crc & 0x3FF;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if(GPIO_Pin == RDCL_Pin) { // RDCL = clock
        uint8_t bit = HAL_GPIO_ReadPin(RDDA_GPIO_Port, RDDA_Pin) ? 1 : 0;
        rds_push(&rds_rb, bit);
    }
}

void decode_af(uint8_t code)
{
    if (code == 0 || code == 205) return; // vazio
    if (code >= 224 && code <= 249) return; // códigos especiais
    uint16_t freq = 87500 + code * 100; // kHz
    printf("AF: %u.%1u MHz\n", freq/1000, (freq%1000)/100);
}

void decode_ct(uint16_t blkC, uint16_t blkD)
{
    // Bloco C: bits 0..13 → MJD MSB
    // Bloco D: bits 0..15 → MJD LSB + hora/minuto/offset
    uint32_t mjd = ((blkC & 0x3FFF) << 17) | ((blkD >> 15) & 0x1FFFF);

    uint16_t days = mjd;
    int y = (int)((days - 15078.2)/365.25);
    int m = (int)((days - 14956.1 - (int)(y*365.25))/30.6001);
    int d = days - 14956 - (int)(y*365.25) - (int)(m*30.6001);
    int month = m - 1;
    int year = y + 1900;
    if (month > 12) { month -= 12; year++; }

    uint8_t hour = (blkD >> 12) & 0x0F;
    uint8_t minute = (blkD >> 6) & 0x3F;
    int8_t offset = (blkD & 0x1F) * 0.5; // meia hora

    printf("CT: %04d-%02d-%02d %02d:%02d UTC%+d\n",
           year, month, d, hour, minute, offset);
}

void decode_ptyn(uint16_t blkC, uint16_t blkD)
{
    char c1 = blkC >> 8;
    char c2 = blkC & 0xFF;
    char c3 = blkD >> 8;
    char c4 = blkD & 0xFF;
    char buf[9];
    buf[0] = c1; buf[1] = c2; buf[2] = c3; buf[3] = c4;
    buf[4] = 0; // restante
    if (strcmp(buf, ptyn_prev) != 0) {
        strncpy(ptyn_prev, buf, 8);
        rds_info.PTYN[0]=c1; rds_info.PTYN[1]=c2;
        rds_info.PTYN[2]=c3; rds_info.PTYN[3]=c4;
        rds_info.PTYN[4]='\0';
        printf("PTYN: %s\n", rds_info.PTYN);
    }
}

void decode_tps(uint16_t blkC, uint16_t blkD)
{
    printf("TPS raw: C=%04X D=%04X\n", blkC, blkD);
    // Pode extrair TA, DI, EON bits conforme a necessidade
}

void update_ps(uint16_t blkB, uint16_t blkD)
{
    uint8_t addr = blkB & 0x3;
    char c1 = blkD >> 8;
    char c2 = blkD & 0xFF;
    rds_info.PS[addr*2] = c1;
    rds_info.PS[addr*2+1] = c2;

    if (addr==3) {
        rds_info.PS[8]='\0';
        if (strcmp(rds_info.PS, ps_prev)!=0) {
            strcpy(ps_prev, rds_info.PS);
            printf("PS: \"%s\"\n", rds_info.PS);
            rds_stats.ps_updates++;
            rds_stats.last_ps_ms = HAL_GetTick();
        }
    }
}

void update_rt(uint8_t version, uint16_t blkB, uint16_t blkC, uint16_t blkD)
{
    if (version==0) { // 2A, 4 chars
        uint8_t addr = blkB & 0xF;
        rds_info.RT[addr*4+0] = blkC >> 8;
        rds_info.RT[addr*4+1] = blkC & 0xFF;
        rds_info.RT[addr*4+2] = blkD >> 8;
        rds_info.RT[addr*4+3] = blkD & 0xFF;
        if (addr==15) {
            rds_info.RT[64]='\0';
            if (strcmp(rds_info.RT, rt_prev)!=0) {
                strcpy(rt_prev, rds_info.RT);
                printf("RT: \"%s\"\n", rds_info.RT);
                rds_stats.rt_updates++;
                rds_stats.last_rt_ms = HAL_GetTick();
            }
        }
    } else { // 2B, 2 chars
        uint8_t addr = blkB & 0xF;
        rds_info.RT[addr*2+0] = blkD >> 8;
        rds_info.RT[addr*2+1] = blkD & 0xFF;
        if (addr==31) {
            rds_info.RT[64]='\0';
            if (strcmp(rds_info.RT, rt_prev)!=0) {
                strcpy(rt_prev, rds_info.RT);
                printf("RT: \"%s\"\n", rds_info.RT);
                rds_stats.rt_updates++;
                rds_stats.last_rt_ms = HAL_GetTick();
            }
        }
    }
}

void process_rds_group(uint16_t *g, int cprime)
{
    uint16_t pi = g[0];
    uint16_t blkB = g[1];
    uint16_t blkC = g[2];
    uint16_t blkD = g[3];

    uint8_t group_type = (blkB >> 12) & 0xF;
    uint8_t version    = (blkB >> 11) & 0x1;
    uint8_t pty        = (blkB >> 5) & 0x1F;
    uint8_t tp         = (blkB >> 10) & 0x1;
    uint8_t ms         = (blkB >> 3) & 0x1;

    rds_info.PI  = pi;
    rds_info.PTY = pty;
    rds_info.TP  = tp;
    rds_info.MS  = ms;

    rds_stats.total_groups++;

    switch (group_type) {
    case 0: update_ps(blkB, blkD); decode_af(blkC>>8); decode_af(blkC&0xFF); break;
    case 2: update_rt(version, blkB, blkC, blkD); break;
    case 4: decode_ct(blkC, blkD); break;
    case 10: decode_ptyn(blkC, blkD); break;
    case 8: decode_tps(blkC, blkD); break;
    default:
        if (cprime) {
            // futuro: tratamento especial
        }
        break;
    }
}

void RDS_Task(void) {
    uint8_t bit;
    static uint32_t shiftreg = 0;
    static int bitcount = 0;

    while(rds_pop(&rds_rb,&bit)) {
        shiftreg = ((shiftreg<<1)|(bit&1)) & 0x3FFFFFF; // 26 bits
        bitcount++;

        if(bitcount>=26) {
            bitcount=0;
            uint16_t data16 = (shiftreg>>10)&0xFFFF;
            uint16_t check10 = shiftreg & 0x3FF;

            int valid=-1;
            for(int off=0; off<5; off++) {
                uint16_t crc = rds_crc10(data16);
                if((crc ^ rds_offset_words[off])==check10) {
                    valid = off;
                    break;
                }
            }

            if(valid>=0) {
                sync_errors=0;

                switch(sync_state) {
                    case SYNC_SEARCH:
                        if(valid==0){ group[0]=data16; sync_state=SYNC_B; }
                        break;
                    case SYNC_B:
                        if(valid==1){ group[1]=data16; sync_state=SYNC_C; }
                        else sync_state=SYNC_SEARCH;
                        break;
                    case SYNC_C:
                        if(valid==2){ group[2]=data16; expecting_cprime=0; sync_state=SYNC_D; }
                        else if(valid==3){ group[2]=data16; expecting_cprime=1; sync_state=SYNC_D; }
                        else sync_state=SYNC_SEARCH;
                        break;
                    case SYNC_D:
                        if(valid==4){ group[3]=data16; process_rds_group(group, expecting_cprime); }
                        sync_state=SYNC_SEARCH;
                        break;
                }
            } else {
                sync_errors++;
                if(sync_errors>MAX_SYNC_ERRORS){ sync_state=SYNC_SEARCH; sync_errors=0; }
            }
        }
    }
}


void RDS_PrintStats(void)
{
    float ber = 0.0f;
    if (rds_stats.total_blocks > 0) {
        ber = 100.0f * (float)rds_stats.bad_blocks / (float)rds_stats.total_blocks;
    }

    printf("=== RDS Stats ===\n");
    printf("Bits: %lu\n", (unsigned long)rds_stats.total_bits);
    printf("Blocks: %lu (Good=%lu Bad=%lu, BER=%.2f%%)\n",
           (unsigned long)rds_stats.total_blocks,
           (unsigned long)rds_stats.good_blocks,
           (unsigned long)rds_stats.bad_blocks,
           ber);
    printf("Groups: %lu\n", (unsigned long)rds_stats.total_groups);
    printf("PS Updates: %lu (Last @ %lums)\n",
           (unsigned long)rds_stats.ps_updates,
           (unsigned long)(HAL_GetTick() - rds_stats.last_ps_ms));
    printf("RT Updates: %lu (Last @ %lums)\n",
           (unsigned long)rds_stats.rt_updates,
           (unsigned long)(HAL_GetTick() - rds_stats.last_rt_ms));
    printf("=================\n");
}

int RDS_GetQuality(void) {
    if (rds_stats.total_blocks < 50) return 0; // ainda sem base suficiente

    float ber = (float)rds_stats.bad_blocks / (float)rds_stats.total_blocks;

    if (ber < 0.01f) return 5;  // Excelente
    if (ber < 0.03f) return 4;  // Bom
    if (ber < 0.07f) return 3;  // Médio
    if (ber < 0.15f) return 2;  // Ruim
    return 1;                   // Péssimo
}

