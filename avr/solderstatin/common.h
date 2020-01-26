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
#define PERIOD_1S						1000	//������ 1 �������
#define PinOutputMode(port, pin)		do {*DDR(&port) |= (1<<pin); port &= ~(1<<pin);} while (0)
#define PinInputMode(port, pin)			do {*DDR(&port) &= ~(1<<pin); port |= (1<<pin);} while (0)

typedef enum {
    STATE_OFF = 0,								//���������� ���������
    STATE_ON,									//��������
    STATE_SET,									//��������, ������������ ����� ������.
    STATE_FAN_PREPARE_OFF,						//fan �������� ������� ����� �����������
    STATE_NO_DEVICE,							//���������� �� ����������
} eState;

//Proportional kofficent
#define K_P     0.00 //max 1.265625
//Integral kofficent
#define K_I     0.00
//Differential coefficient
#define K_D     0.00

//������� �������
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


//�������� �������� ����������
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
    u16 need;									//������� �������� �������� ���������� ���������� ����������
    u16	current;								//������� ���������� (��� �����������) ��������
    u16 setSelect;								//�������� � ������ STATE_SET, �������������� � need ����� �������� � ����� STATE_ON
    u16 maxValue;								//����������� ��������� �������� ���������� ����������
    eState state;
    //keyboard
    u16 periodRepeatMs;							//������� ����������� �������.
    u08 periodSettingS;							//������� ������� ���������
    u08 delayOffToOn;								//������ �� �������� ��� ���������
    u16	limitADC;								//������������ �������� ����������� �������������
    settind_dev_t *setting;						//��������� ��� ����������
} device_t;

device_t solder,
         fan,
         fan_heat;

#endif /* COMMON_H_ */