/*
 * �������� �������� � EEPROM
 * ������������ ���� ������ eeprom ��� ������������ ������������ ������� flash-�����. �� ���� �� ������� ��� ������ �����, �� ���������.
 */


#ifndef EEPROM_H_
#define EEPROM_H_

#include <stdint.h>

typedef enum {
    EEP_BAD = 0,										//� eeprom ��� ��������� ������
    EEP_READY = 1,										//������ ���������
} epState_t;


typedef void (*epEndOp)(epState_t result);				//callback function �� ��������� ��������

epState_t EepromInit(size_t sizePacket);				//������������� ���������� � ��������� EEPROM. sizePacket - ������ ��������� ����������� � eeprom
epState_t EepromStartWrite(uint8_t *value, epEndOp cb_endOp);	//������ ������ value � EEPROM. ������ ��������� ������ ���������� � ����������!
epState_t EepromRead(uint8_t *value);					//��������� ������ � value

#endif /* EEPROM_H_ */