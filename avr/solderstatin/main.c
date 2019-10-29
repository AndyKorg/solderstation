/*
* �������:
* 0 - ��������
* 1 - ��������
* 2 - �����������
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
	adc_preset();//����� ���� ��������� AREF, ��� �� �������� ����������� ������������� ����������� AREF
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