/*
 * screen_main.c
 *
 *  Created on: 28 de jun de 2019
 *      Author: Rinaldo Dos Santos
 *      Sinteck Next
 */

#include <GUI/screen_main_RX.h>
#include "main.h"
#include "stdio.h"


extern RTC_DateTypeDef gDate;
extern RTC_TimeTypeDef gTime;

void create_rx_vumeter(void);
void create_rx_vumeter_lr(void);
void create_rx_rds(void);
void create_rx_frequency(void);
void create_rx_status(void);
void create_rx_signal(void);
void create_rx_image(void);
void create_rx_clock(void);
void create_rx_buttons(void);
void update_main_screen_rx(lv_task_t * param);
void update_rx_vumeter_main_lr(uint32_t value);
void update_rx_vumeter(uint32_t value);

LV_IMG_DECLARE(TELA_RX_NSTL);
LV_IMG_DECLARE(LED_NSTL_VM);
LV_IMG_DECLARE(LED_NSTL_VD);
LV_IMG_DECLARE(LED_NSTL_AZ);
LV_IMG_DECLARE(LED_NSTL_AM);
LV_IMG_DECLARE(LINK_RX);
LV_IMG_DECLARE(RDS_RX);
LV_IMG_DECLARE(STEREO_RX);
LV_IMG_DECLARE(NUVEM_RX);

static lv_style_t style_fundo;
static lv_style_t style_btn_am;
static lv_style_t style_btn;
static lv_style_t style_btn_status_vm;
static lv_style_t style_btn_status_vd;

static lv_obj_t * Tela_Principal;
static lv_obj_t * img_fundo;
static lv_obj_t * ui_txt_vu[8];
static lv_obj_t * ui_vumeter_x1[50];
static lv_obj_t * ui_vumeter_x2[50];
static lv_obj_t * ui_vumeter_l[12];
static lv_obj_t * ui_vumeter_r[12];
static lv_obj_t * ui_vumeter_m1[12];
static lv_obj_t * ui_vumeter_m2[12];
static lv_obj_t * ui_txt_rds;
static lv_obj_t * ui_text_frequency;
static lv_obj_t * ui_text_status;
static lv_obj_t * ui_status;
static lv_obj_t * ui_text_date;
static lv_obj_t * ui_text_ampm;
static lv_obj_t * ui_text_clock;
static lv_obj_t * btn_nstl[6];
static lv_obj_t * btn_nstl_label[6];
static lv_obj_t * btn_status;
static lv_obj_t * ui_text_signal[3];
static lv_obj_t * img_rx1;
static lv_obj_t * img_rx2;
static lv_obj_t * img_rx3;
static lv_obj_t * img_rx4;

static lv_task_t * Task_Principal;

uint32_t flag_rx_vumeter_lr = 0;
uint32_t demo_rx_vumeter_lr = 0;

uint32_t flag_rx_vumeter = 0;
uint32_t demo_rx_vumeter = 0;

extern const char *MES[];

const uint32_t pos_led_vumeter_x[48] = {
		23, 29, 33, 39, 44, 50, 55, 61, 66, 72,
		78, 84, 89, 94, 100, 105, 111, 117, 123, 129,
		135, 140, 145, 151, 156, 162, 167, 173, 178, 184,
		190, 196, 202, 208, 214, 220, 226, 232, 238, 244,
		250, 256, 262, 268, 274, 279, 284, 290
};

const uint32_t pos_led_vumeter_l[12] = {
		314, 320, 325, 330, 336, 341, 346, 352, 357, 362, 368, 373
};

void main_screen_RX(void)
{
	// Create a Screen
	Tela_Principal = lv_obj_create(NULL, NULL);

	lv_style_copy(&style_fundo, &lv_style_plain_color);
	style_fundo.body.main_color = LV_COLOR_BLACK;
	style_fundo.body.grad_color = LV_COLOR_BLACK;
	lv_obj_set_style(Tela_Principal, &style_fundo); 					// Configura o estilo criado

	// Imagem de Fundo
	img_fundo = lv_img_create(Tela_Principal, NULL);
	lv_img_cache_invalidate_src(NULL);
	lv_img_set_src(img_fundo, &TELA_RX_NSTL);
	lv_obj_set_width(img_fundo, LV_HOR_RES);
	lv_obj_set_protect(img_fundo, LV_PROTECT_POS);

	//
	create_rx_rds();
	create_rx_vumeter();
	create_rx_vumeter_lr();
	create_rx_frequency();
	create_rx_status();
	create_rx_signal();
	create_rx_image();
	create_rx_clock();
	create_rx_buttons();

	// Task Update Main Screen
	Task_Principal = lv_task_create(update_main_screen_rx, 200, LV_TASK_PRIO_MID, NULL);

	lv_scr_load(Tela_Principal);
}

void update_main_screen_rx(lv_task_t * param)
{
	lv_label_set_text_fmt(ui_text_clock, "%02d:%02d", gTime.Hours, gTime.Minutes);
	lv_label_set_text_fmt(ui_text_date, "%04d-%s %02d", 2000+gDate.Year, MES[gDate.Month], gDate.Date);
	if(gTime.Hours >= 12)
		lv_label_set_text(ui_text_ampm, "PM");
	else
		lv_label_set_text(ui_text_ampm, "AM");

	update_rx_vumeter_main_lr(demo_rx_vumeter_lr);
	if(!flag_rx_vumeter_lr) {
		demo_rx_vumeter_lr++;
		if(demo_rx_vumeter_lr > 12) flag_rx_vumeter_lr = 1;
	}
	else {
		if(demo_rx_vumeter_lr >= 1) {
			demo_rx_vumeter_lr--;
		}
		else flag_rx_vumeter_lr = 0;
	}
	//
	update_rx_vumeter(demo_rx_vumeter);
	if(!flag_rx_vumeter) {
		demo_rx_vumeter++;
		if(demo_rx_vumeter > 47) flag_rx_vumeter = 1;
	}
	else {
		if(demo_rx_vumeter >= 1) {
			demo_rx_vumeter--;
		}
		else flag_rx_vumeter = 0;
	}
}

void update_rx_vumeter(uint32_t value)
{
	uint32_t x;

	if(value > 47) value = 47;

	for(x = 0; x < value; x++) {
		lv_obj_set_hidden(ui_vumeter_x1[x], false);
		lv_obj_set_hidden(ui_vumeter_x2[x], false);
	}

	for(x = value; x < 47; x++) {
		lv_obj_set_hidden(ui_vumeter_x1[x], true);
		lv_obj_set_hidden(ui_vumeter_x2[x], true);
	}
}

void create_rx_vumeter(void)
{
	static lv_style_t style_txt_vu;
	lv_style_copy(&style_txt_vu, &lv_style_plain);
	style_txt_vu.text.font = &Helvetica_12;
	style_txt_vu.text.letter_space = 1;
	style_txt_vu.text.line_space = 1;
	style_txt_vu.text.color = LV_COLOR_WHITE;

	// Texto VU MPX
	for(uint8_t x = 0; x < 6; x++) {
			ui_txt_vu[x] = lv_label_create(Tela_Principal, NULL);
			lv_obj_set_style(ui_txt_vu[x], &style_txt_vu); 						// Configura o estilo criado
			lv_label_set_long_mode(ui_txt_vu[x], LV_LABEL_LONG_EXPAND); 		// Quebra as linhas longas
			lv_label_set_recolor(ui_txt_vu[x], true); 							// Ativa recolorizar por comandos no texto
			lv_label_set_align(ui_txt_vu[x], LV_ALIGN_CENTER); 					// Centraliza linhas alinhadas
	}
	lv_label_set_text(ui_txt_vu[0], "FM DEV.");
	lv_obj_set_pos(ui_txt_vu[0], 6, 20);

	lv_label_set_text(ui_txt_vu[1], "25");
	lv_obj_set_pos(ui_txt_vu[1], 74, 20);

	lv_label_set_text(ui_txt_vu[2], "50");
	lv_obj_set_pos(ui_txt_vu[2], 120, 20);

	lv_label_set_text(ui_txt_vu[3], "75KHz");
	lv_obj_set_pos(ui_txt_vu[3], 170, 20);

	lv_label_set_text(ui_txt_vu[4], "100");
	lv_obj_set_pos(ui_txt_vu[4], 210, 20);

	lv_label_set_text(ui_txt_vu[5], "120 >");
	lv_obj_set_pos(ui_txt_vu[5], 252, 20);

	// LEDS
	for(uint8_t x = 0; x < 47; x++) {
		ui_vumeter_x1[x] = lv_img_create(Tela_Principal, NULL);
		lv_img_cache_invalidate_src(NULL);
		if(x < 30) {
			lv_img_set_src(ui_vumeter_x1[x], &LED_NSTL_AZ);
		}
		else if( x >= 30 && x < 34 ) {
			lv_img_set_src(ui_vumeter_x1[x], &LED_NSTL_VD);
		}
		else if( x >= 34 && x < 38 ) {
			lv_img_set_src(ui_vumeter_x1[x], &LED_NSTL_AM);
		}
		else if(x >= 38){
			lv_img_set_src(ui_vumeter_x1[x], &LED_NSTL_VM);
		}
			lv_obj_set_protect(ui_vumeter_x1[x], LV_PROTECT_POS);
			lv_obj_set_pos(ui_vumeter_x1[x], pos_led_vumeter_x[x], 34);
	}

	// LEDS
	for(uint8_t x = 0; x < 47; x++) {
		ui_vumeter_x2[x] = lv_img_create(Tela_Principal, NULL);
		lv_img_cache_invalidate_src(NULL);
		lv_img_set_src(ui_vumeter_x2[x], &LED_NSTL_AZ);
		lv_obj_set_protect(ui_vumeter_x2[x], LV_PROTECT_POS);
		lv_obj_set_pos(ui_vumeter_x2[x], pos_led_vumeter_x[x], 70);
	}
}

void create_rx_rds(void)
{
	static lv_style_t style_txt_rds;
	lv_style_copy(&style_txt_rds, &lv_style_plain);
	style_txt_rds.text.font = &Helvetica_12;
	style_txt_rds.text.letter_space = 1;
	style_txt_rds.text.line_space = 1;
	style_txt_rds.text.color = LV_COLOR_WHITE;

	ui_txt_rds = lv_label_create(Tela_Principal, NULL);
	lv_obj_set_style(ui_txt_rds, &style_txt_rds); 					// Configura o estilo criado
	lv_label_set_long_mode(ui_txt_rds, LV_LABEL_LONG_EXPAND); 		// Quebra as linhas longas
	lv_label_set_recolor(ui_txt_rds, true); 						// Ativa recolorizar por comandos no texto
	lv_label_set_align(ui_txt_rds, LV_ALIGN_CENTER); 				// Centraliza linhas alinhadas
	lv_label_set_text(ui_txt_rds, "RADIO JOVEM PAN PIRACICABA RDS ON");
	lv_obj_set_pos(ui_txt_rds, 6, 2);
}

void update_rx_vumeter_main_lr(uint32_t value)
{
	uint32_t x;

	if(value > 12) value = 12;

	for(x = 0; x < value; x++) {
		lv_obj_set_hidden(ui_vumeter_l[x], false);
		lv_obj_set_hidden(ui_vumeter_r[x], false);
		lv_obj_set_hidden(ui_vumeter_m1[x], false);
		lv_obj_set_hidden(ui_vumeter_m2[x], false);
	}

	for(x = value; x < 12; x++) {
		lv_obj_set_hidden(ui_vumeter_l[x], true);
		lv_obj_set_hidden(ui_vumeter_r[x], true);
		lv_obj_set_hidden(ui_vumeter_m1[x], true);
		lv_obj_set_hidden(ui_vumeter_m2[x], true);
	}
}

void create_rx_vumeter_lr(void)
{
	// LEDS
	for(uint8_t x = 0; x < 12; x++) {
		ui_vumeter_l[x] = lv_img_create(Tela_Principal, NULL);
		lv_img_cache_invalidate_src(NULL);
		lv_img_set_src(ui_vumeter_l[x], &LED_NSTL_AZ);
		lv_obj_set_protect(ui_vumeter_l[x], LV_PROTECT_POS);
		lv_obj_set_pos(ui_vumeter_l[x], pos_led_vumeter_l[x], 72);
	}
	lv_img_set_src(ui_vumeter_l[11], &LED_NSTL_VM);

	for(uint8_t x = 0; x < 12; x++) {
		ui_vumeter_r[x] = lv_img_create(Tela_Principal, NULL);
		lv_img_cache_invalidate_src(NULL);
		lv_img_set_src(ui_vumeter_r[x], &LED_NSTL_AZ);
		lv_obj_set_protect(ui_vumeter_r[x], LV_PROTECT_POS);
		lv_obj_set_pos(ui_vumeter_r[x], pos_led_vumeter_l[x], 88);
	}
	lv_img_set_src(ui_vumeter_r[11], &LED_NSTL_VM);

	for(uint8_t x = 0; x < 12; x++) {
		ui_vumeter_m1[x] = lv_img_create(Tela_Principal, NULL);
		lv_img_cache_invalidate_src(NULL);
		lv_img_set_src(ui_vumeter_m1[x], &LED_NSTL_AZ);
		lv_obj_set_protect(ui_vumeter_m1[x], LV_PROTECT_POS);
		lv_obj_set_pos(ui_vumeter_m1[x], (94+pos_led_vumeter_l[x]), 72);
	}
	lv_img_set_src(ui_vumeter_m1[11], &LED_NSTL_VM);

	for(uint8_t x = 0; x < 12; x++) {
		ui_vumeter_m2[x] = lv_img_create(Tela_Principal, NULL);
		lv_img_cache_invalidate_src(NULL);
		lv_img_set_src(ui_vumeter_m2[x], &LED_NSTL_AZ);
		lv_obj_set_protect(ui_vumeter_m2[x], LV_PROTECT_POS);
		lv_obj_set_pos(ui_vumeter_m2[x], (94+pos_led_vumeter_l[x]), 88);
	}
	lv_img_set_src(ui_vumeter_m2[11], &LED_NSTL_VM);
}

void create_rx_frequency(void)
{
	static lv_style_t style_txt_frequency;
	lv_style_copy(&style_txt_frequency, &lv_style_plain);
	style_txt_frequency.text.font = &Helvetica_12;
	style_txt_frequency.text.letter_space = 0;
	style_txt_frequency.text.line_space = 0;
	style_txt_frequency.text.color = LV_COLOR_WHITE;

	ui_text_frequency = lv_label_create(Tela_Principal, NULL);
	lv_obj_set_style(ui_text_frequency, &style_txt_frequency); 				// Configura o estilo criado
	lv_label_set_long_mode(ui_text_frequency, LV_LABEL_LONG_EXPAND); 		// Quebra as linhas longas
	lv_label_set_recolor(ui_text_frequency, true); 							// Ativa recolorizar por comandos no texto
	lv_label_set_align(ui_text_frequency, LV_ALIGN_CENTER); 				// Centraliza linhas alinhadas
	lv_label_set_text(ui_text_frequency, "FREQUENCY:    950.875MHz");
	lv_obj_set_pos(ui_text_frequency, 10, 53);
}

void create_rx_status(void)
{
	static lv_style_t style_txt_status;
	lv_style_copy(&style_txt_status, &lv_style_plain);
	style_txt_status.text.font = &Helvetica_12;
	style_txt_status.text.letter_space = 1;
	style_txt_status.text.line_space = 1;
	style_txt_status.text.color = LV_COLOR_WHITE;

	lv_style_copy(&style_btn_status_vd, &lv_style_plain_color);
	style_btn_status_vd.body.main_color = LV_COLOR_GREEN;
	style_btn_status_vd.body.grad_color = LV_COLOR_GREEN;
	style_btn_status_vd.body.radius = 2;
	style_btn_status_vd.text.color = LV_COLOR_WHITE;
	style_btn_status_vd.text.font = &Helvetica_12;
	style_btn_status_vd.text.letter_space = 1;
	style_btn_status_vd.text.line_space = 1;
	style_btn_status_vd.text.color = LV_COLOR_WHITE;

	lv_style_copy(&style_btn_status_vm, &lv_style_plain_color);
	style_btn_status_vm.body.main_color = LV_COLOR_RED;
	style_btn_status_vm.body.grad_color = LV_COLOR_RED;
	style_btn_status_vm.body.radius = 2;
	style_btn_status_vm.text.color = LV_COLOR_BLACK;
	style_btn_status_vm.text.font = &Helvetica_12;
	style_btn_status_vm.text.letter_space = 1;
	style_btn_status_vm.text.line_space = 1;

	btn_status = lv_btn_create(Tela_Principal, NULL); 					// Cria um botao na tela
	lv_obj_set_user_data(btn_status, 0);
	lv_btn_set_style(btn_status, LV_BTN_STYLE_REL, &style_btn_status_vd);
	lv_btn_set_style(btn_status, LV_BTN_STYLE_TGL_REL, &style_btn_status_vd);
	lv_btn_set_style(btn_status, LV_BTN_STYLE_TGL_PR, &style_btn_status_vm);
	lv_btn_set_style(btn_status, LV_BTN_STATE_PR, &style_btn_status_vd);
	lv_btn_set_style(btn_status, LV_BTN_STATE_REL, &style_btn_status_vd);
	lv_btn_set_style(btn_status, LV_BTN_STATE_INA, &style_btn_status_vd);
	//lv_obj_set_event_cb(btn_status, btn_nstl_event_btn);
	lv_obj_set_size(btn_status, 141, 17);
	lv_obj_set_pos(btn_status, 149, 52);

	ui_text_status = lv_label_create(Tela_Principal, NULL);
	lv_obj_set_style(ui_text_status, &style_txt_status); 				// Configura o estilo criado
	lv_label_set_long_mode(ui_text_status, LV_LABEL_LONG_EXPAND); 		// Quebra as linhas longas
	lv_label_set_recolor(ui_text_status, true); 						// Ativa recolorizar por comandos no texto
	lv_label_set_align(ui_text_status, LV_ALIGN_CENTER); 				// Centraliza linhas alinhadas
	lv_label_set_text(ui_text_status, "MAIN STATUS:");
	lv_obj_set_pos(ui_text_status, 154, 53);

	ui_status = lv_label_create(Tela_Principal, NULL);
	lv_obj_set_style(ui_status, &style_txt_status); 				// Configura o estilo criado
	lv_label_set_long_mode(ui_status, LV_LABEL_LONG_EXPAND); 		// Quebra as linhas longas
	lv_label_set_recolor(ui_status, true); 							// Ativa recolorizar por comandos no texto
	lv_label_set_align(ui_status, LV_ALIGN_CENTER); 				// Centraliza linhas alinhadas
	lv_label_set_text(ui_status, "OK");
	lv_obj_set_pos(ui_status, 268, 53);
}

void create_rx_image(void)
{
	// Imagem de Fundo
	img_rx1 = lv_img_create(Tela_Principal, NULL);
	lv_img_cache_invalidate_src(NULL);
	lv_img_set_src(img_rx1, &LINK_RX);
	lv_obj_set_pos(img_rx1, 294, 52);

	// Imagem de Fundo
	img_rx2 = lv_img_create(Tela_Principal, NULL);
	lv_img_cache_invalidate_src(NULL);
	lv_img_set_src(img_rx2, &RDS_RX);
	lv_obj_set_pos(img_rx2, 354, 54);

	// Imagem M1
	img_rx3 = lv_img_create(Tela_Principal, NULL);
	lv_img_cache_invalidate_src(NULL);
	lv_img_set_src(img_rx3, &STEREO_RX);
	lv_obj_set_pos(img_rx3, 414, 54);

	// Imagem M1
	img_rx4 = lv_img_create(Tela_Principal, NULL);
	lv_img_cache_invalidate_src(NULL);
	lv_img_set_src(img_rx4, &NUVEM_RX);
	lv_obj_set_pos(img_rx4, 242, 2);
}

void create_rx_clock(void)
{
	static lv_style_t style_txt_date;
	lv_style_copy(&style_txt_date, &lv_style_plain);
	style_txt_date.text.font = &Helvetica_12;
	style_txt_date.text.letter_space = 0;
	style_txt_date.text.line_space = 0;
	style_txt_date.text.color = LV_COLOR_WHITE;

	static lv_style_t style_txt_clock;
	lv_style_copy(&style_txt_clock, &lv_style_plain);
	style_txt_clock.text.font = &Helvetica_32;
	style_txt_clock.text.letter_space = 0;
	style_txt_clock.text.line_space = 0;
	style_txt_clock.text.color = LV_COLOR_WHITE;

	static lv_style_t style_txt_ampm;
	lv_style_copy(&style_txt_ampm, &lv_style_plain);
	style_txt_ampm.text.font = &Helvetica_16;
	style_txt_ampm.text.letter_space = 0;
	style_txt_ampm.text.line_space = 0;
	style_txt_ampm.text.color = LV_COLOR_WHITE;

	ui_text_date = lv_label_create(Tela_Principal, NULL);
	lv_obj_set_style(ui_text_date, &style_txt_date); 					// Configura o estilo criado
	lv_label_set_long_mode(ui_text_date, LV_LABEL_LONG_EXPAND); 		// Quebra as linhas longas
	lv_label_set_recolor(ui_text_date, true); 							// Ativa recolorizar por comandos no texto
	lv_label_set_align(ui_text_date, LV_ALIGN_CENTER); 					// Centraliza linhas alinhadas
	lv_label_set_text(ui_text_date, "2025 - AUGUST 14");
	lv_obj_set_pos(ui_text_date, 300, 20);

	ui_text_clock = lv_label_create(Tela_Principal, NULL);
	lv_obj_set_style(ui_text_clock, &style_txt_clock); 					// Configura o estilo criado
	lv_label_set_long_mode(ui_text_clock, LV_LABEL_LONG_EXPAND); 		// Quebra as linhas longas
	lv_label_set_recolor(ui_text_clock, true); 						// Ativa recolorizar por comandos no texto
	lv_label_set_align(ui_text_clock, LV_ALIGN_CENTER); 				// Centraliza linhas alinhadas
	lv_label_set_text(ui_text_clock, "10:28");
	lv_obj_set_pos(ui_text_clock, 402, 17);

	ui_text_ampm = lv_label_create(Tela_Principal, NULL);
	lv_obj_set_style(ui_text_ampm, &style_txt_ampm); 					// Configura o estilo criado
	lv_label_set_long_mode(ui_text_ampm, LV_LABEL_LONG_EXPAND); 		// Quebra as linhas longas
	lv_label_set_recolor(ui_text_ampm, true); 						// Ativa recolorizar por comandos no texto
	lv_label_set_align(ui_text_ampm, LV_ALIGN_CENTER); 				// Centraliza linhas alinhadas
	lv_label_set_text(ui_text_ampm, "AM");
	lv_obj_set_pos(ui_text_ampm, 355, 36);

}

void create_rx_buttons(void)
{
	// Cria 6 buttons
	lv_style_copy(&style_btn, &lv_style_plain_color);
	style_btn.body.main_color = LV_COLOR_GRAY;
	style_btn.body.grad_color = LV_COLOR_GRAY;
	style_btn.text.color = LV_COLOR_WHITE;
	style_btn.text.font = &Helvetica_12;
	style_btn.text.letter_space = 1;
	style_btn.text.line_space = 1;
	style_btn.text.color = LV_COLOR_WHITE;

	lv_style_copy(&style_btn_am, &lv_style_plain_color);
	style_btn_am.body.main_color = LV_COLOR_YELLOW;
	style_btn_am.body.grad_color = LV_COLOR_YELLOW;
	style_btn_am.text.color = LV_COLOR_BLACK;
	style_btn_am.text.font = &Helvetica_12;
	style_btn_am.text.letter_space = 1;
	style_btn_am.text.line_space = 1;

	btn_nstl[0] = lv_btn_create(Tela_Principal, NULL); 					// Cria um botao na tela
	lv_obj_set_user_data(btn_nstl[0], 0);
	lv_btn_set_style(btn_nstl[0], LV_BTN_STYLE_REL, &style_btn);
	lv_btn_set_style(btn_nstl[0], LV_BTN_STYLE_TGL_REL, &style_btn);
	lv_btn_set_style(btn_nstl[0], LV_BTN_STYLE_TGL_PR, &style_btn_am);
	lv_btn_set_style(btn_nstl[0], LV_BTN_STATE_PR, &style_btn);
	lv_btn_set_style(btn_nstl[0], LV_BTN_STATE_REL, &style_btn);
	lv_btn_set_style(btn_nstl[0], LV_BTN_STATE_INA, &style_btn);
	//lv_obj_set_event_cb(btn_nstl[0], btn_nstl_event_btn);
	lv_obj_set_size(btn_nstl[0], 115, 16);
	lv_obj_set_pos(btn_nstl[0], 7, 105);

	btn_nstl[1] = lv_btn_create(Tela_Principal, NULL); 					// Cria um botao na tela
	lv_obj_set_user_data(btn_nstl[1], 1);
	lv_btn_set_style(btn_nstl[1], LV_BTN_STYLE_REL, &style_btn);
	lv_btn_set_style(btn_nstl[1], LV_BTN_STYLE_TGL_REL, &style_btn);
	lv_btn_set_style(btn_nstl[1], LV_BTN_STYLE_TGL_PR, &style_btn_am);
	lv_btn_set_style(btn_nstl[1], LV_BTN_STATE_PR, &style_btn);
	lv_btn_set_style(btn_nstl[1], LV_BTN_STATE_REL, &style_btn);
	lv_btn_set_style(btn_nstl[1], LV_BTN_STATE_INA, &style_btn);
	//lv_obj_set_event_cb(btn_nstl[1], btn_nstl_event_btn);
	lv_obj_set_size(btn_nstl[1], 115, 16);
	lv_obj_set_pos(btn_nstl[1], 124, 105);

	btn_nstl[2] = lv_btn_create(Tela_Principal, NULL); 					// Cria um botao na tela
	lv_obj_set_user_data(btn_nstl[2], 2);
	lv_btn_set_style(btn_nstl[2], LV_BTN_STYLE_REL, &style_btn);
	lv_btn_set_style(btn_nstl[2], LV_BTN_STYLE_TGL_REL, &style_btn);
	lv_btn_set_style(btn_nstl[2], LV_BTN_STYLE_TGL_PR, &style_btn_am);
	lv_btn_set_style(btn_nstl[2], LV_BTN_STATE_PR, &style_btn);
	lv_btn_set_style(btn_nstl[2], LV_BTN_STATE_REL, &style_btn);
	lv_btn_set_style(btn_nstl[2], LV_BTN_STATE_INA, &style_btn);
	//lv_obj_set_event_cb(btn_nstl[2], btn_nstl_event_btn);
	lv_obj_set_size(btn_nstl[2], 114, 16);
	lv_obj_set_pos(btn_nstl[2], 241, 105);

	btn_nstl[3] = lv_btn_create(Tela_Principal, NULL); 					// Cria um botao na tela
	lv_obj_set_user_data(btn_nstl[3], 3);
	lv_btn_set_style(btn_nstl[3], LV_BTN_STYLE_REL, &style_btn);
	lv_btn_set_style(btn_nstl[3], LV_BTN_STYLE_TGL_REL, &style_btn);
	lv_btn_set_style(btn_nstl[3], LV_BTN_STYLE_TGL_PR, &style_btn_am);
	lv_btn_set_style(btn_nstl[3], LV_BTN_STATE_PR, &style_btn);
	lv_btn_set_style(btn_nstl[3], LV_BTN_STATE_REL, &style_btn);
	lv_btn_set_style(btn_nstl[3], LV_BTN_STATE_INA, &style_btn);
	//lv_obj_set_event_cb(btn_nstl[3], btn_nstl_event_btn);
	lv_obj_set_size(btn_nstl[3], 114, 16);
	lv_obj_set_pos(btn_nstl[3], 358, 105);

	// Adiciona Labels Buttons
	btn_nstl_label[0] = lv_label_create(btn_nstl[0], NULL); 		// Cria um rotulo nos primeiros botoes
	lv_obj_set_style(btn_nstl_label[0], &style_btn); 				// Configura o estilo criado
	lv_label_set_text(btn_nstl_label[0], "AUDIO"); 					// Atribui o texto ao rotulo

	btn_nstl_label[1] = lv_label_create(btn_nstl[1], NULL); 		// Cria um rotulo nos primeiros botoes
	lv_obj_set_style(btn_nstl_label[1], &style_btn); 				// Configura o estilo criado
	lv_label_set_text(btn_nstl_label[1], "FREQUENCY"); 				// Atribui o texto ao rotulo

	btn_nstl_label[2] = lv_label_create(btn_nstl[2], NULL); 		// Cria um rotulo nos primeiros botoes
	lv_obj_set_style(btn_nstl_label[2], &style_btn); 				// Configura o estilo criado
	lv_label_set_text(btn_nstl_label[2], "CONFIGS"); 					// Atribui o texto ao rotulo

	btn_nstl_label[3] = lv_label_create(btn_nstl[3], NULL); 		// Cria um rotulo nos primeiros botoes
	lv_obj_set_style(btn_nstl_label[3], &style_btn); 				// Configura o estilo criado
	lv_label_set_text(btn_nstl_label[3], "READINGS");				// Atribui o texto ao rotulo
}

void create_rx_signal(void)
{
	static lv_style_t style_txt_signal;
	lv_style_copy(&style_txt_signal, &lv_style_plain);
	style_txt_signal.text.font = &Helvetica_12;
	style_txt_signal.text.letter_space = 0;
	style_txt_signal.text.line_space = 0;
	style_txt_signal.text.color = LV_COLOR_WHITE;

	ui_text_signal[0] = lv_label_create(Tela_Principal, NULL);
	lv_obj_set_style(ui_text_signal[0], &style_txt_signal); 				// Configura o estilo criado
	lv_label_set_long_mode(ui_text_signal[0], LV_LABEL_LONG_EXPAND); 		// Quebra as linhas longas
	lv_label_set_recolor(ui_text_signal[0], true); 							// Ativa recolorizar por comandos no texto
	lv_label_set_align(ui_text_signal[0], LV_ALIGN_CENTER); 				// Centraliza linhas alinhadas
	lv_label_set_text(ui_text_signal[0], "RX SIGNAL LEVEL:      96.6%");
	lv_obj_set_pos(ui_text_signal[0], 10, 87);

	ui_text_signal[1] = lv_label_create(Tela_Principal, NULL);
	lv_obj_set_style(ui_text_signal[1], &style_txt_signal); 				// Configura o estilo criado
	lv_label_set_long_mode(ui_text_signal[1], LV_LABEL_LONG_EXPAND); 		// Quebra as linhas longas
	lv_label_set_recolor(ui_text_signal[1], true); 							// Ativa recolorizar por comandos no texto
	lv_label_set_align(ui_text_signal[1], LV_ALIGN_CENTER); 				// Centraliza linhas alinhadas
	lv_label_set_text(ui_text_signal[1], "-65.5dBm");
	lv_obj_set_pos(ui_text_signal[1], 160, 87);

	ui_text_signal[2] = lv_label_create(Tela_Principal, NULL);
	lv_obj_set_style(ui_text_signal[2], &style_txt_signal); 				// Configura o estilo criado
	lv_label_set_long_mode(ui_text_signal[2], LV_LABEL_LONG_EXPAND); 		// Quebra as linhas longas
	lv_label_set_recolor(ui_text_signal[2], true); 							// Ativa recolorizar por comandos no texto
	lv_label_set_align(ui_text_signal[2], LV_ALIGN_CENTER); 				// Centraliza linhas alinhadas
	lv_label_set_text(ui_text_signal[2], "145uV");
	lv_obj_set_pos(ui_text_signal[2], 240, 87);
}
