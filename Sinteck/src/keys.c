/*
 * keys.c
 *
 *  Created on: 26 de jun de 2019
 *      Author: Rinaldo Dos Santos
 *      Sinteck Next
 */
#include "main.h"
#include "../Sinteck/src/keys.h"
#include "../Sinteck/src/buzzer.h"
#include "../Sinteck/src/delay.h"
#include "../Sinteck/GUI/GUI.h"
#include "../lvgl/lvgl.h"
//
#define ADVANCE_QPTR(x)     ((x+1) % EVT_QDEPTH)

static struct
{
	uint8_t buff[EVT_QDEPTH][EVT_QWIDTH];
	uint8_t head;
	uint8_t tail;
} evt_queue;


// Estrututa Botoes
pushbtn_param btenter;
pushbtn bt[6];

extern long int frequencia;
extern uint8_t mp3sts[];
extern uint64_t falha; 
extern uint32_t TelaAtiva, timer_alarm_lvgl;
extern uint8_t AudioCfg;

// Inicia base de Dados Leitura de Teclado
void KeyboardInit(uint8_t mask)
{
	int i ;
	uint8_t x;

	for(x = 0; x < 6; x++) {
		// clear data
		bt[x]->old_state = bt[x]->new_state = 0;
		bt[x]->mask = mask;
		bt[x]->mode = 0;
		bt[x]->flag = 0;

		// clear log
		for(i = 0; i < 8; i++) {
			ClearLog(x, i);
		}
	}
}

/** Clear the duration parameter and the click count parameter of the button.
 */
void ClearLog(uint8_t key, uint8_t index)
{
	if(index < 8)
	{
		bt[key]->click[index] = bt[key]->duration[index] = 0;
	}
}

void KeyboardSetMode(uint8_t key, uint8_t mode, bool flag)
{
	int i;

	// clear data
	bt[key]->old_state = bt[key]->new_state = 0;
	// this looks weird but correct
	bt[key]->flag = !flag;

	// clear log
	for(i = 0; i < 8; i++)
	{
		ClearLog(key, i);
	}

	// change mode
	bt[key]->mode = mode;
}

/** Clear the duration parameter and the click count parameter of the button.
 */
void Enter_ClearLog(uint8_t index)
{
	if(index < 8)
	{
		bt[2]->click[index] = bt[2]->duration[index] = 0;
	}
}

void Enter_SetMode(uint8_t mode, bool flag)
{
	int i;

	// clear data
	bt[2]->old_state = bt[2]->new_state = 0;
	// this looks weird but correct
	bt[2]->flag = !flag;

	// clear log
	for(i = 0; i < 8; i++)
	{
		Enter_ClearLog(i);
	}

	// change mode
	bt[2]->mode = mode;
}

void KeyEnter_Routine(uint8_t key)
{
	int i;
	uint8_t diff_state;
	//uint8_t evento;

	bt[key]->new_state = PushButton_Read(key);

	// difference in the button state
	diff_state = bt[key]->old_state ^ bt[key]->new_state;

	i = 0;
	// up-down mode
	if(((bt[key]->mode >> i) & 0x01) == PUSHBTN_MODE_UDOWN) {
		// the button pressed
		if(((bt[key]->new_state >> i) & 0x01) == 0x01) {
			//logI("I - KEY Evento: State Down [0]: %d [1]: %d [2]: %d \n\r", EVT_PBTN_INPUT, (uint8_t)(i+1), PBTN_DOWN);
		}
		// button released
		else {
			// actually it has just risen
			if(((bt[key]->old_state >> i) & 0x01) == 0x01) {
				Enter_SetMode(PUSHBTN_MODE_CLICK, true);
				//logI("I - KEY Evento: Change Up [0]: %d [1]: %d [2]: %d \n\r", EVT_PBTN_INPUT, (uint8_t)(i+1), PBTN_ENDN);
			}
		}
	}
	// click mode
	else {
		// the button state changed
		if((diff_state >> i) & 0x01) {
			// (re)start duration count
			bt[key]->duration[i] = 1;

			// the button released
			if(((bt[key]->new_state >> i) & 0x01) == 0x00) {
				if(bt[key]->flag) {
					bt[key]->flag = false;
					bt[key]->duration[i] = 0;
				}
				else {
					// increase click count
					bt[key]->click[i]++;
				}
			}
		}
		// button state not changed
		else {
			// increase duration count
			if((bt[key]->duration[i] > 0) && (bt[key]->duration[i] < PUSHBTN_TO_MAX)) {
				bt[key]->duration[i]++;
			}
		}
		// triple click
		if(bt[key]->click[i] >= 3) {
			// triple click event
			//logI("I - KEY Evento: Triple Click [0]: %d [1]: %d [2]: %d \n\r", EVT_PBTN_INPUT, (uint8_t)(i+1), PBTN_TCLK);
			// clear log
			//Enter_ClearLog(i);
			bt[key]->click[i] = 0;
			bt[key]->duration[i] = 0;
			//ClearLog(key, i);
			Enter_SetMode(PUSHBTN_MODE_UDOWN, true);
		}
		// button relased and short timeout passed
		else if((bt[key]->duration[i] > PUSHBTN_TO_SHORT) &&	(((bt[key]->new_state >> i) & 0x01) == 0x00)) {
			// double click
			if(bt[key]->click[i] == 2) {
				// double click event
				//evento = PBTN_DCLK;
				//logI("I - KEY Evento: Double Click [0]: %d [1]: %d [2]: %d \n\r", EVT_PBTN_INPUT, (uint8_t)(i+1), evento);
			}
			// single click
			else {
				// single click event
				//evento = PBTN_SCLK;
				//logI("I - KEY Evento: Single Click [0]: %d [1]: %d [2]: %d \n\r", EVT_PBTN_INPUT, (uint8_t)(i+1), evento);
			}
			// clear log
			//Enter_ClearLog(i);
			bt[key]->click[i] = 0;
			bt[key]->duration[i] = 0;
			//ClearLog(key, i);
		}
		// button pressed and long timeout passed
		else if((bt[key]->duration[i] > PUSHBTN_TO_LONG) && (((bt[key]->new_state >> i) & 0x01) == 0x01)) {
			// long click event
			//logI("I - KEY Evento: Long Click [0]: %d [1]: %d [2]: %d \n\r", EVT_PBTN_INPUT, (uint8_t)(i+1), PBTN_LCLK);
			// clear log
			//Enter_ClearLog(i);
			bt[key]->click[i] = 0;
			bt[key]->duration[i] = 0;
			//ClearLog(key, i);
			// raise flag: this will prevent false detect after long click
			bt[key]->flag = true;
		}
	}
	// update pin state
	bt[key]->old_state = bt[key]->new_state;
}

void KeyboardRead(uint8_t key)
{
	int i;
	uint8_t diff_state;
	uint8_t event[EVT_QWIDTH];

	bt[key]->new_state = PushButton_Read(key);

	// difference in the button state
	diff_state = bt[key]->old_state ^ bt[key]->new_state;

	i = 0;
	// up-down mode
	if(((bt[key]->mode >> i) & 0x01)  == PUSHBTN_MODE_UDOWN) {
		// the button pressed
		if(((bt[key]->new_state >> i) & 0x01) == 0x01) {
			event[0] = EVT_PBTN_INPUT;
			event[1] = (uint8_t)key;
			event[2] = PBTN_DOWN;
			Evt_EnQueue(event);
			//buzzer(3);
		}
		// button released
		else {
			// actually it has just risen
			if(((bt[key]->old_state >> i) & 0x01) == 0x01) {
				KeyboardSetMode(key, PUSHBTN_MODE_CLICK, true);
				event[0] = EVT_PBTN_INPUT;
				event[1] = (uint8_t)key;
				event[2] = PBTN_ENDN;
				Evt_EnQueue(event);
				//buzzer(3);
			}
		}
	}
	// click mode
	else {
		// the button state changed
		if((diff_state >> i) & 0x01) {
			// (re)start duration count
			bt[key]->duration[i] = 1;

			// the button released
			if(((bt[key]->new_state >> i) & 0x01) == 0x00) {
				if(bt[key]->flag) {
					bt[key]->flag = false;
					bt[key]->duration[i] = 0;
				}
				else {
					// increase click count
					bt[key]->click[i]++;
				}
			}
		}
		// button state not changed
		else {
			// increase duration count
			if((bt[key]->duration[i] > 0) && (bt[key]->duration[i] < PUSHBTN_TO_MAX)) {
				bt[key]->duration[i]++;
			}
		}
		// triple click
		if(bt[key]->click[i] >= 3) {
			// triple click event
			event[0] = EVT_PBTN_INPUT;
			event[1] = (uint8_t)key;
			event[2] = PBTN_TCLK;
			Evt_EnQueue(event);
			//buzzer(3);
			bt[key]->click[i] = 0;
			bt[key]->duration[i] = 0;
		}
		// button relased and short timeout passed
		else if((bt[key]->duration[i] > PUSHBTN_TO_SHORT) &&	(((bt[key]->new_state >> i) & 0x01) == 0x00)) {
			// double click
			if(bt[key]->click[i] == 2) {
				// double click event
				event[0] = EVT_PBTN_INPUT;
				event[1] = (uint8_t)key;
				event[2] = PBTN_DCLK;
				Evt_EnQueue(event);
				//buzzer(3);
			}
			// single click
			else {
				// single click event
				event[0] = EVT_PBTN_INPUT;
				event[1] = (uint8_t)key;
				event[2] = PBTN_SCLK;
				Evt_EnQueue(event);
				//buzzer(3);
			}
			// clear log,3699
			bt[key]->click[i] = 0;
			bt[key]->duration[i] = 0;
		}
		// button pressed and long timeout passed
		else if((bt[key]->duration[i] > PUSHBTN_TO_LONG) && (((bt[key]->new_state >> i) & 0x01) == 0x01)) {
			// long click event
			event[0] = EVT_PBTN_INPUT;
			event[1] = (uint8_t)key;
			event[2] = PBTN_LCLK;
			Evt_EnQueue(event);
			//buzzer(3);
			bt[key]->click[i] = 0;
			bt[key]->duration[i] = 0;
			// raise flag: this will prevent false detect after long click
			bt[key]->flag = true;
		}
	}
	// update pin state
	bt[key]->old_state = bt[key]->new_state;
}


uint8_t PushButton_Read(uint8_t key)
{
	uint8_t ret = 0x00;

	switch(key) {
		case 0:			// Down
			ret = !(HAL_GPIO_ReadPin(KEY_DOWN_GPIO_Port, KEY_DOWN_Pin));
			break;
		case 1:			// UP
			ret = !(HAL_GPIO_ReadPin(KEY_UP_GPIO_Port, KEY_UP_Pin));
			break;
		case 2:			// Enter
			ret = !(HAL_GPIO_ReadPin(KEY_ENTER_GPIO_Port, KEY_ENTER_Pin));
			break;
		case 3:			// Esc
			ret = !(HAL_GPIO_ReadPin(KEY_ESC_GPIO_Port, KEY_ESC_Pin));
			break;
		default:
			break;
	}
	return ret;
}

void Key_Read(void)
{
	uint8_t x;
	for(x = 0; x < 4; x++) {
		KeyboardRead(x);
	}
}

/**
 * Append a new event at the end of the queue. If the queue is full, then
 * the event is ignored and it returns with false.
 *
 * \param  event data in an array of uint8_t
 * \return false if the queue is full
 */
bool Evt_EnQueue(uint8_t *event)
{
	unsigned i;
	uint8_t next = ADVANCE_QPTR(evt_queue.head);

	// queue is full
	if(next == evt_queue.tail)
	{
		// event will be lost
		return false;
	}

	// copy event bytes into the buffer
	for(i = 0; i < EVT_QWIDTH; i++)
	{
		evt_queue.buff[evt_queue.head][i] = event[i];
	}
	// move to the next positition
	evt_queue.head = next;

	return true;
}

/**
 * Retrieve the oldest event from the queue. If the return value is false
 * the retrieved event data should be ignored. Note that the access of the
 * queue is protected by HAL_SuspendTick / Hal_ResumeTick. If any other
 * interrupt service routine were to access the queue through Evt_EnQueue,
 * corresponding interrupt should be suspended here.
 *
 * \param  event data in an array of uint8_t
 * \return false if the queue is empty
 */
bool Evt_DeQueue(uint8_t *event)
{
	uint8_t i;
	bool flag = false;

	// queue is not empty
	if(evt_queue.tail != evt_queue.head)
	{
		// copy event bytes into the buffer
		for(i = 0; i < EVT_QWIDTH; i++)
		{
			event[i] = evt_queue.buff[evt_queue.tail][i];
		}
		// move to the next position
		evt_queue.tail = ADVANCE_QPTR(evt_queue.tail);
		// set flag
		flag = true;
	}

	// return with the flag
	return flag;
}

/**
 * The tail and the head pointers are set to zero. This will invalidate all
 * the data in the queue.
 */
void Evt_InitQueue(void)
{
	// clear queue by resetting the pointers
	evt_queue.head = evt_queue.tail = 0;
}

// Eventos Teclado
void KeyboardEvent(void)
{
	uint8_t event[EVT_QWIDTH];
	//const char* teclas[] = {"KEY-DOWN", "KEY-UP", "KEY-ENTER", "KEY-ESC", "KEY-ROTARY", "KEY-USER"};

	// check event queue
	if(Evt_DeQueue(event)) {
		switch(event[0]) {
			// pushbutton event ================================================
			// event[1]: button id
			// event[2]: PBTN_SCLK, _DCLK, _TCLK, _LCLK, _DOWN, _ENDN
			case EVT_PBTN_INPUT:
				if(event[2] == PBTN_SCLK) {
					if(event[1] == 0) {
					}
					else if(event[1] == 1) {
					}
				}
				else if(event[2] == PBTN_LCLK) {
				}
				else if(event[2] == PBTN_DCLK) {
				}
				else if(event[2] == PBTN_TCLK) {
					//PushButton_SetMode(PUSHBTN_MODE_UDOWN, true);
					//logI("\r\n --> Switch to up-down mode.");
				}
				else if(event[2] == PBTN_DOWN) {
				}
				else if(event[2] == PBTN_ENDN) {
					//PushButton_SetMode(PUSHBTN_MODE_CLICK, true);
					//logI("\r\n --> Switch to click mode.");
				}
				break;
		}
	}
}

void ButtonEvent(void)
{
	uint8_t event[EVT_QWIDTH];
	// check event queue
	if(Evt_DeQueue(event)) {
		timer_alarm_lvgl = HAL_GetTick();

		switch(TelaAtiva) {
			case TelaPrincipal:
				EventTelaPrincipal(event[0], event[2], event[1]);
				break;
/*			case TelaPrincipal_2:
				EventTelaPrincipal_2(event[0], event[2], event[1]);
				break;
			case TelaAudio:
				EventTelaAudio(event[0], event[2], event[1]);
				break;
			case TelaAudioConfig:
				EventTelaAudioConfig(event[0], event[2], event[1]);
				break;
			case TelaAudioVolume:
				EventTelaAudioVolume(event[0], event[2], event[1]);
				break;
			case TelaPreset:
				EventTelaPreset(event[0], event[2], event[1]);
				break;
			case TelaPresetEdit:
				EventTelaPresetEdit(event[0], event[2], event[1]);
				break;
			case TelaPresetName:
				EventTelaPresetName(event[0], event[2], event[1]);
				break;
			case TelaPresetForward:
				EventTelaPresetForward(event[0], event[2], event[1]);
				break;
			case TelaPresetSafe:
				EventTelaPresetSafe(event[0], event[2], event[1]);
				break;
			case TelaPresetReduc:
				EventTelaPresetReduc(event[0], event[2], event[1]);
				break;
			case TelaPresetTime1:
				EventTelaPresetTime1(event[0], event[2], event[1]);
				break;
			case TelaPresetTime2:
				EventTelaPresetTime2(event[0], event[2], event[1]);
				break;
			case TelaPresetWeek:
				EventTelaPresetWeek(event[0], event[2], event[1]);
				break;
			case TelaRds:
				EventTelaRds(event[0], event[2], event[1]);
				break;
			case TelaMp3:
				EventTelaMp3(event[0], event[2], event[1]);
				break;
			case TelaConfig:
				EventTelaConfig(event[0], event[2], event[1]);
				break;
			case TelaReadings:
				EventTelaReadings(event[0], event[2], event[1]);
				break;
			case TelaTxONOFF:
				EventTelaTxONOFF(event[0], event[2], event[1]);
				break;
			case TelaFrequency:
				EventTelaFrequency(event[0], event[2], event[1]);
				break;
			case TelaPowerLock:
				EventTelaPowerLock(event[0], event[2], event[1]);
				break;
			case TelaAdvancedSettings:
				EventTelaAdvancedSettings(event[0], event[2], event[1]);
				break;
			case TelaAC_Input:
				EventTelaSetup_ACIn(event[0], event[2], event[1]);
				break;
			case TelaSoftwareOptions:
				EventTelaSoftwareOptions(event[0], event[2], event[1]);
				break;
			case TelaConfigHold:
				EventTelaConfigHold(event[0], event[2], event[1]);
				break;
			case TelaReadingForward:
				EventTelaReadingForward(event[0], event[2], event[1]);
				break;
			case TelaReadingForward_2:
				EventTelaReadingForward_2(event[0], event[2], event[1]);
				break;
			case TelaReadingForward_3:
				EventTelaReadingForward_3(event[0], event[2], event[1]);
				break;
			case TelaReadingForward_4:
				EventTelaReadingForward_4(event[0], event[2], event[1]);
				break;
			case TelaReadingForward_5:
				EventTelaReadingForward_5(event[0], event[2], event[1]);
				break;
			case TelaReadingVpaIpa:
				EventTelaReadingVpaIpa(event[0], event[2], event[1]);
				break;
			case TelaReadingTemperature:
				EventTelaReadingTemperature(event[0], event[2], event[1]);
				break;
			case TelaReadingTemperature_2:
				EventTelaReadingTemperature_2(event[0], event[2], event[1]);
				break;
			case TelaReadingTemperature_3:
				EventTelaReadingTemperature_3(event[0], event[2], event[1]);
				break;
			case TelaReadingTemperature_4:
				EventTelaReadingTemperature_4(event[0], event[2], event[1]);
				break;
			case TelaReadingRFDriver:
				EventTelaReadingRFDriver(event[0], event[2], event[1]);
				break;
			case TelaReadingACIN:
				EventTelaReadingACin(event[0], event[2], event[1]);
				break;
			case TelaReadingACIN_2:
				EventTelaReadingACin_2(event[0], event[2], event[1]);
				break;
			case TelaReadingACIN_3:
				EventTelaReadingACin_3(event[0], event[2], event[1]);
				break;
			case TelaReadingFailList:
				EventTelaReadingFailList(event[0], event[2], event[1]);
				break;
			case TelaSendingAlert:
				EventSendingAlert(event[0], event[2], event[1]);
				break;
			case TelaLicense:
				EventLicense(event[0], event[2], event[1]);
				break;
			case TelaNetwork:
				EventNetwork(event[0], event[2], event[1]);
				break;
			case TelaPassword:
				EventPassword(event[0], event[2], event[1]);
				break;
			case TelaClock:
				EventClock(event[0], event[2], event[1]);
				break;
			case TelaSoftwareUpdate:
				EventSoftwareUpdate(event[0], event[2], event[1]);
				break;
			case TelaToken:
				EventTelaToken(event[0], event[2], event[1]);
				break;
			case TelaSerialNumber:
				EventSerialNumber(event[0], event[2], event[1]);
				break;
			case TelaLoginFreq:
				EventLoginFreq(event[0], event[2], event[1]);
				break;
			case TelaLoginPowerLock:
				EventLoginPowerLock(event[0], event[2], event[1]);
				break;
			case TelaLoginAdvSett:
				EventLoginAdvSet(event[0], event[2], event[1]);
				break;
			case TelaLoginSoftOptions:
				EventLoginOptions(event[0], event[2], event[1]);
				break;
			case TelaLoginFactorySett:
				EventLoginHold(event[0], event[2], event[1]);
				break;
			case TelaBateria:
				EventTelaBattery(event[0], event[2], event[1]);
				break;
			case TelaPassword2:
				EventPassword2(event[0], event[2], event[1]);
				break;
			case TelaBateria2:
				EventTelaBattery2(event[0], event[2], event[1]);
				break;
			case TelaBateria3:
				EventTelaBattery3(event[0], event[2], event[1]);
				break;
*/
		}
	}
}

/*
void ButtonEvent(void)
{
	uint8_t event[EVT_QWIDTH];
	//uint32_t var;
	//uint16_t ad;

	// check event queue
	if(Evt_DeQueue(event)) {
		if(event[0] == EVT_PBTN_INPUT) {
			//buzzer(3);
			switch(event[2]) {
				case PBTN_SCLK:
					switch(event[1]) {
						case KEY_DN:
							AudioCfg++;
							if(AudioCfg > 4) AudioCfg = 0;
							EnableAudioChannel(AudioCfg);
							break;
						case KEY_UP:
							frequencia += 10;
							if(frequencia > MAX_FREQ) frequencia = MIN_FREQ;
							mb1501(frequencia);
							break;
						case KEY_ENTER:
							logI("Debug Falha - %X\n", falha);
							break;
						case KEY_ESC:
							Mount_FATFS();
							//logI("")
							break;
					}
					break;
				case PBTN_LCLK:
					break;
				case PBTN_DCLK:
					break;
				case PBTN_TCLK:
					break;
				case PBTN_DOWN:
					break;
				case PBTN_ENDN:
					break;
			}
		}
	}
}
*/
