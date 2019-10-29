/*
 * display.c
 *
 */ 

#include <stdio.h>
#include <avr/io.h>
#include <stdlib.h>
#include "avrlibtypes.h"
#include "common.h"
#include "EERTOS.h"
#include "HAL.h"
#include "wg12864b.h"
#include "display.h"


u08 flash = 0;

inline char *itoaFlash(sDevice dev, char *buf){
	if ((dev.state == STATE_SET) && (flash)){
		buf = "   ";
	}
	else{
		u16 tmp = (dev.state == STATE_SET)?dev.setSelect:dev.current;
		/*if (tmp > 999){
			tmp = 999;
		}*/
		sprintf(buf, "%04d", tmp);
	}
	return buf;
}
void Show(void){
	char tmp[6];
	solder.xPosEndTempr = drawStringAt(0, 0, itoaFlash(solder, tmp), &FontSuperBigDigit, COLORED_SHOW);
	drawStringAt(solder.xPosEndTempr, 0, (solder.state == STATE_OFF)?" OFF":" ON  ", &FontSmall, COLORED_SHOW);
	u08 y = enterY(0, 0, &FontSuperBigDigit)+4;
	fan.xPosEndTempr = drawStringAt(0, y, itoaFlash(fan, tmp), &FontSuperBigDigit, COLORED_SHOW);
	SetTimerTask(Show, DISPLAY_REFRESH_MS);
}

void flashSwitch(void){
	flash ^= 0xff;
	SetTimerTask(flashSwitch, FLASH_PERIOD_MS);
}

void display_init(void){
	wg12864_init();	//Инициализация ЖКИ
	SetTimerTask(Show, DISPLAY_REFRESH_MS);
	SetTimerTask(flashSwitch, FLASH_PERIOD_MS);
}