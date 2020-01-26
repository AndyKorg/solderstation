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
    STATE_SET,									//включено, производится выбор режима.
    STATE_FAN_PREPARE_OFF,						//fan остужает спираль перед выключением
    STATE_NO_DEVICE,							//устройство не подключено
} eState;

//Proportional kofficent
#define K_P     0.00 //max 1.265625
//Integral kofficent
#define K_I     0.00
//Differential coefficient
#define K_D     0.00

//Команды консоли
//Change PID koefficent
#define K_CHANGE_MASK			0x0f
#define K_DEVICE_MASK			0xf0
#define K_P_CHANGE				(K_CHANGE_MASK & 1)
#define K_I_CHANGE				(K_CHANGE_MASK & 2)
#define K_D_CHANGE				(K_CHANGE_MASK & 3)
#define K_PID_SOLDER			(K_DEVICE_MASK & 0xd0)	//Solder command PID
#define K_PID_FAN_HEAT			(K_DEVICE_MASK & 0xc0)	//Fan heat command PID
//Change fan speed
#define FAN_SPEED_START_CHANGE	0xa0
#define FAN_SPEED_STOP_CHANGE	0xa1
//reserSystem
#define RESET_CMD				0x11
#define SAVE_CMD				0x12
#define LOAD_CMD				0x13


//Хранение настроек устройства
typedef struct  {
    uint16_t value;
    int16_t P_Factor;
    int16_t I_Factor;
    int16_t D_Factor;
} settind_dev_t ;

typedef struct {
    settind_dev_t solder;
    settind_dev_t fan_heat;
    settind_dev_t fan;
} setting_t;

typedef struct {
    u16 need;									//Целевое значение которого необходимо достигнуть регулятору
    u16	current;								//Текущее измеренное (или вычисленное) значение
    u16 setSelect;								//Значение в режиме STATE_SET, переписывается в need после перехода в режим STATE_ON
    u16 maxValue;								//Максимально возможное значение набираемое оператором
    eState state;
    //keyboard
    u16 periodRepeatMs;							//счетчик автоповтора нажатия.
    u08 periodSettingS;							//счетчик периода установки
    u08 delayOffToOn;								//Защита от дребезга при включении
    u16	limitADC;								//Максимальное значение выключающее регулирование
    settind_dev_t *setting;						//настройки для устройства
} device_t;

device_t solder,
         fan,
         fan_heat;

#endif /* COMMON_H_ */