/*
 * solder_fan.h
 *
 */ 


#ifndef SOLDER_FAN_H_
#define SOLDER_FAN_H_

#define SOLDER_MAX						450		//������������ ����������� ���������
#define FAN_MAX							450		//������������ ����������� ����
#define ADC_PERIOD_TICK					20
#define SOLDER_PERIOD_TICK				10		//������ ���������� ����������

void device_init(void);
void adc_preset(void);							//������ ���� ������� ����� ����� ������

#endif /* SOLDER_FAN_H_ */