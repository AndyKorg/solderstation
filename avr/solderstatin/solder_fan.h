/*
 * solder_fan.h
 *
 */ 


#ifndef SOLDER_FAN_H_
#define SOLDER_FAN_H_

#define SOLDER_MAX						800		//максимальная температура паяльника в условных еденицах
#define FAN_MAX							450		//максимальная температура фена
#define ADC_PERIOD_TICK					20
#define SOLDER_PERIOD_TICK				1000	//Период управления паяльником

void device_init(void);
void adc_preset(void);							//Должно быть вызвано сразу после ресета

#endif /* SOLDER_FAN_H_ */