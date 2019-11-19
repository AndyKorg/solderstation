/*
 * keyboard.h
 *
 */ 

#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#define KEY_SCAN_PERIOD_MS				50		//Период опроса клавиатуры и защитный интервал от дребезга
#define KEY_PERIOD_REPEAT_MS			100		//Квант времени для отсчета автоповтора
#define KEY_DELAY_REPEAT_TICK			4		//Квантов времени для автоповтора
#define KEY_DELAY_ONOFF_TICK			10		//Квантов времени повторного включения\выключения
#define SETTING_MODE_PERIOD_S			10		//Длительность режима установки, секунд
#define KEY_STEP_CHANGE					10		//Шаг изменения величины

void keyboard_init(void);

#endif /* KEYBOARD_H_ */