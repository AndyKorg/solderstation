/*
* keyboard.c
*
*/

#include <stdio.h>
#include <avr/io.h>
#include <stdlib.h>
#include "avrlibtypes.h"
#include "common.h"
#include "EERTOS.h"
#include "HAL.h"
#include "keyboard.h"
#include "solder_fan.h"

#define ButtonIsOff(portOut, pin)		(*PIN(&portOut) & (1<<pin))
#define ButtonIsOn(portOut, pin)		(!((*PIN(&portOut)) & (1<<pin)))

//
#define ENCODER_A_BIT					0		//статус фазы A энкодера
#define ENCODER_B_BIT					1		//статус фазы B
#define ENCODER_COUNT_BIT				2		//Два бита на полный статус
#define EncoderCurrentState(phaseAport, phaseApin, phaseBport, phaseBpin)	((ButtonIsOn(phaseAport, phaseApin)?(1<<ENCODER_A_BIT):(0<<ENCODER_A_BIT)) | (ButtonIsOn(phaseBport, phaseBpin)?(1<<ENCODER_B_BIT):(0<<ENCODER_B_BIT)))
#define ENCODER_ERROR					3
const int8_t EncoderStates[] = { //check ENCODER_A_BIT & ENCODER_B_BIT !!!
	0,				//0000
	-1,				//0001
	1,				//0010
	ENCODER_ERROR,	//0011
	1,				//0100
	0,				//0101
	ENCODER_ERROR,	//0110
	-1,				//0111
	-1,				//1000
	ENCODER_ERROR,	//1001
	0,				//1010
	1,				//1011
	ENCODER_ERROR,	//1100
	1,				//1101
	-1,				//1110
	0				//1111
};

ISR(SOLDER_BUTTON_INT){
	if ((solder.state != STATE_OFF) && (!solder.delayOff)){
		SolderOff();
		solder.state = STATE_OFF;
		solder.periodRepeatMs = KEY_DELAY_REPEAT_TICK;//включить можно только через этот период
	}
}

ISR(FAN_BUTTON_INT){
	if ((fan_head.state != STATE_OFF) && (!fan_head.delayOff)){
		FanHeatOff();
		fan_head.state = STATE_OFF;
		fan_head.periodRepeatMs = KEY_DELAY_REPEAT_TICK;//включить можно только через этот период
	}
}

inline void settingMode(sDevice *device){
	if (device->periodSettingS){
		device->periodSettingS--;
		if (!device->periodSettingS){
			if (device->state == STATE_SET){
				device->need = device->setSelect;
				device->state = STATE_ON;
			}
		}
	}
}

void keySettingModeOff(void){
	settingMode(&solder);
	settingMode(&fan);
	settingMode(&fan_head);
	SetTimerTask(keySettingModeOff, PERIOD_1S);
}

inline void repeatMode(sDevice *device){
	if (device->periodRepeatMs){
		device->periodRepeatMs--;
	}
}

void keyboardRepeat(void){
	repeatMode(&solder);
	repeatMode(&fan);
	repeatMode(&fan_head);
	SetTimerTask(keyboardRepeat, KEY_PERIOD_REPEAT_MS);
}
/*
inline void keyProcess(sDevice *device, u08 onOff, u08 plus, u08 minus){
if (device->delayOff){
device->delayOff--;
}
if (onOff){
if ((device->state == STATE_OFF) && (!device->periodRepeatMs)){
device->state = STATE_ON;
device->delayOff = KEY_DELAY_REPEAT_TICK;
}
}
else{
if (plus || minus){
device->periodSettingS = SETTING_MODE_PERIOD_S;
if (device->state == STATE_ON){
device->setSelect = device->need;
device->state = STATE_SET;
device->periodRepeatMs = KEY_DELAY_REPEAT_TICK;
}
else if (device->state == STATE_SET){
if (!device->periodRepeatMs){
if (plus && ((device->setSelect + KEY_STEP_CHANGE) <= device->maxValue)){
device->setSelect += KEY_STEP_CHANGE;
}
else if (minus && device->setSelect){
device->setSelect -= (device->setSelect > KEY_STEP_CHANGE)?KEY_STEP_CHANGE:device->setSelect;
}
device->periodRepeatMs = KEY_DELAY_REPEAT_TICK;
}
}
}
if (!plus && !minus){
device->periodRepeatMs = 0;
}
}
}
*/
inline void keyProcess(sDevice *device, uint8_t onOff, uint8_t currEnc){

	if (device->delayOff){
		device->delayOff--;
	}
	if (onOff){
		if ((device->state == STATE_OFF) && (!device->periodRepeatMs)){
			device->state = STATE_ON;
			device->delayOff = KEY_DELAY_REPEAT_TICK;
		}
	}
	else{
		int8_t	currentEncoder = EncoderStates[(device->encoderState << ENCODER_COUNT_BIT) | currEnc];
		uint8_t tmp = device->encoderState;
		device->encoderState = currEnc;
		if (currentEncoder && (currentEncoder != ENCODER_ERROR)){
			fan_head.current = (tmp << ENCODER_COUNT_BIT) | currEnc;
			device->periodSettingS = SETTING_MODE_PERIOD_S;
			if (device->state == STATE_ON){
				device->setSelect = device->need;
				device->state = STATE_SET;
			}
			else if (device->state == STATE_SET){
				if ((currentEncoder == 1) && ((device->setSelect+KEY_STEP_CHANGE) <= device->maxValue)) device->setSelect += KEY_STEP_CHANGE;
				if ((currentEncoder == -1) && (device->setSelect >= KEY_STEP_CHANGE)) device->setSelect -= KEY_STEP_CHANGE;
			}
		}
	}
}

void keyboard(void){
	keyProcess(&solder,
	ButtonIsOn(SOLDER_BUTTON_ON_OUT, SOLDER_BUTTON_ON_PIN),
	EncoderCurrentState(SOLDER_BUTTON_PLUS_OUT, SOLDER_BUTTON_PLUS_PIN, SOLDER_BUTTON_MINUS_OUT, SOLDER_BUTTON_MINUS_PIN)
	);
	/*keyProcess(&fan_head,
	ButtonIsOn(FAN_BUTTON_ON_OUT, FAN_BUTTON_ON_PIN),
	ButtonIsOn(FAN_BUTTON_PLUS_OUT, FAN_BUTTON_PLUS_PIN),
	ButtonIsOn(FAN_BUTTON_MINUS_OUT, FAN_BUTTON_MINUS_PIN)
	);*/
	SetTimerTask(keyboard, 10/*KEY_SCAN_PERIOD_MS*/);
}


void keyboard_init(void){
	
	PinInputMode(SOLDER_BUTTON_ON_OUT, SOLDER_BUTTON_ON_PIN);
	PinInputMode(SOLDER_BUTTON_MINUS_OUT, SOLDER_BUTTON_MINUS_PIN);
	PinInputMode(SOLDER_BUTTON_PLUS_OUT, SOLDER_BUTTON_PLUS_PIN);
	PinInputMode(FAN_BUTTON_ON_OUT, FAN_BUTTON_ON_PIN);
	PinInputMode(FAN_BUTTON_MINUS_OUT, FAN_BUTTON_MINUS_PIN);
	PinInputMode(FAN_BUTTON_PLUS_OUT, FAN_BUTTON_PLUS_PIN);
	
	solder.maxValue = SOLDER_MAX;
	//Начальное значение ног энкодера паяльника
	solder.encoderState = EncoderCurrentState(SOLDER_BUTTON_PLUS_OUT, SOLDER_BUTTON_PLUS_PIN, SOLDER_BUTTON_MINUS_OUT, SOLDER_BUTTON_MINUS_PIN);
	fan_head.maxValue = FAN_MAX;

	SolderButInteruptOn();
	FanButInteruptOn();
	GerconFanInteruptOn();
	
	SetTimerTask(keyboard, KEY_SCAN_PERIOD_MS);
	SetTimerTask(keySettingModeOff, PERIOD_1S);
	SetTimerTask(keyboardRepeat, KEY_PERIOD_REPEAT_MS);
}