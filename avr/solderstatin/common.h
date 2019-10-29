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
	STATE_SET									//��������, ������������ ����� ������
} eState;

typedef struct{
	u16 need;									//������� �������� �������� ���������� ���������� ���������� �����������
	u16	current;								//������� ���������� (��� �����������) ��������
	u16 setSelect;								//�������� � ������ STATE_SET, �������������� � need ����� �������� � ����� STATE_ON
	u16 maxValue;
	eState state;
	//display
	u08 xPosEndTempr;							//������� �� x ����� ������ �������� ������������
	//keyboard
	u16 periodRepeatMs;							//������� ����������� �������
	u08 periodSettingS;							//������� ������� ���������
	u08 delayOff;								//������ �� �������� ��� ���������
} sDevice;

sDevice solder, fan, fan_head;

#endif /* COMMON_H_ */