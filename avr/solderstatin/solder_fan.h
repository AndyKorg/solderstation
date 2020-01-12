/*
 * solder_fan.h
 *
 */ 


#ifndef SOLDER_FAN_H_
#define SOLDER_FAN_H_

#define SOLDER_MAX						800		//максимальная температура паяльника в условных еденицах
#define SOLDER_PERIOD_TICK				1000	//Период управления паяльником ms

#define FAN_HEAT_MAX					450		//максимальная температура фена
#define FAN_PERIOD_TICK					250		//Период управления феном
#define FAN_PID_PERIOD_TICK				1000	//Период управления ПИД фена
#define FAN_HEAT_TEMPR_STOP_MIN			50		//минимальная температура фена после которой вентилятор выключается
#define FAN_HEAT_TEMPR_STOP_TICK		60		//период в течении которого температура фена была минимальной для приняти решения о выключении вентилятора
#define FAN_SPEED_MIN					10		//минимальное значение оборотов фена ниже которого не выключается

#define ADC_PERIOD_TICK					20

void device_init(void);
void adc_preset(void);							//Должно быть вызвано сразу после ресета

#endif /* SOLDER_FAN_H_ */