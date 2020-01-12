/*
 * common.h
 *
 * Created: 13.10.2019 11:03:25
 *  Author: Administrator
 */ 


#ifndef COMMON_H_
#define COMMON_H_

#include "avrlibdefs.h"
#include "avrlibtypes.h"

/************************************************************************/
/*                        COMMON                                        */
/************************************************************************/
#define PERIOD_1S						1000	//Период 1 секунда
#define PinOutputMode(port, pin)		do {*DDR(&port) |= (1<<pin); port &= ~(1<<pin);} while (0)
#define PinInputMode(port, pin)			do {*DDR(&port) &= ~(1<<pin); port |= (1<<pin);} while (0)

typedef enum {
	STATE_OFF = 0,								//устройство выключено
	STATE_ON,									//включено
	STATE_SET									//включено, производится выбор режима
} eState;

typedef struct{
	u16 need;									//Целевое значение которого необходимо достигнуть регулятору
	u16	current;								//Текущее измеренное (или вычисленное) значение
	u16 setSelect;								//Значение в режиме STATE_SET, переписывается в need после перехода в режим STATE_ON
	u16 maxValue;								//Максимально возможное значение набираемое оператором
	eState state;
	//keyboard
	u16 periodRepeatMs;							//счетчик автоповтора нажатия. 
	u08 periodSettingS;							//счетчик периода установки
	u08 delayOffOn;								//Защита от дребезга при включении
	u16	limitADC;								//Максимальное значение выключающее регулирование
} device_t;

device_t solder, 
		fan,
		fan_heat;

#endif /* COMMON_H_ */