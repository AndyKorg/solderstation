/*
 * �������� ��������� �� ������ ��������
 *
 */


#ifndef SOLDER_FAN_H_
#define SOLDER_FAN_H_

#define SOLDER_MAX						300		//������������ ����������� ��������� � �������� ��������
#define SOLDER_PERIOD_TICK				500		//������ ���������� ���������� ms

#define FAN_HEAT_MAX					450		//������������ ����������� ���� �������� ��������
#define FAN_PERIOD_TICK					250		//������ ���������� �����
#define FAN_PID_PERIOD_TICK				500		//������ ���������� ��� ����
#define FAN_HEAT_TEMPR_STOP_MIN			20		//����������� ����������� ���� � �������� �������� ����� ������� ���������� �����������
#define FAN_HEAT_TEMPR_STOP_TICK		90		//������ � ������� �������� ����������� ���� ���� ����������� ��� ������� ������� � ���������� �����������
#define FAN_SPEED_MIN					10		//����������� �������� �������� ���� ���� �������� �� �����������
#define FAN_IN_STAND_PERIOD_OFF_S		30		//������ ����� ������� ��� ����������� � ���������

#define ADC_PERIOD_TICK					20

void device_init(void);
void adc_preset(void);							//������ ���� ������� ����� ����� ������

#endif /* SOLDER_FAN_H_ */