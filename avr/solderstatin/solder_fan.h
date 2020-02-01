/*
 * Контроль устройств на основе статусов
 *
 */


#ifndef SOLDER_FAN_H_
#define SOLDER_FAN_H_

#define SOLDER_MAX						300		//максимальная температура паяльника в условных еденицах
#define SOLDER_PERIOD_TICK				500		//Период управления паяльником ms

#define FAN_HEAT_MAX					450		//максимальная температура фена условных еденицах
#define FAN_PERIOD_TICK					250		//Период управления феном
#define FAN_PID_PERIOD_TICK				500		//Период управления ПИД фена
#define FAN_HEAT_TEMPR_STOP_MIN			20		//минимальная температура фена в условных еденицах после которой вентилятор выключается
#define FAN_HEAT_TEMPR_STOP_TICK		90		//период в течении которого температура фена была минимальной для приняти решения о выключении вентилятора
#define FAN_SPEED_MIN					10		//минимальное значение оборотов фена ниже которого не выключается
#define FAN_IN_STAND_PERIOD_OFF_S		30		//Период через который фен выключается в подставке

#define ADC_PERIOD_TICK					20

void device_init(void);
void adc_preset(void);							//Должно быть вызвано сразу после ресета

#endif /* SOLDER_FAN_H_ */