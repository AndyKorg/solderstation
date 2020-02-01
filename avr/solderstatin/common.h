/*
 * common.h
 *
 * Created: 13.10.2019 11:03:25
 *  Author: Administrator
 */


#ifndef COMMON_H_
#define COMMON_H_

#include <stdbool.h>
#include "avrlibdefs.h"
#include "avrlibtypes.h"

/************************************************************************/
/*                        COMMON                                        */
/************************************************************************/
#define PERIOD_1S						1000	//������ 1 �������
#define PinOutputMode(port, pin)		do {*DDR(&port) |= (1<<pin); port &= ~(1<<pin);} while (0)
#define PinInputMode(port, pin)			do {*DDR(&port) &= ~(1<<pin); port |= (1<<pin);} while (0)

typedef enum {
    STATE_NORMAL = 0,							//������� ������� �� �������, ��������� ������� �����������
    STATE_SET,									//������������ ����� ������, ��������� ���������� ��������
    STATE_FAN_PREPARE_OFF,						//������ ��� fan, ���������� ������� ����� �����������
    STATE_NO_DEVICE,							//���������� �� ����������, ��������� ������� �����������, ������� � ������ ������ ��������
} eState;

//Fan heat default
#define K_P_FAN_HEAT    0x55
#define K_I_FAN_HEAT    5
#define K_D_FAN_HEAT    5

//solder default
#define K_P_SOLDER		0xa1
#define K_I_SOLDER		0x10
#define K_D_SOLDER		0x0

//������� �������
//read PID koefficent
#define K_READ_CMD					0x52		//'R'
#define K_READ_PRM					0x44		//'D'
//Change PID koefficent
#define K_CHANGE_MASK			0x0f
#define K_DEVICE_MASK			0xf0
#define K_PID_FAN_HEAT			(K_DEVICE_MASK & 0xc0)	//Fan heat command PID
#define K_PID_SOLDER			(K_DEVICE_MASK & 0xd0)	//Solder command PID
#define K_P_CHANGE				(K_CHANGE_MASK & 1)
#define K_I_CHANGE				(K_CHANGE_MASK & 2)
#define K_D_CHANGE				(K_CHANGE_MASK & 3)
//Change fan speed
//#define CONSLOE_FAN_SPEED
#ifdef CONSLOE_FAN_SPEED
#define FAN_SPEED_START_CHANGE	0xa0
#define FAN_SPEED_STOP_CHANGE	0xa1
#endif
//resetSystem
#define RESET_CMD				0x11
#define SAVE_CMD				0x12
#define LOAD_CMD				0x13

//������������ ����� ���� ���� �������
typedef	enum {
    SOUND_SHORT,	//�������� ������
    SOUND_LONG,		//������� ������
    SOUND_FLASH_3,	//3 ����������� �������
} sound_type_t;


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
    eState state;								//����� �����������, ������ �������� ��� �����-�� �����������
    bool on;									//���������� ��������
    //keyboard
    u16 periodRepeatMs;							//������� ����������� �������.
    u08 periodSettingS;							//������� ������� ���������
    u08 delayOffToOn;							//������ �� �������� ��� ���������
    u16	limitADC;								//������������ �������� ����������� �������������
    //display
    uint8_t disp_add;							//���������� ����������� � �������� ��� ������ �� �������
    //setting
    settind_dev_t *setting;						//��������� ��� ����������
} device_t;

device_t solder,
         fan,
         fan_heat;


#endif /* COMMON_H_ */