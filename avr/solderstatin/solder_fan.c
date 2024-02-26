/*
*  онтроль фена и па€льника
* TODO: –азнести по разным файлам
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
#include "power.h"
#include "setting_save.h"
#include "buzzer.h"

#if (SOLDER_MAX_ADC >= NO_DEVICE_ADC)
#error "SOLER_MAX_ADC >= NO_DEVICE_ADC! Check them!"
#endif
#if (FAN_HEAT_MAX_ADC >= NO_DEVICE_ADC)
#error "FAN_HEAT_MAX_ADC >= NO_DEVICE_ADC! Check them!"
#endif

#define ADC_START()			do {ADCSRA = (1<<ADEN) | (1<<ADSC) | (0<<ADATE) | (1<<ADIE) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);} while (0)

static struct PID_DATA solderPID, fan_head_PID;

ISR(ADC_vect)
{
	static uint8_t countMux = 0;
	uint16_t tmp;

	switch(countMux) {
		case 0:
		tmp = ADC;
		if (tmp > fan.limitADC) {
			fan.limitADC = tmp; //ѕодогнать границу под текущие детали
		}
		fan.current = fan.limitADC - tmp;//speed regulator! Ќа плате первой версии перепутаны выводы переменного резистора
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

static inline void _checkDev(device_t *device, uint16_t *count)
{
	if (device->state != STATE_NO_DEVICE) {	//”стройство на месте, ждем его исчезновени€
		*count = (device->current >NO_DEVICE_ADC)?(*count+1):0;
		if (*count > ((NO_DEVICE_PERIOD_S * PERIOD_1S)/NO_DEVICE_MS)) {//ѕревышен период контрол€, переводим устройство в отсутствующие
			device->state = STATE_NO_DEVICE;
			*count = 0;
		}
		} else {				//Ќет устройства, ждем его по€влени€
		*count = (device->current <NO_DEVICE_ADC)?(*count+1):0;
		if (*count > ((DEVICE_CONNECT_S * PERIOD_1S)/NO_DEVICE_MS)) {
			device->state = STATE_NORMAL;
			*count = 0;
		}
	}
}

//ѕроверка геркона
static void GerconControl(void)
{
	static uint16_t countTick = 0;
	if (FanInStand()) {
		countTick++;
		} else {
		countTick = 0;
	}
	if (countTick > (FAN_IN_STAND_PERIOD_OFF_S * PERIOD_1S)/FAN_PERIOD_TICK) { //истек период в течении которого фен находитс€ в подставке
		if (fan_heat.on) {
			fan_heat.on = false;
		}
		countTick = 0;
	}
	SetTimerTask(GerconControl, FAN_PERIOD_TICK);
}

//ѕроверка наличи€ устройства в гнезде
static void checkDevice(void)
{
	static uint16_t countSolder = 0, countFanHeat = 0;

	_checkDev(&solder, &countSolder);
	_checkDev(&fan_heat, &countFanHeat);
	SetTimerTask(checkDevice, NO_DEVICE_MS);
}

//Start stop fan & fan heat
static void fanControl(void)
{
	static uint16_t whaitTicFenOff = 0;

	if (!fan_heat.on) {
		FanHeatOff();
		if (fan_heat.current >= FAN_HEAT_TEMPR_STOP_MIN) {
			FAN_PWM_OCR = 0xff;//Maximum speed fen
			FanOn();
			whaitTicFenOff = FAN_HEAT_TEMPR_STOP_TICK;
			fan.state = STATE_FAN_PREPARE_OFF;
			fan.on = true;
			power_on();
			} else {
			if (whaitTicFenOff) {
				whaitTicFenOff--;
			}
			if (whaitTicFenOff == 0) {
				FanOff();
				fan.state = STATE_NORMAL;
				fan.on = false;
				power_off();
			}
		}
		} else if (fan_heat.on) {
		FanOn();
		FanHeatOn();
		fan.on = true;
		fan.state = STATE_NORMAL;
		power_on();	//гарантировано питание станции
		} else if (fan_heat.state == STATE_NO_DEVICE) {
		FanHeatOff();
		FanOff();
		fan_heat.on = false;
		fan.on = false;
	}
	SetTimerTask(fanControl, FAN_PERIOD_TICK);
}

static uint8_t map(uint16_t x, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max)
{
	uint32_t ret = ((uint32_t)x - (uint32_t)in_min) * ((uint32_t)out_max - (uint32_t)out_min) / ((uint32_t)in_max - (uint32_t)in_min) + (uint32_t)out_min;
	return (ret > 0xff)?0xff:ret;
}

//temperature fan heat control
static void fanControlTempr(void)
{
	int16_t fan_heat_temper = 0;
	static uint8_t prevFan = 0;
	uint8_t speedFan;
	if (fan_heat.on) {
		switch (fan_heat.setting->Method){
			case METHOD_SIMPLE:
			fan_heat_temper = solder.need - solder.current;
			if (fan_heat_temper < TEMPERATURE_HYSTERESSIS){
				fan_heat_temper = 0;
			}
			fan_heat_temper = map(fan_heat_temper, 0, FAN_HEAT_MAX, 0, 0xff);
			break;
			case METHOD_PID:
			fan_heat_temper = pid_Controller(fan_heat.need, fan_heat.current, &fan_head_PID);
			break;
			case METHOD_NOT_APP:
			//Ќа вс€кий случай
			break;
		}
		if (fan_heat_temper >0xff) fan_heat_temper = 0xff;
		if (fan_heat_temper<0) fan_heat_temper = 0;
		FAN_HEAT_PWM_OCR = (uint8_t) fan_heat_temper;
		char tmp[UART_BUF_SIZE];
		snprintf(tmp, UART_BUF_SIZE, "%d\r", fan_heat.current);
		console_print(tmp);
		if (fan_heat.current > fan_heat.limitADC) {
			FanHeatOff();
			if (fan_heat.on) {
				buzzerShow(SOUND_FLASH_3);
			}
			fan_heat.on = false;//Alarm!!
			} else {
			FanHeatOn();
			speedFan = map(fan.current, 0, FAN_MAX_ADC, FAN_SPEED_MIN, 0xff);
			if (prevFan != speedFan) {
				if (speedFan < FAN_SPEED_MIN) speedFan = FAN_SPEED_MIN;
				FAN_PWM_OCR = speedFan;
				prevFan = speedFan;
			}
		}
		} else {
		prevFan = 0;//сбросить скорость что бы при пуске правильно настроить
	}
	if ((fan.setting->value < fan.limitADC) && (fan.limitADC <= 1024)) {//Ќовый максимальный предел ADC
		fan.setting->value = fan.limitADC;
		settingSave();
	}
	SetTimerTask(fanControlTempr, FAN_PID_PERIOD_TICK);
}

static void solderControl(void)
{
	int16_t solder_tempr=0;
	if ((!solder.on) || (solder.state == STATE_NO_DEVICE)) {
		SolderOff();
		} else if (solder.on) {
		switch (solder.setting->Method){
			case METHOD_SIMPLE:
			solder_tempr = solder.need - solder.current;
			if (solder_tempr < TEMPERATURE_HYSTERESSIS){
				solder_tempr = 0;
			}
			solder_tempr = map(solder_tempr, 0, SOLDER_MAX, 0, 0xff);
			break;
			case METHOD_PID:
			solder_tempr = pid_Controller(solder.need, solder.current, &solderPID);
			break;
			case METHOD_NOT_APP:
			//Ќа вс€кий случай
			break;
		}
		if (solder_tempr <0 ) solder_tempr = 0;
		if (solder_tempr > 0xff) solder_tempr = 0xff;//8 bit
		SOLDER_PWM_OCR = solder_tempr;
		char tmp[UART_BUF_SIZE];
		snprintf(tmp, UART_BUF_SIZE, "%d\r", solder.current);
		console_print(tmp);
		if (solder.current > solder.limitADC) {
			SolderOff();
			if (solder.on) {
				buzzerShow(SOUND_FLASH_3);//Alarm
			}
			solder.on = false;
			} else {
			SolderOn();
		}
	}
	SetTimerTask(solderControl, SOLDER_PERIOD_TICK);
}

static void solder_init(void)
{
	PinOutputMode(SOLDER_PWM_PORT, SOLDER_PWM_PIN); //with PWM_INIT
	SolderOff();
	solder.on = false;
	solder.state = STATE_NORMAL;
	solder.need = 0;
	solder.maxValue = SOLDER_MAX;
	solder.limitADC = SOLDER_MAX_ADC;
	solder.current = 0;
	solder.disp_add = SOLDER_KOEF_DISP;
	solder.setting = &setting.set.solder;
	//default
	solder.setting->D_Factor = K_D_SOLDER;
	solder.setting->I_Factor = K_I_SOLDER;
	solder.setting->P_Factor = K_P_SOLDER;
	solder.setting->value = 0;
	solder.setting->Method = METHOD_SIMPLE;
}

static void fan_init(void)
{
	PinOutputMode(FAN_HEAT_PWM_PORT, FAN_HEAT_PWM_PIN);
	FanHeatOff();
	PinOutputMode(FAN_PWM_PORT, FAN_PWM_PIN);
	FanOff();
	PinInputMode(GERCON_FAN_OUT_PORT, GERCON_FAN_PIN);
	PinOutputMode(FAN_HEAT_POWER_PORT, FAN_HEAT_POWER_PIN);
	FanheatPowerOff();
	fan_heat.on = false;
	fan_heat.state = STATE_NORMAL;
	fan_heat.need = 0;
	fan_heat.maxValue = FAN_HEAT_MAX;
	fan_heat.limitADC = FAN_HEAT_MAX_ADC;
	fan_heat.setting = &setting.set.fan_heat;
	fan_heat.disp_add = FAN_HEAT_KOEF_DISP;
	//default
	fan_heat.setting->D_Factor = K_D_FAN_HEAT;
	fan_heat.setting->I_Factor = K_I_FAN_HEAT;
	fan_heat.setting->P_Factor = K_P_FAN_HEAT;
	fan_heat.setting->value = 0;
	fan_heat.setting->Method = METHOD_SIMPLE;
	fan.on = false;
	fan.state = STATE_NORMAL;
	fan.need = 0;
	fan.current = 0;
	fan.limitADC = FAN_MAX_ADC; //speed regulator
	fan.setting = &setting.set.fan;
	fan.setting->value = fan.limitADC;
	fan.setting->Method = METHOD_NOT_APP;
}

static void StartADC(void)
{
	if (!(ADCSRA & (1<<ADSC))) {
		ADC_START();
	}
	SetTimerTask(StartADC, ADC_PERIOD_TICK);
}

//ѕреднастройка adc
void adc_preset(void)
{
	SOLDER_SENSOR_ADC();
}

static uint8_t pidChange(uint8_t cmd, uint16_t value)
{
	bool change = false;
	struct PID_DATA *pid = (cmd & K_DEVICE_MASK) == K_PID_SOLDER ? &solderPID : &fan_head_PID;
	device_t *dev = (cmd & K_DEVICE_MASK) == K_PID_SOLDER ? &solder : &fan_heat;
	switch (cmd & K_CHANGE_MASK) {
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
	if (change) {
		dev->setting->P_Factor = pid->P_Factor;
		dev->setting->I_Factor = pid->I_Factor;
		dev->setting->D_Factor = pid->D_Factor;
		settingSave();
		pid_Reset_Integrator(pid);
		char tmp[UART_BUF_SIZE];
		snprintf(tmp, UART_BUF_SIZE, "c %02x new k=%04x max=%04x\r", cmd, value, pid->maxError);
		console_print(tmp);
	}
	return 0;
}

static uint8_t readPID(uint8_t cmd, uint16_t value)
{
	char tmp[UART_BUF_SIZE];
	if (value == K_READ_PRM) {
		snprintf(tmp, UART_BUF_SIZE, "SP %d\r", solderPID.P_Factor);
		console_print(tmp);
		snprintf(tmp, UART_BUF_SIZE, "SI %d\r", solderPID.I_Factor);
		console_print(tmp);
		snprintf(tmp, UART_BUF_SIZE, "SD %d\r", solderPID.D_Factor);
		console_print(tmp);
		snprintf(tmp, UART_BUF_SIZE, "FP %d\r", fan_head_PID.P_Factor);
		console_print(tmp);
		snprintf(tmp, UART_BUF_SIZE, "FI %d\r", fan_head_PID.I_Factor);
		console_print(tmp);
		snprintf(tmp, UART_BUF_SIZE, "FD %d\r", fan_head_PID.D_Factor);
		console_print(tmp);
		snprintf(tmp, UART_BUF_SIZE, "scale %d\r", SCALING_FACTOR);
		console_print(tmp);
	}
	return 0;
}

#ifdef CONSLOE_FAN_SPEED
static uint8_t fanSpeedStartChange(uint8_t cmd, uint16_t value)
{
	fan.state = STATE_SET;
	fan.need = (uint8_t) value;
	return 0;
}

static uint8_t fanSpeedStopChange(uint8_t cmd, uint16_t value)
{
	fan.state = STATE_NORMAL;
	return 0;
}
#endif

void device_init()
{
	FanAndSolderTimerInit();
	solder_init();
	fan_init();
	if (settingLoad()) { //загрузить настройки из eeprom если они там есть
		solder.need = solder.setting->value;
		fan_heat.need = fan_heat.setting->value;
		fan.limitADC = fan.setting->value;
	}
	SOLDER_SENSOR_ADC();
	pid_Init(solder.setting->P_Factor * SCALING_FACTOR, solder.setting->I_Factor * SCALING_FACTOR, solder.setting->D_Factor * SCALING_FACTOR, &solderPID);
	pid_Init(fan_heat.setting->P_Factor * SCALING_FACTOR, fan_heat.setting->I_Factor * SCALING_FACTOR, fan_heat.setting->D_Factor * SCALING_FACTOR, &fan_head_PID);
	SetTask(StartADC);
	SetTask(solderControl);
	SetTask(fanControl);
	SetTask(fanControlTempr);
	SetTask(checkDevice);
	SetTask(GerconControl);
	//solder command
	console_cb(K_PID_SOLDER+K_P_CHANGE, pidChange);
	console_cb(K_PID_SOLDER+K_I_CHANGE, pidChange);
	console_cb(K_PID_SOLDER+K_D_CHANGE, pidChange);
	//fan heat command
	console_cb(K_PID_FAN_HEAT+K_P_CHANGE, pidChange);
	console_cb(K_PID_FAN_HEAT+K_I_CHANGE, pidChange);
	console_cb(K_PID_FAN_HEAT+K_D_CHANGE, pidChange);
	//read pid koefficent
	console_cb(K_READ_CMD, readPID);
	#ifdef CONSLOE_FAN_SPEED
	//fan speed command
	console_cb(FAN_SPEED_START_CHANGE, fanSpeedStartChange);
	console_cb(FAN_SPEED_STOP_CHANGE, fanSpeedStopChange);
	#endif
}
