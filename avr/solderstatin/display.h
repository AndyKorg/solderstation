/*
 * display.h
 *
 */
#ifndef DISPLAY_H_
#define DISPLAY_H_
#include "common.h"

#define DISPLAY_REFRESH_MS				250		//���������� �������
#define FLASH_PERIOD_MS					500		//������ �������
#define SHOW_NEED_PERIOD_S				3		//������ � ������� �������� ������������ ������� ����������� ����� ������������ ������ ON

void display_init(void);
void displaySetting(void);		//�������� ���������. �������� ������ ���� ��� ���������� � ������� off

#endif /* DISPLAY_H_ */