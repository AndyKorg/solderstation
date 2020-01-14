#include <stdio.h>
#include <avr/io.h>
#include <stdlib.h>
#include <avr/wdt.h>
#include "EERTOS.h"
#include "power.h"
#include "display.h"
#include "keyboard.h"
#include "solder_fan.h"

#include "avrlibdefs.h"

#include "console.h"

#include "common.h"
#include "HAL.h"

int main(void)
{
	//wdt_enable(WDTO_1S);
	adc_preset();//Сразу надо настроить AREF, что бы избежать длительного шунтирования внутреннего AREF
	MCUCSR |= (1<<JTD);//Double!
	MCUCSR |= (1<<JTD);

	PinOutputMode(FAN_PWM_PORT, FAN_PWM_PIN);

	power_init();

	InitRTOS();
	console_init();
	
	RunRTOS();
	
	device_init();
	display_init();
	keyboard_init();

	console_print("start\r");
	
	while (1){
		TaskManager();
		wdt_reset();
	}

	return 1;
}