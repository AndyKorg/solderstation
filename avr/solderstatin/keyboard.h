/*
 * keyboard.h
 *
 */ 

#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#define KEY_SCAN_PERIOD_MS				50		//������ ������ ���������� � �������� �������� �� ��������
#define KEY_PERIOD_REPEAT_MS			100		//����� ������� ��� ������� �����������
#define KEY_DELAY_REPEAT_TICK			4		//������� ������� ��� �����������
#define KEY_DELAY_ONOFF_TICK			10		//������� ������� ���������� ���������\����������
#define SETTING_MODE_PERIOD_S			10		//������������ ������ ���������, ������
#define KEY_STEP_CHANGE					10		//��� ��������� ��������

void keyboard_init(void);

#endif /* KEYBOARD_H_ */