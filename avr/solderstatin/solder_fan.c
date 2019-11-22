/*
* solder_fan.c
*/

#include <stdio.h>
#include <avr/io.h>
#include <stdlib.h>
#include <stdbool.h>
#include "avrlibtypes.h"
#include "common.h"
#include "EERTOS.h"
#include "HAL.h"
#include "FIFO.h"
#include "solder_fan.h"
#include "pid.h"
#include "console.h"

#define ADC_START()			do {ADCSRA = (1<<ADEN) | (1<<ADSC) | (0<<ADATE) | (1<<ADIE) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);} while (0)

//Proportional kofficent
#define K_P_CHANGE	0xdd
#define K_P     0.00 //max 1.265625
//Integral kofficent
#define K_I_CHANGE	0xde
#define K_I     0.00
//Differential coefficient
#define K_D_CHANGE	0xdf
#define K_D     0.00

struct PID_DATA solderPID, fan_head_PID;

ISR(ADC_vect){
	static uint8_t isSolder = 0;
	if (isSolder) {
		fan_head.current = ADC;
		SOLDER_SENSOR_ADC();
	}
	else {
		solder.current = ADC;
		FAN_HEAT_SENSOR_ADC();
	}
	isSolder ^= 0xff;
}

void solderControl(void){
	int16_t pid=0;
	if (solder.state == STATE_OFF){
		SolderOff();
	}
	else if ((solder.state == STATE_ON) || (solder.state == STATE_SET)){
		pid = pid_Controller(solder.need, solder.current, &solderPID);
		if (pid <0 ) pid = 0;
		//if (pid > 0x3ff) pid = 0x3ff;//10 bit
		if (pid > 0xff) pid = 0xff;//8 bit
		SOLDER_PWM_OCR = pid;
		/*		char tmp[UART_BUF_SIZE];
		snprintf(tmp, UART_BUF_SIZE, "a=%d p=%d kp=%d\r", solder.current, pid, solderPID.P_Factor);
		console_print(tmp);*/
		if (solder.current > solder.limitADC){
			SolderOff();
		}
		else{
			SolderOn();
		}
	}
	SetTimerTask(solderControl, SOLDER_PERIOD_TICK);
}

void solder_init(void){
	PinOutputMode(SOLDER_PWM_PORT, SOLDER_PWM_PIN); //with PWM_INIT
	SolderOff();
	solder.state = STATE_OFF;
	solder.need = 0;
	solder.limitADC = 800;//over 450 C//!DEBUG!
	solder.current = 0;
}

void fan_init(void){
	PinOutputMode(FAN_PWM_PORT, FAN_PWM_PIN);
	FanOff();
	//	PinOutputMode(FAN_HEAT_PORT, FAN_HEAT_PIN); //with PWM_INIT
	//	FanHeatOff();
	fan.state = STATE_OFF;
	fan.need = 0;
	fan.current = 0;
	fan_head.state = STATE_OFF;
	fan_head.need = 0;
	fan_head.limitADC = 550;//over 450 C//!DEBUG!
}

void StartADC(void){
	if (!(ADCSRA & (1<<ADSC))){
		ADC_START();
	}
	SetTimerTask(StartADC, ADC_PERIOD_TICK);
}

//Преднастройка adc
void adc_preset(void){
	SOLDER_SENSOR_ADC();
}

uint8_t pidChangeSolder(uint8_t cmd, uint16_t value){
	bool change = false;
	switch (cmd){
		case K_P_CHANGE:
		pid_Init((uint8_t)value, solderPID.I_Factor, solderPID.D_Factor, &solderPID);
		change = true;
		break;
		case K_I_CHANGE:
		pid_Init(solderPID.P_Factor, (uint8_t) value, solderPID.D_Factor, &solderPID);
		change = true;
		break;
		case K_D_CHANGE:
		pid_Init(solderPID.P_Factor, solderPID.I_Factor, (uint8_t) value, &solderPID);
		change = true;
		break;
		default:
		break;
	}
	if (change){
		pid_Reset_Integrator(&solderPID);
		char tmp[UART_BUF_SIZE];
		snprintf(tmp, UART_BUF_SIZE, "c %02x new k=%04x max=%04x\r", cmd, value, solderPID.maxError);
		console_print(tmp);
	}
	return 0;
}

void device_init(){
	TCCR1A = (SOLDER_PWM_OCR_INIT | FAN_HEAT_PWM_OCR_INIT | (0<<FOC1A) | (0<<FOC1B) | (0<<WGM11) | (1<<WGM10));	//Fast PWM, 8-bit
	TCCR1B = ((0<<ICNC1) | (0<<ICES1) | (0<<WGM13) | (0<<WGM12) | (0<<CS12) | (0<<CS11) | (1<<CS10));	//prescaller 1
	TCNT0 =0;
	solder_init();
	fan_init();
	SOLDER_SENSOR_ADC();
	pid_Init(K_P * SCALING_FACTOR, K_I * SCALING_FACTOR , K_D * SCALING_FACTOR , &solderPID);
	pid_Init(K_P * SCALING_FACTOR, K_I * SCALING_FACTOR , K_D * SCALING_FACTOR , &fan_head_PID);
	SetTimerTask(StartADC, ADC_PERIOD_TICK);
	SetTimerTask(solderControl, SOLDER_PERIOD_TICK);
	console_cb(K_P_CHANGE, pidChangeSolder);
	console_cb(K_I_CHANGE, pidChangeSolder);
	console_cb(K_D_CHANGE, pidChangeSolder);
}
