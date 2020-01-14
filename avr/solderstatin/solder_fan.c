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
#define K_P     0.00 //max 1.265625
//Integral kofficent
#define K_I     0.00
//Differential coefficient
#define K_D     0.00

//Change PID koefficent
#define K_CHANGE_MASK			0x0f
#define K_DEVICE_MASK			0xf0
#define K_P_CHANGE				(K_CHANGE_MASK & 1)
#define K_I_CHANGE				(K_CHANGE_MASK & 2)
#define K_D_CHANGE				(K_CHANGE_MASK & 3)
#define K_PID_SOLDER			(K_DEVICE_MASK & 0xd0)	//Solder command PID
#define K_PID_FAN_HEAT			(K_DEVICE_MASK & 0xc0)	//Fan heat command PID

//Change fan speed
#define FAN_SPEED_START_CHANGE	0xa0
#define FAN_SPEED_STOP_CHANGE	0xa1

static struct PID_DATA solderPID, fan_head_PID;

//Сработал геркон
ISR(GERCON_FAN_INT){
	
}

ISR(ADC_vect){
	static uint8_t countMux = 0;
	switch(countMux){
		case 0:
		fan.current = ADC;//speed regulator!
		countMux = 1;
		SOLDER_SENSOR_ADC();
		break;
		case 1:
		solder.current = ADC;
		countMux = 2;
		FAN_HEAT_SENSOR_ADC();
		break;
		case 2:
		fan_heat.current = ADC;
		countMux = 0;
		FAN_SENSOR_ADC();
		break;
		default:
		break;
	}
}
//Start stop fan & fan heat
void fanControl(void){
	static uint16_t whaitTicFenOff = 0;
	
	if (fan_heat.state == STATE_OFF){
		FanHeatOff();
		if (fan_heat.current >= FAN_HEAT_TEMPR_STOP_MIN){
			FAN_PWM_OCR = 0xff;//Maximum speed fen
			FanOn();
			whaitTicFenOff = FAN_HEAT_TEMPR_STOP_TICK;
		}
		else{
			if (whaitTicFenOff){
				whaitTicFenOff--;
			}
			if (whaitTicFenOff == 0){
				FanOff();
			}
		}
	}
	else if ((fan_heat.state == STATE_ON) || (fan_heat.state == STATE_SET)){
		FanOn();
		FanHeatOn();
	}
	SetTimerTask(fanControl, FAN_PERIOD_TICK);
}
//temperature fan heat control
void fanControlPID(void){
	int16_t pid = 0;
	static uint8_t prevFan = 0;
	uint8_t speedFan;
	if ((fan_heat.state == STATE_ON) || (fan_heat.state == STATE_SET)){
		pid = pid_Controller(fan_heat.need, fan_heat.current, &fan_head_PID);
		if (pid >0xff) pid = 0xff;
		FAN_HEAT_PWM_OCR = pid;
		char tmp[UART_BUF_SIZE];
		snprintf(tmp, UART_BUF_SIZE, "a=%d p=%d kp=%d\r", fan_heat.current, pid, fan_head_PID.P_Factor);
		console_print(tmp);
		if (fan_heat.current > fan_heat.limitADC){
			FanHeatOff();
		}
		else{
			FanHeatOn();
		}
		//Hand control fan
		if (fan.state == STATE_SET){
			speedFan = fan_heat.current < FAN_HEAT_TEMPR_STOP_MIN?fan.need:0xff;
		}
		else{
			speedFan = (uint8_t) (fan.current>>2);
		}
		if (prevFan != speedFan){
			if (speedFan < FAN_SPEED_MIN) speedFan = FAN_SPEED_MIN;
			FAN_PWM_OCR = speedFan;
			prevFan = speedFan;
			console_uint8(prevFan, 1);
		}
	}
	SetTimerTask(fanControlPID, FAN_PID_PERIOD_TICK);
}

void solderControl(void){
	int16_t pid=0;
	if (solder.state == STATE_OFF){
		SolderOff();
	}
	else if ((solder.state == STATE_ON) || (solder.state == STATE_SET)){
		pid = pid_Controller(solder.need, solder.current, &solderPID);
		if (pid <0 ) pid = 0;
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
	SetTimerTask(solderControl, FAN_PERIOD_TICK);
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
	PinOutputMode(FAN_HEAT_PWM_PORT, FAN_HEAT_PWM_PIN);
	FanHeatOff();
	PinOutputMode(FAN_PWM_PORT, FAN_PWM_PIN);
	FanOff();
	fan_heat.state = STATE_OFF;
	fan_heat.need = 0;
	fan_heat.limitADC = 550;//over 450 C//!DEBUG!
	fan.state = STATE_OFF;
	fan.need = 0;
	fan.current = 0;
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

uint8_t pidChange(uint8_t cmd, uint16_t value){
	bool change = false;
	struct PID_DATA *pid = (cmd & K_DEVICE_MASK) == K_PID_SOLDER ? &solderPID : &fan_head_PID;
	switch (cmd & K_CHANGE_MASK){
		case K_P_CHANGE:
		pid_Init((uint8_t)value, pid->I_Factor, pid->D_Factor, pid);
		change = true;
		break;
		case K_I_CHANGE:
		pid_Init(pid->P_Factor, (uint8_t) value, pid->D_Factor, pid);
		change = true;
		break;
		case K_D_CHANGE:
		pid_Init(pid->P_Factor, pid->I_Factor, (uint8_t) value, pid);
		change = true;
		break;
		default:
		break;
	}
	if (change){
		pid_Reset_Integrator(pid);
		char tmp[UART_BUF_SIZE];
		snprintf(tmp, UART_BUF_SIZE, "c %02x new k=%04x max=%04x\r", cmd, value, pid->maxError);
		console_print(tmp);
	}
	return 0;
}

uint8_t fanSpeedStartChange(uint8_t cmd, uint16_t value){
	fan.state = STATE_SET;
	fan.need = (uint8_t) value;
	return 0;
}

uint8_t fanSpeedStopChange(uint8_t cmd, uint16_t value){
	fan.state = STATE_OFF;
	return 0;
}

void device_init(){
	FanAndSolderTimerInit();
	solder_init();
	fan_init();
	SOLDER_SENSOR_ADC();
	pid_Init(K_P * SCALING_FACTOR, K_I * SCALING_FACTOR , K_D * SCALING_FACTOR , &solderPID);
	pid_Init(K_P * SCALING_FACTOR, K_I * SCALING_FACTOR , K_D * SCALING_FACTOR , &fan_head_PID);
	SetTimerTask(StartADC, ADC_PERIOD_TICK);
	SetTimerTask(solderControl, SOLDER_PERIOD_TICK);
	SetTimerTask(fanControl, FAN_PERIOD_TICK);
	SetTimerTask(fanControlPID, FAN_PID_PERIOD_TICK);
	//solder command
	console_cb(K_PID_SOLDER+K_P_CHANGE, pidChange);
	console_cb(K_PID_SOLDER+K_I_CHANGE, pidChange);
	console_cb(K_PID_SOLDER+K_D_CHANGE, pidChange);
	//fan heat command
	console_cb(K_PID_FAN_HEAT+K_P_CHANGE, pidChange);
	console_cb(K_PID_FAN_HEAT+K_I_CHANGE, pidChange);
	console_cb(K_PID_FAN_HEAT+K_D_CHANGE, pidChange);
	//fan speed command
	console_cb(FAN_SPEED_START_CHANGE, fanSpeedStartChange);
	console_cb(FAN_SPEED_STOP_CHANGE, fanSpeedStopChange);
}
