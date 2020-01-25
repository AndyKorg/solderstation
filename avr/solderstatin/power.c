/*
 * power.c
 *
 */ 

#include <stdio.h>
#include <avr/io.h>
#include <stdlib.h>
#include "common.h"
#include "HAL.h"

void power_init(void){
	PinOutputMode(POWER_OFF_OUT, POWER_OFF_PIN);
	PowerOff();//Что бы не щелкало при каждом включении
}

void power_off(){
	PowerOff();
}

void power_on(){
	PowerOn();
}
