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
#include "power.h"
#include "setting_save.h"

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

inline void _checkDev(device_t *device, uint16_t *count)
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
            device->state = STATE_OFF;
            *count = 0;
        }
    }
}

//ѕроверка геркона
void GerconControl(void)
{
    static uint16_t countTick = 0;
    if (FanInStand()) {
        countTick++;
    } else {
        countTick = 0;
    }
    if (countTick > (FAN_IN_STAND_PERIOD_OFF_S * PERIOD_1S)/FAN_PERIOD_TICK) { //истек период в течении которого фен находитс€ в подставке
        if (fan_heat.state == STATE_ON) {
            fan_heat.state = STATE_OFF;
        }
        countTick = 0;
    }
    SetTimerTask(GerconControl, FAN_PERIOD_TICK);
}

//ѕроверка наличи€ устройства в гнезде
void checkDevice(void)
{
    static uint16_t countSolder = 0, countFanHeat = 0;

    _checkDev(&solder, &countSolder);
    _checkDev(&fan_heat, &countFanHeat);
    SetTimerTask(checkDevice, NO_DEVICE_MS);
}
//Start stop fan & fan heat
void fanControl(void)
{
    static uint16_t whaitTicFenOff = 0;

    if (fan_heat.state == STATE_OFF) {
        FanHeatOff();
        if (fan_heat.current >= FAN_HEAT_TEMPR_STOP_MIN) {
            FAN_PWM_OCR = 0xff;//Maximum speed fen
            FanOn();
            whaitTicFenOff = FAN_HEAT_TEMPR_STOP_TICK;
            fan.state = STATE_FAN_PREPARE_OFF;
            power_on();
        } else {
            if (whaitTicFenOff) {
                whaitTicFenOff--;
            }
            if (whaitTicFenOff == 0) {
                FanOff();
                fan.state = STATE_OFF;
                power_off();
            }
        }
    } else if ((fan_heat.state == STATE_ON) || (fan_heat.state == STATE_SET)) {
        FanOn();
        FanHeatOn();
        fan.state = STATE_ON;
        power_on();	//гарантировано питание станции
    } else if (fan_heat.state == STATE_NO_DEVICE) {
        FanHeatOff();
        FanOff();
    }
    SetTimerTask(fanControl, FAN_PERIOD_TICK);
}

uint8_t map(uint16_t x, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max)
{
    uint32_t ret = ((uint32_t)x - (uint32_t)in_min) * ((uint32_t)out_max - (uint32_t)out_min) / ((uint32_t)in_max - (uint32_t)in_min) + (uint32_t)out_min;
    return (ret > 0xff)?0xff:ret;
}

//temperature fan heat control
void fanControlPID(void)
{
    int16_t pid = 0;
    static uint8_t prevFan = 0;
    uint8_t speedFan;
    if ((fan_heat.state == STATE_ON) || (fan_heat.state == STATE_SET)) {
        pid = pid_Controller(fan_heat.need, fan_heat.current, &fan_head_PID);
        if (pid >0xff) pid = 0xff;
        FAN_HEAT_PWM_OCR = pid;
        char tmp[UART_BUF_SIZE];
        snprintf(tmp, UART_BUF_SIZE, "a=%d p=%d kp=%d\r", fan_heat.current, pid, fan_head_PID.P_Factor);
        console_print(tmp);
        if (fan_heat.current > fan_heat.limitADC) {
            FanHeatOff();
            fan_heat.state = STATE_OFF;//Alarm!!
        } else {
            FanHeatOn();
            speedFan = map(fan.current, 0, FAN_MAX_ADC, FAN_SPEED_MIN, 0xff);
            snprintf(tmp, UART_BUF_SIZE, "f=%d\r", fan.current);
            console_print(tmp);
            if (prevFan != speedFan) {
                if (speedFan < FAN_SPEED_MIN) speedFan = FAN_SPEED_MIN;
                FAN_PWM_OCR = speedFan;
                prevFan = speedFan;
                console_uint8(prevFan, 1);
            }
        }
    }
    if ((fan.setting->value < fan.limitADC) && (fan.limitADC <= 1024)) {//Ќовый максимальный предел ADC
        fan.setting->value = fan.limitADC;
        settingSave();
    }
    SetTimerTask(fanControlPID, FAN_PID_PERIOD_TICK);
}

void solderControl(void)
{
    int16_t pid=0;
    if ((solder.state == STATE_OFF) || (solder.state == STATE_NO_DEVICE)) {
        SolderOff();
    } else if ((solder.state == STATE_ON) || (solder.state == STATE_SET)) {
        pid = pid_Controller(solder.need, solder.current, &solderPID);
        if (pid <0 ) pid = 0;
        if (pid > 0xff) pid = 0xff;//8 bit
        SOLDER_PWM_OCR = pid;
        /*		char tmp[UART_BUF_SIZE];
        snprintf(tmp, UART_BUF_SIZE, "a=%d p=%d kp=%d\r", solder.current, pid, solderPID.P_Factor);
        console_print(tmp);*/
        if (solder.current > solder.limitADC) {
            SolderOff();
            solder.state = STATE_OFF;
        } else {
            SolderOn();
        }
    }
    SetTimerTask(solderControl, FAN_PERIOD_TICK);
}

void solder_init(void)
{
    PinOutputMode(SOLDER_PWM_PORT, SOLDER_PWM_PIN); //with PWM_INIT
    SolderOff();
    solder.state = STATE_OFF;
    solder.need = 0;
    solder.limitADC = SOLDER_MAX_ADC;
    solder.current = 0;
    solder.setting = &setting.set.solder;
    //default
    solder.setting->D_Factor = K_D;
    solder.setting->I_Factor = K_I;
    solder.setting->P_Factor = K_P;
    solder.setting->value = 0;
}

void fan_init(void)
{
    PinOutputMode(FAN_HEAT_PWM_PORT, FAN_HEAT_PWM_PIN);
    FanHeatOff();
    PinOutputMode(FAN_PWM_PORT, FAN_PWM_PIN);
    FanOff();
    PinInputMode(GERCON_FAN_OUT_PORT, GERCON_FAN_PIN);
    PinOutputMode(FAN_HEAT_POWER_PORT, FAN_HEAT_POWER_PIN);
    FanheatPowerOff();
    fan_heat.state = STATE_OFF;
    fan_heat.need = 0;
    fan_heat.limitADC = FAN_HEAT_MAX_ADC;
    fan_heat.setting = &setting.set.fan_heat;
    //default
    fan_heat.setting->D_Factor = K_D;
    fan_heat.setting->I_Factor = K_I;
    fan_heat.setting->P_Factor = K_P;
    fan_heat.setting->value = 0;
    fan.state = STATE_OFF;
    fan.need = 0;
    fan.current = 0;
    fan.limitADC = FAN_MAX_ADC; //speed regulator
    fan.setting = &setting.set.fan;
    fan.setting->value = fan.limitADC;
}

void StartADC(void)
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

uint8_t pidChange(uint8_t cmd, uint16_t value)
{
    bool change = false;
    struct PID_DATA *pid = (cmd & K_DEVICE_MASK) == K_PID_SOLDER ? &solderPID : &fan_head_PID;
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
        settingSave();
        pid_Reset_Integrator(pid);
        char tmp[UART_BUF_SIZE];
        snprintf(tmp, UART_BUF_SIZE, "c %02x new k=%04x max=%04x\r", cmd, value, pid->maxError);
        console_print(tmp);
    }
    return 0;
}

uint8_t fanSpeedStartChange(uint8_t cmd, uint16_t value)
{
    fan.state = STATE_SET;
    fan.need = (uint8_t) value;
    return 0;
}

uint8_t fanSpeedStopChange(uint8_t cmd, uint16_t value)
{
    fan.state = STATE_OFF;
    return 0;
}

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
    SetTask(fanControlPID);
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
    //fan speed command
    console_cb(FAN_SPEED_START_CHANGE, fanSpeedStartChange);
    console_cb(FAN_SPEED_STOP_CHANGE, fanSpeedStopChange);
}
