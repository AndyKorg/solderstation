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
#if defined(KEY_MODULE_STLED)
#include "stled316.h"
#endif

#include "console.h"

void solder_Off(void){
	if ((solder.state != STATE_OFF) && (!solder.delayOffOn)){
		SolderOff();
		solder.state = STATE_OFF;
		solder.delayOffOn = KEY_DELAY_ONOFF_TICK;//включить можно только через этот период
		console_print("sol of\r");
	}
}

void fan_heat_Off(void){
	if ((fan_heat.state != STATE_OFF) && (!fan_heat.delayOffOn)){
		FanHeatOff();
		fan_heat.state = STATE_OFF;
		fan_heat.delayOffOn = KEY_DELAY_ONOFF_TICK;//включить можно только через этот период
		console_print("fan of\r");
	}
}

#if defined(KEY_MODULE_SCAN)
#define ButtonIsOff(portOut, pin)		(*PIN(&portOut) & (1<<pin))
#define ButtonIsOn(portOut, pin)		(!((*PIN(&portOut)) & (1<<pin)))

ISR(SOLDER_BUTTON_INT){
	solder_Off();
}

ISR(FAN_BUTTON_INT){
	fan_heat_Off();
}
#endif

//Возврат из режима установки по прошествеии времени periodSettingS
inline void settingMode(device_t *device){
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
	settingMode(&fan_heat);
	SetTimerTask(keySettingModeOff, PERIOD_1S);
}

inline void repeatMode(device_t *device){
	if (device->periodRepeatMs){
		device->periodRepeatMs--;
	}
}

void keyboardRepeat(void){
	repeatMode(&solder);
	repeatMode(&fan);
	repeatMode(&fan_heat);
	SetTimerTask(keyboardRepeat, KEY_PERIOD_REPEAT_MS);
}

inline void keyProcess(device_t *device, u08 onOff, u08 plus, u08 minus){
	if (device->delayOffOn){			//Идет задержка после срабатывания включения или выключения
		device->delayOffOn = onOff?KEY_DELAY_ONOFF_TICK:(device->delayOffOn-1);//Если кнопка еще нажата, то увеличиваем интервал
	}
	if (onOff){
		if ((device->state == STATE_OFF) && (!device->delayOffOn)){
			device->state = STATE_ON;
			device->delayOffOn = KEY_DELAY_ONOFF_TICK;
			if (device == &solder){
				console_print("sol on\r");
			}
			if (device == &fan_heat){
				console_print("fan on\r");
			}
		}
	}
	else{
		if (plus || minus){
			device->periodSettingS = SETTING_MODE_PERIOD_S;
			if (device->state == STATE_ON){
				device->setSelect = device->need;
				device->state = STATE_SET;
				device->periodRepeatMs = KEY_DELAY_REPEAT_TICK;
				console_print("dev set on\r");
			}
			else if (device->state == STATE_SET){
				if (!device->periodRepeatMs){
					if (plus && ((device->setSelect + KEY_STEP_CHANGE) <= device->maxValue)){
						device->setSelect += KEY_STEP_CHANGE;
						console_print("dev +\r");
					}
					else if (minus && device->setSelect){
						device->setSelect -= (device->setSelect > KEY_STEP_CHANGE)?KEY_STEP_CHANGE:device->setSelect;
						console_print("dev -\r");
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

void keyboard(void){

	#if defined(KEY_MODULE_SCAN)
	keyProcess(&solder,
	ButtonIsOn(SOLDER_BUTTON_ON_OUT, SOLDER_BUTTON_ON_PIN),
	ButtonIsOn(SOLDER_BUTTON_PLUS_OUT, SOLDER_BUTTON_PLUS_PIN),
	ButtonIsOn(SOLDER_BUTTON_MINUS_OUT, SOLDER_BUTTON_MINUS_PIN)
	);
	keyProcess(&fan_heat,
	ButtonIsOn(FAN_BUTTON_ON_OUT, FAN_BUTTON_ON_PIN),
	ButtonIsOn(FAN_BUTTON_PLUS_OUT, FAN_BUTTON_PLUS_PIN),
	ButtonIsOn(FAN_BUTTON_MINUS_OUT, FAN_BUTTON_MINUS_PIN)
	);
	#elif defined(KEY_MODULE_STLED)
	stled316_keys_t keys;
	stled316_get(&keys);
	keyProcess(&solder, keys.solder.onOff, keys.solder.plus, keys.solder.minus);
	keyProcess(&fan_heat, keys.fan.onOff, keys.fan.plus, keys.fan.minus);
	#endif
	SetTimerTask(keyboard, KEY_SCAN_PERIOD_MS);
}


void keyboard_init(void){
	
	//setting device
	solder.maxValue = SOLDER_MAX;
	fan_heat.maxValue = FAN_HEAT_MAX;
	//Повторное выключение-выключение, тиков диспетчера задач
	solder.delayOffOn = KEY_DELAY_ONOFF_TICK;
	fan_heat.delayOffOn = KEY_DELAY_ONOFF_TICK;
	solder.periodRepeatMs = KEY_DELAY_REPEAT_TICK;
	fan_heat.periodRepeatMs = KEY_DELAY_REPEAT_TICK;

	#if defined(KEY_MODULE_SCAN)
	PinInputMode(SOLDER_BUTTON_ON_OUT, SOLDER_BUTTON_ON_PIN);
	PinInputMode(SOLDER_BUTTON_MINUS_OUT, SOLDER_BUTTON_MINUS_PIN);
	PinInputMode(SOLDER_BUTTON_PLUS_OUT, SOLDER_BUTTON_PLUS_PIN);
	PinInputMode(FAN_BUTTON_ON_OUT, FAN_BUTTON_ON_PIN);
	PinInputMode(FAN_BUTTON_MINUS_OUT, FAN_BUTTON_MINUS_PIN);
	PinInputMode(FAN_BUTTON_PLUS_OUT, FAN_BUTTON_PLUS_PIN);
	SolderButInteruptOn();
	FanButInteruptOn();
	#elif defined(KEY_MODULE_STLED)
	funcOff_t config;
	config.fan_scan_code = FAN_ON;
	config.fan_off = fan_heat_Off;
	config.solder_scan_code = SOLDER_ON;
	config.solder_off = solder_Off;
	stled316_init(config);
	#endif

	GerconFanInteruptOn();
	
	SetTimerTask(keyboard, KEY_SCAN_PERIOD_MS);			//Сканирование клавиатуры
	SetTimerTask(keySettingModeOff, PERIOD_1S);			//Снятие режима установки
	SetTimerTask(keyboardRepeat, KEY_PERIOD_REPEAT_MS);	//Повторять код клавиши пока нажата

}