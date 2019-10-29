/*
* Таймеры:
* 0 - свободен
* 1 - свободен
* 2 - планировщик
*/
#include <stdio.h>
#include <avr/io.h>
#include <stdlib.h>
#include <avr/wdt.h>
#include "EERTOS.h"
#include "power.h"
#include "display.h"
#include "keyboard.h"
#include "solder_fan.h"

int main(void)
{
	wdt_enable(WDTO_1S);
	adc_preset();//Сразу надо настроить AREF, что бы избежать длительного шунткирования внутреннего AREF
	power_init();
	
	InitRTOS();

	MCUCSR |= (1<<JTD);//Double!
	MCUCSR |= (1<<JTD);

	display_init();
	keyboard_init();
	device_init();
	
	RunRTOS();
	
	while (1){
		TaskManager();
		wdt_reset();
	}

	return 1;
}