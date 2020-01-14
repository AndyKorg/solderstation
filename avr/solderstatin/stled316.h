/*
* driver STLED316
* only keyboard
*/


#ifndef STLED316_H_
#define STLED316_H_

#include "avrlibtypes.h"

//������� ������ ���������� 0 - �� ������, 1 - ������
typedef struct{
	u08 onOff;
	u08 plus;
	u08 minus;
} stled316_key_t;

//������� ����� ����������
typedef struct{
	stled316_key_t solder;
	stled316_key_t fan;
} stled316_keys_t;

//���� ������ ���������� � �� �������
typedef void (*funcOff)(void);
typedef struct{
	u16 solder_scan_code;
	funcOff solder_off;
	u16 fan_scan_code;
	funcOff fan_off;
} funcOff_t;

void stled316_init(funcOff_t config);
void stled316_get(stled316_keys_t *keys_state);

#endif /* STLED316_H_ */