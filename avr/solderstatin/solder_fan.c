/*
* solder_fan.c
*/

#include <stdio.h>
#include <avr/io.h>
#include <stdlib.h>
#include "avrlibtypes.h"
#include "common.h"
#include "EERTOS.h"
#include "HAL.h"
#include "solder_fan.h"

#define ADC_START()			do {ADCSRA = (1<<ADEN) | (1<<ADSC) | (0<<ADATE) | (1<<ADIE) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);} while (0);

ISR(ADC_vect){
	if SOLDER_IS_ADC(){
		solder.current = ADC;
	}
	else{
		fan_head.current = ADC;
	}
}

void solderControl(void){
	if (solder.state == STATE_OFF){
		SolderOff();
	}
	else if (solder.state == STATE_ON){
		if (solder.current < solder.need-2){
			SolderOn();
		}
		else if (solder.current > solder.need){
			SolderOff();
		}
	}
	SetTimerTask(solderControl, SOLDER_PERIOD_TICK);
}

void solder_init(void){
	PinOutputMode(SOLDER_PWM_PORT, SOLDER_PWM_PIN);
	SolderOff();
	solder.state = STATE_OFF;
	solder.need = 0;
	solder.current = 0;//!DEBUG!
}

void fan_init(void){
	PinOutputMode(FAN_PWM_PORT, FAN_PWM_PIN);
	FanOff();
	PinOutputMode(FAN_HEAT_PORT, FAN_HEAT_PIN);
	FanHeatOff();
	fan.state = STATE_OFF;
	fan.need = 0;
	fan.current = 0;//!DEBUG!
	fan_head.state = STATE_OFF;
	fan_head.need = 0;
}

void StartADC(void){
	ADC_START();	
	SetTimerTask(StartADC, ADC_PERIOD_TICK);
}

//Преднастройка adc
void adc_preset(void){
	SOLDER_SENSOR_ADC();
}

void device_init(){
	solder_init();
	fan_init();
	SOLDER_SENSOR_ADC();
	SetTimerTask(StartADC, ADC_PERIOD_TICK);
	SetTimerTask(solderControl, SOLDER_PERIOD_TICK);
}

