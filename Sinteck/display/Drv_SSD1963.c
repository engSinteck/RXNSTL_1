/**
 * @file SSD1963.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "main.h"
#include "dma.h"
#include "dma2d.h"
#include "mdma.h"
#include "cmsis_os.h"
#include <display/Drv_SSD1963.h>
#include "../Sinteck/display/Tela2.c"
#include "../Sinteck/display/TelaI.c"
#if USE_SSD1963

#include <stdbool.h>
#include LV_DRV_DISP_INCLUDE
//#include LV_DRV_DELAY_INCLUDE

/*********************
 *      DEFINES
 *********************/
#define SSD1963_CMD_MODE     0
#define SSD1963_DATA_MODE    1

/**********************
 *      TYPEDEFS
 **********************/
#define TFT_CMD              ((volatile uint32_t)0x60000000) /* RS = 0 */
#define TFT_DATA             ((volatile uint32_t)0x60080000) /* RS = 1 */

#define MAX_LINES_PER_CHUNK  16

/**********************
 *  STATIC PROTOTYPES
 **********************/
static inline void drv_ssd1963_data(uint8_t data);
static inline void drv_ssd1963_cmd(uint8_t cmd);

//static inline void drv_ssd1963_cmd_mode(void);
static inline void drv_ssd1963_data_mode(void);
static void drv_ssd1963_reset(void);

/**********************
 *  STATIC VARIABLES
 **********************/
static bool cmd_mode = true;
int32_t pos_bmp;
uint16_t col_x, col_y;

//volatile uint8_t buf_ri[480*16*3] __attribute__((section(".RAM_D1"))) __attribute__((aligned(32)));
/* NOVO BUFFER: Para os dados convertidos de 3 bytes (RGB888) */
uint8_t dma_tx_buf[480 * 16 * 3] __attribute__((aligned(32)));// __attribute__((section(".RAM_D1"))) __attribute__((aligned(32)));;
int32_t ri_i = 0;
int32_t ri_j = 0;
uint32_t size_in_pixels = 0;
uint32_t total_bytes_to_transfer = 0;
uint32_t cache_clean_size = 0;
uint32_t p_ri = 0;
volatile uint8_t dma_transfer_complete = 0;
lv_disp_drv_t* g_disp_drv = NULL;

volatile uint8_t spi_transfer_complete = 1;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void drv_ssd1963_init(void)
{
    cmd_mode = true;

    drv_ssd1963_reset();
    LV_DRV_DELAY_MS(200);

    drv_ssd1963_cmd(0x00E2);    //PLL multiplier, set PLL clock to 120M
    drv_ssd1963_data(0x0023);   //N=0x36 for 6.5M, 0x23 for 10M crystal
    drv_ssd1963_data(0x0002);
    drv_ssd1963_data(0x0004);

    drv_ssd1963_cmd(0x00E0);    // PLL enable
    drv_ssd1963_data(0x0001);
    LV_DRV_DELAY_MS(5);

    drv_ssd1963_cmd(0x00E0);
    drv_ssd1963_data(0x0003);   // now, use PLL output as system clock
    LV_DRV_DELAY_MS(5);

    drv_ssd1963_cmd(0x0001);    // software reset
    LV_DRV_DELAY_MS(5);

    drv_ssd1963_cmd(0x00E6);    //PLL setting for PCLK, depends on resolution
    drv_ssd1963_data(0x0001);  //
    drv_ssd1963_data(0x0047);  //
    drv_ssd1963_data(0x00B1);  //PCLK=9.6MHz

    drv_ssd1963_cmd(0x00B0);    //LCD SPECIFICATION
    drv_ssd1963_data(0x0020);
    drv_ssd1963_data(0x0000);
    drv_ssd1963_data(((SSD1963_HOR_RES - 1) >> 8) & 0X00FF); //Set HDP
    drv_ssd1963_data((SSD1963_HOR_RES - 1) & 0X00FF);
    drv_ssd1963_data(((SSD1963_VER_RES - 1) >> 8) & 0X00FF); //Set VDP
    drv_ssd1963_data((SSD1963_VER_RES - 1) & 0X00FF);
    drv_ssd1963_data(0x0000);

    LV_DRV_DELAY_MS(5);//Delay10us(5);

    drv_ssd1963_cmd(0x00B4);            //HSYNC
    drv_ssd1963_data((SSD1963_HT >> 8) & 0X00FF); //Set HT
    drv_ssd1963_data(SSD1963_HT & 0X00FF);
    drv_ssd1963_data((SSD1963_HPS >> 8) & 0X00FF); //Set HPS
    drv_ssd1963_data(SSD1963_HPS & 0X00FF);
    drv_ssd1963_data(SSD1963_HPW);              //Set HPW
    drv_ssd1963_data((SSD1963_LPS >> 8) & 0X00FF); //SetLPS
    drv_ssd1963_data(SSD1963_LPS & 0X00FF);
    drv_ssd1963_data(0x0000);

    drv_ssd1963_cmd(0x00B6);            //VSYNC
    drv_ssd1963_data((SSD1963_VT >> 8) & 0X00FF); //Set VT
    drv_ssd1963_data(SSD1963_VT & 0X00FF);
    drv_ssd1963_data((SSD1963_VPS >> 8) & 0X00FF); //Set VPS
    drv_ssd1963_data(SSD1963_VPS & 0X00FF);
    drv_ssd1963_data(SSD1963_VPW);              //Set VPW
    drv_ssd1963_data((SSD1963_FPS >> 8) & 0X00FF); //Set FPS
    drv_ssd1963_data(SSD1963_FPS & 0X00FF);

    drv_ssd1963_cmd(0x00B8);
    drv_ssd1963_data(0x0007);    //GPIO is controlled by host GPIO[3:0]=output   GPIO[0]=1  LCD ON  GPIO[0]=1  LCD OFF
    drv_ssd1963_data(0x0001);    //GPIO0 normal

    drv_ssd1963_cmd(0x00BA);
    drv_ssd1963_data(0x000F);    //GPIO[0] out 1 --- LCD display on/off control PIN

    drv_ssd1963_cmd(0x0036);    //rotation
    drv_ssd1963_data(0x0010);   //RGB=BGR

    drv_ssd1963_cmd(0x003A);    //Set the current pixel format for RGB image data
    drv_ssd1963_data(0x0050);   //16-bit/pixel

    drv_ssd1963_cmd(0x00F0);    //Pixel Data Interface Format
    drv_ssd1963_data(0x0000);   //16-bit(565 format) data

    drv_ssd1963_cmd(0x00BC);
    drv_ssd1963_data(0x0040);   //contrast value
    drv_ssd1963_data(0x0080);   //brightness value
    drv_ssd1963_data(0x0040);   //saturation value
    drv_ssd1963_data(0x0001);   //Post Processor Enable

    LV_DRV_DELAY_MS(5);

    drv_ssd1963_cmd(0x2a);			//SET page address
    drv_ssd1963_data(0x00);			//SET start page address
    drv_ssd1963_data(0x00);
    drv_ssd1963_data((SSD1963_HOR_RES - 1) >> 8);		//SET end page address
    drv_ssd1963_data((SSD1963_HOR_RES - 1) & 0xFF);

    drv_ssd1963_cmd(0x2b);			//SET column address
    drv_ssd1963_data(0x00);				//SET start column address
    drv_ssd1963_data(0x00);
    drv_ssd1963_data((SSD1963_VER_RES - 1) >> 8);		//SET end column address
    drv_ssd1963_data((SSD1963_VER_RES - 1) & 0xFF);

    drv_ssd1963_cmd(0x2c);

    for (uint16_t x=0; x < SSD1963_HOR_RES; x++) {
    	for (uint16_t y= 0; y < SSD1963_VER_RES; y++) {
    		drv_ssd1963_data( 0x00 ); 		// color is red
    		drv_ssd1963_data( 0x00 );  		// color is green
    		drv_ssd1963_data( 0x00 ); 		// color is blue
    	}
    }

    drv_ssd1963_cmd(0x29); //display on

    drv_ssd1963_cmd(0xBE); //set PWM for B/L
    drv_ssd1963_data(0x06);
    drv_ssd1963_data(0xF0);
    drv_ssd1963_data(0x01);
    drv_ssd1963_data(0xf0);
    drv_ssd1963_data(0x00);
    drv_ssd1963_data(0x00);

    drv_ssd1963_cmd(0x00d0);
    drv_ssd1963_data(0x000d);

    drv_ssd1963_cmd(0x2c);

    LV_DRV_DELAY_MS(50);
}

void drv_ssd1963_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    /*Return if the area is out the screen*/
    if(area->x2 < 0) return;
    if(area->y2 < 0) return;
    if(area->x1 > SSD1963_HOR_RES - 1) return;
    if(area->y1 > SSD1963_VER_RES - 1) return;

    /*Truncate the area to the screen*/
    int32_t act_x1 = area->x1 < 0 ? 0 : area->x1;
    int32_t act_y1 = area->y1 < 0 ? 0 : area->y1;
    int32_t act_x2 = area->x2 > SSD1963_HOR_RES - 1 ? SSD1963_HOR_RES - 1 : area->x2;
    int32_t act_y2 = area->y2 > SSD1963_VER_RES - 1 ? SSD1963_VER_RES - 1 : area->y2;

    //Set the rectangular area
    drv_ssd1963_cmd(0x002A);
    drv_ssd1963_data(act_x1 >> 8);
    drv_ssd1963_data(0x00FF & act_x1);
    drv_ssd1963_data(act_x2 >> 8);
    drv_ssd1963_data(0x00FF & act_x2);

    drv_ssd1963_cmd(0x002B);
    drv_ssd1963_data((act_y1 + OFFSET_Y) >> 8);
    drv_ssd1963_data(0x00FF & (act_y1 + OFFSET_Y));
    drv_ssd1963_data((act_y2 + OFFSET_Y) >> 8);
    drv_ssd1963_data(0x00FF & (act_y2 + OFFSET_Y));
    drv_ssd1963_cmd(0x2c);
    drv_ssd1963_data_mode();

#if LV_COLOR_DEPTH == 16
    uint16_t act_w = act_x2 - act_x1 + 1;
    for(i = act_y1; i <= act_y2; i++) {
        LV_DRV_DISP_PAR_WR_ARRAY((uint16_t *)color_p, act_w);
        color_p += full_w;
    }
    LV_DRV_DISP_PAR_CS(1);
#else
    int32_t size = (act_x2 - act_x1 + 1) * (act_y2 - act_y1 + 1);
    for(int32_t i = 0; i <= size-1; i++) {
    	drv_ssd1963_data(color_p->ch.red); 			// color red
    	drv_ssd1963_data(color_p->ch.green); 		// color green
    	drv_ssd1963_data(color_p->ch.blue); 		// color blue
    	color_p++;
    }
#endif

    lv_disp_flush_ready(disp_drv);                  /* Tell you are ready with the flushing*/
}

void drv_ssd1963_flush_2(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
	// Return if the area is out the screen
	if(area->x2 < 0) return;
	if(area->y2 < 0) return;
	if(area->x1 > SSD1963_HOR_RES - 1) return;
	if(area->y1 > SSD1963_VER_RES - 1) return;

	// Truncate the area to the screen
	int32_t act_x1 = area->x1 < 0 ? 0 : area->x1;
	int32_t act_y1 = area->y1 < 0 ? 0 : area->y1;
	int32_t act_x2 = area->x2 > SSD1963_HOR_RES - 1 ? SSD1963_HOR_RES - 1 : area->x2;
	int32_t act_y2 = area->y2 > SSD1963_VER_RES - 1 ? SSD1963_VER_RES - 1 : area->y2;

	// Set the rectangular area
	drv_ssd1963_cmd(0x002A);
	drv_ssd1963_data(act_x1 >> 8);
	drv_ssd1963_data(0x00FF & act_x1);
	drv_ssd1963_data(act_x2 >> 8);
	drv_ssd1963_data(0x00FF & act_x2);

	drv_ssd1963_cmd(0x002B);
	drv_ssd1963_data((act_y1 + OFFSET_Y) >> 8);
	drv_ssd1963_data(0x00FF & (act_y1 + OFFSET_Y));
	drv_ssd1963_data((act_y2 + OFFSET_Y) >> 8);
	drv_ssd1963_data(0x00FF & (act_y2 + OFFSET_Y));
	drv_ssd1963_cmd(0x2c);
	drv_ssd1963_data_mode();

#if LV_COLOR_DEPTH == 16
	uint16_t act_w = act_x2 - act_x1 + 1;
	for(i = act_y1; i <= act_y2; i++) {
		LV_DRV_DISP_PAR_WR_ARRAY((uint16_t *)color_p, act_w);
	    color_p += full_w;
	}
	LV_DRV_DISP_PAR_CS(1);
#else
	for(int32_t i = act_y1; i <= act_y2; i++) {
		for(int32_t j = act_x1; j <= act_x2; j++) {
			drv_ssd1963_data(color_p->ch.red); 			// color red
			drv_ssd1963_data(color_p->ch.green); 		// color green
	        drv_ssd1963_data(color_p->ch.blue); 		// color blue
	        color_p++;
		}
	}
#endif
	lv_disp_flush_ready(disp_drv);						// Tell you are ready with the flushing
}

void drv_ssd1963_flush_3(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
	// Truncate the area to the screen
	int32_t act_x1 = area->x1 < 0 ? 0 : area->x1;
	int32_t act_y1 = area->y1 < 0 ? 0 : area->y1;
	int32_t act_x2 = area->x2 > SSD1963_HOR_RES - 1 ? SSD1963_HOR_RES - 1 : area->x2;
	int32_t act_y2 = area->y2 > SSD1963_VER_RES - 1 ? SSD1963_VER_RES - 1 : area->y2;

	size_in_pixels = lv_area_get_size(area);

	// Set the rectangular area
	drv_ssd1963_cmd(0x002A);
	drv_ssd1963_data(act_x1 >> 8);
	drv_ssd1963_data(0x00FF & act_x1);
	drv_ssd1963_data(act_x2 >> 8);
	drv_ssd1963_data(0x00FF & act_x2);

	drv_ssd1963_cmd(0x002B);
	drv_ssd1963_data((act_y1 + OFFSET_Y) >> 8);
	drv_ssd1963_data(0x00FF & (act_y1 + OFFSET_Y));
	drv_ssd1963_data((act_y2 + OFFSET_Y) >> 8);
	drv_ssd1963_data(0x00FF & (act_y2 + OFFSET_Y));
	drv_ssd1963_cmd(0x2c);
	drv_ssd1963_data_mode();

#if LV_COLOR_DEPTH == 16
	uint16_t act_w = act_x2 - act_x1 + 1;
	for(i = act_y1; i <= act_y2; i++) {
		LV_DRV_DISP_PAR_WR_ARRAY((uint16_t *)color_p, act_w);
	    color_p += full_w;
	}
	LV_DRV_DISP_PAR_CS(1);
#else
	for(int32_t i = act_y1; i <= act_y2; i++) {
		for(int32_t j = act_x1; j <= act_x2; j++) {
			*(__IO uint8_t *)(TFT_DATA) = color_p->ch.red;		// color red
			*(__IO uint8_t *)(TFT_DATA) = color_p->ch.green;	// color green
			*(__IO uint8_t *)(TFT_DATA) = color_p->ch.blue;		// color blue
	        color_p++;
		}
	}
#endif
	lv_disp_flush_ready(disp_drv);								// Tell you are ready with the flushing
}

void ssd1963_fill(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    /*Return if the area is out the screen*/
    if(area->x2 < 0) return;
    if(area->y2 < 0) return;
    if(area->x1 > SSD1963_HOR_RES - 1) return;
    if(area->y1 > SSD1963_VER_RES - 1) return;

    /*Truncate the area to the screen*/
    int32_t act_x1 = area->x1 < 0 ? 0 : area->x1;
    int32_t act_y1 = area->y1 < 0 ? 0 : area->y1;
    int32_t act_x2 = area->x2 > SSD1963_HOR_RES - 1 ? SSD1963_HOR_RES - 1 : area->x2;
    int32_t act_y2 = area->y2 > SSD1963_VER_RES - 1 ? SSD1963_VER_RES - 1 : area->y2;

    //Set the rectangular area
    drv_ssd1963_cmd(0x002A);
    drv_ssd1963_data(act_x1 >> 8);
    drv_ssd1963_data(0x00FF & act_x1);
    drv_ssd1963_data(act_x2 >> 8);
    drv_ssd1963_data(0x00FF & act_x2);

    drv_ssd1963_cmd(0x002B);
    drv_ssd1963_data((act_y1 + OFFSET_Y) >> 8);
    drv_ssd1963_data(0x00FF & (act_y1 + OFFSET_Y));
    drv_ssd1963_data((act_y2 + OFFSET_Y) >> 8);
    drv_ssd1963_data(0x00FF & (act_y2 + OFFSET_Y));

    drv_ssd1963_cmd(0x2c);

    LV_DRV_DISP_PAR_CS(0);
    drv_ssd1963_data_mode();

    int32_t size = (act_x2 - act_x1 + 1) * (act_y2 - act_y1 + 1);
    int32_t i;
    for(i = 0; i < size-1; i++) {
    	drv_ssd1963_data(color_p->ch.red); 			// color red
    	drv_ssd1963_data(color_p->ch.green); 		// color green
    	drv_ssd1963_data(color_p->ch.blue); 		// color blue
    }
    LV_DRV_DISP_PAR_CS(1);
}

void drv_ssd1963_map(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    /*Return if the area is out the screen*/
    if(area->x2 < 0) return;
    if(area->y2 < 0) return;
    if(area->x1 > SSD1963_HOR_RES - 1) return;
    if(area->y1 > SSD1963_VER_RES - 1) return;

    /*Truncate the area to the screen*/
    int32_t act_x1 = area->x1 < 0 ? 0 : area->x1;
    int32_t act_y1 = area->y1 < 0 ? 0 : area->y1;
    int32_t act_x2 = area->x2 > SSD1963_HOR_RES - 1 ? SSD1963_HOR_RES - 1 : area->x2;
    int32_t act_y2 = area->y2 > SSD1963_VER_RES - 1 ? SSD1963_VER_RES - 1 : area->y2;

    //Set the rectangular area
    drv_ssd1963_cmd(0x002A);
    drv_ssd1963_data(act_x1 >> 8);
    drv_ssd1963_data(0x00FF & act_x1);
    drv_ssd1963_data(act_x2 >> 8);
    drv_ssd1963_data(0x00FF & act_x2);

    drv_ssd1963_cmd(0x002B);
    drv_ssd1963_data((act_y1 + OFFSET_Y) >> 8);
    drv_ssd1963_data(0x00FF & (act_y1 + OFFSET_Y));
    drv_ssd1963_data((act_y2 + OFFSET_Y) >> 8);
    drv_ssd1963_data(0x00FF & (act_y2 + OFFSET_Y));

    drv_ssd1963_cmd(0x2c);
    int16_t i;
    int16_t full_w = area->x2 - area->x1 + 1;

    LV_DRV_DISP_PAR_CS(0);
    drv_ssd1963_data_mode();

#if LV_COLOR_DEPTH == 16
    uint16_t act_w = act_x2 - act_x1 + 1;
    for(i = act_y1; i <= act_y2; i++) {
        LV_DRV_DISP_PAR_WR_ARRAY((uint16_t *)color_p, act_w);
        color_p += full_w;
    }
    LV_DRV_DISP_PAR_CS(1);
#else
    int16_t j;
    for(i = act_y1; i <= act_y2; i++) {
        for(j = 0; j <= act_x2 - act_x1 + 1; j++) {
            LV_DRV_DISP_PAR_WR_WORD(color_p[j]);
            color_p += full_w;
        }
    }
#endif
}

void drv_ssd1963_gpu(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf, const lv_area_t * dest_area, const lv_area_t * fill_area, lv_color_t color)
{

}

void drv_ssd1963_blend(lv_disp_drv_t * disp_drv, lv_color_t * dest, const lv_color_t * src, uint32_t length, lv_opa_t opa)
{

}

void drv_monitor_cb(lv_disp_drv_t * disp_drv, uint32_t time, uint32_t px)
{
  //printf("%d px refreshed in %d ms\n", time, ms);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

//static void ssd1963_io_init(void)
//{
//    LV_DRV_DISP_CMD_DATA(SSD1963_CMD_MODE);
//    cmd_mode = true;
//}

static void drv_ssd1963_reset(void)
{
    /*Hardware reset*/

    /*Software reset*/
    drv_ssd1963_cmd(0x01);
    LV_DRV_DELAY_MS(20);

    drv_ssd1963_cmd(0x01);
    LV_DRV_DELAY_MS(20);

    drv_ssd1963_cmd(0x01);
    LV_DRV_DELAY_MS(100);
}

/**
 * Command mode
 */
//static inline void drv_ssd1963_cmd_mode(void)
//{
//    if(cmd_mode == false) {
//        LV_DRV_DISP_CMD_DATA(SSD1963_CMD_MODE);
//        cmd_mode = true;
//    }
//}

/**
 * Data mode
 */
static inline void drv_ssd1963_data_mode(void)
{
    if(cmd_mode != false) {
        LV_DRV_DISP_CMD_DATA(SSD1963_DATA_MODE);
        cmd_mode = false;
    }
}

/**
 * Write command
 * @param cmd the command
 */
static inline void drv_ssd1963_cmd(uint8_t cmd)
{
	*(__IO uint8_t *)(TFT_CMD) = cmd;
}

/**
 * Write data
 * @param data the data
 */
static inline void drv_ssd1963_data(uint8_t data)
{
	*(__IO uint8_t *)(TFT_DATA) = data;
}


void drv_ssd1963_SetBacklight(uint8_t intensity)
{
	drv_ssd1963_cmd(0xBE);			// Set PWM configuration for backlight control
	drv_ssd1963_data(0x06);			// PWMF[7:0] = 2, PWM base freq = PLL/(256*(1+5))/256 =
									// 300Hz for a PLL freq = 120MHz
	drv_ssd1963_data(intensity);	// Set duty cycle, from 0x00 (total pull-down) to 0xFF
									// (99% pull-up , 255/256)
	drv_ssd1963_data(0x01);			// PWM enabled and controlled by host (mcu)
	drv_ssd1963_data(0x00);
	drv_ssd1963_data(0x00);
	drv_ssd1963_data(0x00);
}

void my_monitor_cb(lv_disp_drv_t * disp_drv, uint32_t time, uint32_t px)
{
	//logI("Debug: %d px refreshed in %d ms\n", px, time);
}

void drv_ssd1963_clear(void)
{
    drv_ssd1963_cmd(0x2a);			//SET page address
    drv_ssd1963_data(0x00);			//SET start page address
    drv_ssd1963_data(0x00);
    drv_ssd1963_data((SSD1963_HOR_RES - 1) >> 8);		//SET end page address
    drv_ssd1963_data((SSD1963_HOR_RES - 1) & 0xFF);

    drv_ssd1963_cmd(0x2b);			//SET column address
    drv_ssd1963_data(OFFSET_Y >> 8);				//SET start column address
    drv_ssd1963_data( 0x00FF & OFFSET_Y);
    drv_ssd1963_data((SSD1963_VER_RES - 1) >> 8);		//SET end column address
    drv_ssd1963_data((SSD1963_VER_RES - 1) & 0xFF);

    drv_ssd1963_cmd(0x2c);

    for (uint16_t x = 0; x < SSD1963_HOR_RES; x++) {
    	for (uint16_t y= 0; y < SSD1963_VER_RES; y++) {
    		drv_ssd1963_data( 0x00 );
    		drv_ssd1963_data( 0x00 );
    		drv_ssd1963_data( 0x00 );
    	}
    }
}

void drv_ssd1963_bmp(void)
{
    drv_ssd1963_cmd(0x2a);								//SET page address
    drv_ssd1963_data(0x00);								//SET start page address
    drv_ssd1963_data(0x00);
    drv_ssd1963_data((SSD1963_HOR_RES - 1) >> 8);		//SET end page address
    drv_ssd1963_data((SSD1963_HOR_RES - 1) & 0xFF);

    drv_ssd1963_cmd(0x2b);								//SET column address
    drv_ssd1963_data(OFFSET_Y >> 8);					//SET start column address
    drv_ssd1963_data( 0x00FF & OFFSET_Y);
    drv_ssd1963_data((SSD1963_VER_RES - 1) >> 8);		//SET end column address
    drv_ssd1963_data((SSD1963_VER_RES - 1) & 0xFF);

    drv_ssd1963_cmd(0x2c);

    pos_bmp = 0;
    for (col_x = 0; col_x < SSD1963_HOR_RES; col_x++) {
    	for (col_y= 0; col_y < 128; col_y++) {
    		drv_ssd1963_data((uint8_t)_acTela2[pos_bmp+0]);
    		drv_ssd1963_data((uint8_t)_acTela2[pos_bmp+1]);
    		drv_ssd1963_data((uint8_t)_acTela2[pos_bmp+2]);
    		pos_bmp += 3;
    	}
    }
}

/**
 * @brief O Flush Callback do LVGL
 */

void ssd1963_flush_dma(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
	g_disp_drv = drv; // Salva o driver globalmente

    // Aguardar transferência anterior completar
    //while(!spi_transfer_complete) {};

	size_in_pixels = lv_area_get_size(area);
	// 2. CALCULAR O NÚMERO TOTAL DE BYTES
	// Para cada pixel, enviamos 3 bytes (R, G, B)
	total_bytes_to_transfer = size_in_pixels * 3;

	uint32_t i;
	lv_color_t* src_buf = color_p;
	uint8_t* dst_buf = dma_tx_buf;

	for (i = 0; i < size_in_pixels; i++) {
		// A struct lv_color_t na v6 tem sub-campos .ch.red, .ch.green, etc.
	    // Se LV_COLOR_DEPTH_32 for BGR:
	    // *dst_buf++ = src_buf[i].ch.red;
	    // *dst_buf++ = src_buf[i].ch.green;
	    // *dst_buf++ = src_buf[i].ch.blue;

	    // Se a ordem de bytes da lv_color_t for ARGB (A=msb, B=lsb)
	    // (A forma mais segura de aceder, independentemente da ordem)
	    *dst_buf++ = src_buf[i].ch.red;
	    *dst_buf++ = src_buf[i].ch.green;
	    *dst_buf++ = src_buf[i].ch.blue;
	}

    //Set the rectangular area
    drv_ssd1963_cmd(0x002A);
    drv_ssd1963_data(area->x1 >> 8);
    drv_ssd1963_data(0x00FF & area->x1);
    drv_ssd1963_data(area->x2 >> 8);
    drv_ssd1963_data(0x00FF & area->x2);

    drv_ssd1963_cmd(0x002B);
    drv_ssd1963_data((area->y1 + OFFSET_Y) >> 8);
    drv_ssd1963_data(0x00FF & (area->y1 + OFFSET_Y));
    drv_ssd1963_data((area->y2 + OFFSET_Y) >> 8);
    drv_ssd1963_data(0x00FF & (area->y2 + OFFSET_Y));

    drv_ssd1963_cmd(0x2c);

    // 3. Gerenciamento de Cache (CRÍTICO NO H7)
    // Limpa o D-Cache para garantir que os dados em SRAM (renderizados pela CPU)
    // sejam escritos na memória principal antes do DMA lê-los.
    //SCB_CleanInvalidateDCache();
    // Certifique-se que o buffer de origem da cache é sempre alinhado para 32 bytes
    // e o tamanho também é arredondado.
    #define CACHE_LINE_SIZE 32
    #define ROUND_UP_32(x) (((x) + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1))

    // A única mudança: arredonde o tamanho da limpeza para o próximo múltiplo de 32
    cache_clean_size = ROUND_UP_32(total_bytes_to_transfer);
    SCB_CleanDCache_by_Addr((uint32_t*)dma_tx_buf, cache_clean_size);

	// Iniciar transferência DMA
    spi_transfer_complete = 0;

    // 4. Iniciar o DMA (modo M2P)
//    printf("DMA Debug:\n");
//    printf(" - Source:      %p (dma_tx_buf)\n", dma_tx_buf);
//    printf(" - Destination: %p (TFT_DATA)\n", TFT_DATA);
//    printf(" - Size:        %lu bytes\n", total_bytes_to_transfer);
//    printf(" - Config: PeripheralInc=%s, MemoryInc=%s\n",
//               hdma_memtomem_dma1_stream1.Init.PeriphInc == DMA_PINC_ENABLE ? "ENABLE" : "DISABLE",
//               hdma_memtomem_dma1_stream1.Init.MemInc == DMA_MINC_ENABLE ? "ENABLE" : "DISABLE");

    // Assumindo hdma_memtomem_dma1_stream1 e que LCD_DATA_ADDR é 0x60080000
    HAL_DMA_Start(&hdma_memtomem_dma1_stream1,
                    (uint32_t)dma_tx_buf,           // Endereço de Origem (SRAM)
                    (uint32_t)TFT_DATA,     		// Endereço de Destino (FMC)
					total_bytes_to_transfer);      // Número de transferências (em Half-Words)

    // 2. AGUARDAR ATÉ O FIM DA TRANSFERÊNCIA (Modo Síncrono/Polling)
    // Isso garante que o buffer está livre e o DMA reconfigura-se.
    HAL_StatusTypeDef status = HAL_DMA_PollForTransfer(&hdma_memtomem_dma1_stream1, HAL_DMA_FULL_TRANSFER, 100); // Timeout em ms

    // 4. Se a transferência foi bem-sucedida
    if (status == HAL_OK) {
    	lv_disp_flush_ready(drv);
    } else {
        // Lidar com o erro
    }
}

/**
 * @brief Callback de interrupção do DMA (quando a transferência termina)
 */
void MyMemToMemCpltCallback(DMA_HandleTypeDef *hdma)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	if (hdma == &hdma_memtomem_dma1_stream1) {
		vTaskNotifyGiveFromISR(xTaskGetCurrentTaskHandle(), &xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}

// Esta função será chamada pelo HAL_MDMA_IRQHandler
void HAL_MDMA_CpltCallback(MDMA_HandleTypeDef *hmdma)
{
    if (hmdma->Instance == MDMA_Channel0)
    {
        // O MDMA trata da limpeza das flags de forma mais robusta que o DMA.
        lv_disp_flush_ready(g_disp_drv);
    }
}

void HAL_MDMA_ErrorCallback(MDMA_HandleTypeDef *hmdma)
{
    // Se este for atingido, a transferência está a falhar.
    // Coloque um breakpoint aqui para verificar se há erros de bus ou transferência.
    // A falha pode ser no acesso ao FMC.
    __NOP();
}

void SSD1963_Flush_DMA2D(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
	uint32_t width  = area->x2 - area->x1 + 1;
	    //uint32_t height = area->y2 - area->y1 + 1;
	    //uint32_t num_pixels = width * height;
	    static uint8_t rgb888_buf[800*3]; // 1 linha máx (ajuste conforme necessário)

	    for (uint32_t y = area->y1; y <= area->y2; y++) {
	        // Converte 1 linha (ARGB8888 -> RGB888)
	        DMA2D->CR = 0;
	        DMA2D->FGMAR = (uint32_t)(color_p + (y - area->y1) * width);
	        DMA2D->OMAR  = (uint32_t)rgb888_buf;
	        DMA2D->NLR = (width << 16) | 1;
	        DMA2D->FGPFCCR = DMA2D_INPUT_ARGB8888;
	        DMA2D->OPFCCR  = DMA2D_OUTPUT_RGB888;
	        DMA2D->CR = DMA2D_M2M | DMA2D_CR_START;
	        while (!(DMA2D->ISR & DMA2D_FLAG_TC));
	        DMA2D->IFCR = DMA2D_FLAG_TC;

	        // Define janela e envia a linha
	        if (y == area->y1) {
	            //Set the rectangular area
	            drv_ssd1963_cmd(0x002A);
	            drv_ssd1963_data(area->x1 >> 8);
	            drv_ssd1963_data(0x00FF & area->x1);
	            drv_ssd1963_data(area->x2 >> 8);
	            drv_ssd1963_data(0x00FF & area->x2);

	            drv_ssd1963_cmd(0x002B);
	            drv_ssd1963_data((area->y1 + OFFSET_Y) >> 8);
	            drv_ssd1963_data(0x00FF & (area->y1 + OFFSET_Y));
	            drv_ssd1963_data((area->y2 + OFFSET_Y) >> 8);
	            drv_ssd1963_data(0x00FF & (area->y2 + OFFSET_Y));

	            drv_ssd1963_cmd(0x2c);
	        }

	        HAL_DMA_Start(&hdma_memtomem_dma1_stream1,
	                      (uint32_t)rgb888_buf,
	                      (uint32_t)TFT_DATA,
	                      width * 3);
	        HAL_DMA_PollForTransfer(&hdma_memtomem_dma1_stream1, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
	    }

	    lv_disp_flush_ready(disp_drv);
}


void DMA_TransferComplete(DMA_HandleTypeDef *hdma)
{
	for(volatile int i = 0; i < 100; i++);

    dma_transfer_complete = 1;

    // Notifica o LVGL que o flush está completo
    if(g_disp_drv != NULL) {
        lv_disp_flush_ready(g_disp_drv);
    }
}

void DMA_TransferError(DMA_HandleTypeDef *hdma)
{
	for(volatile int i = 0; i < 100; i++);

	// Tratamento de erro - você pode querer tentar novamente ou reportar
    dma_transfer_complete = 1;
    if(g_disp_drv != NULL) {
        lv_disp_flush_ready(g_disp_drv);
    }
}

void ssd1963_flush_dma_d(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    // Aguardar transferência anterior completar (timeout para evitar deadlock)
//    uint32_t timeout = 1000; // 1 segundo timeout
//    while(!dma_transfer_complete && timeout--) {
//        vTaskDelay(1); // Se usando FreeRTOS
//    }
//
//    if(!dma_transfer_complete) {
//        // Timeout - forçar finalização e reportar erro
//        HAL_DMA_Abort(&hdma_memtomem_dma1_stream1);
//        lv_disp_flush_ready(drv);
//        return;
//    }

    g_disp_drv = drv;
    dma_transfer_complete = 0;

    size_in_pixels = lv_area_get_size(area);
    total_bytes_to_transfer = size_in_pixels * 3;

    // Converter cores para formato RGB888
    lv_color_t* src_buf = color_p;
    uint8_t* dst_buf = dma_tx_buf;

    for(uint32_t i = 0; i < size_in_pixels; i++) {
        *dst_buf++ = src_buf[i].ch.red;
        *dst_buf++ = src_buf[i].ch.green;
        *dst_buf++ = src_buf[i].ch.blue;
    }

    // Configurar área de display
    drv_ssd1963_cmd(0x002A);
    drv_ssd1963_data(area->x1 >> 8);
    drv_ssd1963_data(0x00FF & area->x1);
    drv_ssd1963_data(area->x2 >> 8);
    drv_ssd1963_data(0x00FF & area->x2);

    drv_ssd1963_cmd(0x002B);
    drv_ssd1963_data((area->y1 + OFFSET_Y) >> 8);
    drv_ssd1963_data(0x00FF & (area->y1 + OFFSET_Y));
    drv_ssd1963_data((area->y2 + OFFSET_Y) >> 8);
    drv_ssd1963_data(0x00FF & (area->y2 + OFFSET_Y));

    drv_ssd1963_cmd(0x2c); // Comando para iniciar escrita de dados

    // Gerenciamento de cache (CRÍTICO)
    SCB_CleanDCache_by_Addr((uint32_t*)dma_tx_buf,
    ROUND_UP_32(total_bytes_to_transfer));

    // Iniciar transferência DMA assíncrona
    HAL_DMA_Start(&hdma_memtomem_dma1_stream1,
                 (uint32_t)dma_tx_buf,
                 (uint32_t)TFT_DATA,
                 total_bytes_to_transfer);

    // 2. AGUARDAR ATÉ O FIM DA TRANSFERÊNCIA (Modo Síncrono/Polling)
    // Isso garante que o buffer está livre e o DMA reconfigura-se.
    HAL_StatusTypeDef status = HAL_DMA_PollForTransfer(&hdma_memtomem_dma1_stream1, HAL_DMA_FULL_TRANSFER, 100); // Timeout em ms

    if(status == HAL_OK) {
        // Se falhar ao iniciar DMA, fallback para método síncrono
        //dma_transfer_complete = 1;
        lv_disp_flush_ready(drv);
    }

    // A CPU está livre agora - o callback será chamado quando a transferência completar
}
#endif
