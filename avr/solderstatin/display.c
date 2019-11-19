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

#include "console.h"

#define  DISPLAY_MAX_BUF 32

uint8_t flash = 0;

typedef struct{
	uint8_t x;
	uint8_t y;
	char str[DISPLAY_MAX_BUF];
	sFONT *font;
	eColored color;
} dispString_t;

typedef uint8_t (*strOut)(dispString_t *value);

static strOut stringOut;

inline uint8_t itoaFlash(device_t dev, char *buf){
	if ((dev.state == STATE_SET) && (flash)){
		return (uint8_t) snprintf(buf, DISPLAY_MAX_BUF, "    ");
	}
	return (uint8_t) snprintf(buf, DISPLAY_MAX_BUF, "%04d", (dev.state == STATE_SET)?dev.setSelect:dev.current);
}
/*
void Show(void){
char tmp[DISPLAY_MAX_BUF];
uint8_t xPosEndTempr;
xPosEndTempr = drawStringAt(0, 0, itoaFlash(solder, tmp), &FontSuperBigDigit, COLORED_SHOW);
drawStringAt(xPosEndTempr, 0, (solder.state == STATE_OFF)?" OFF":" ON  ", &FontSmall, COLORED_SHOW);
u08 y = enterY(0, 0, &FontSuperBigDigit)+4;
xPosEndTempr = drawStringAt(0, y, itoaFlash(fan_head, tmp), &FontSuperBigDigit, COLORED_SHOW);
PORTB ^= (1<<PORTB0);
SetTimerTask(Show, DISPLAY_REFRESH_MS);
}
*/
uint8_t solderTempr(dispString_t *value);

uint8_t fan_headState(dispString_t *value){
	value->x = value->x;
	value->y = value->y;
	value->color = COLORED_SHOW;
	value->font = &FontSmall;
	stringOut = solderTempr;
	return (uint8_t) snprintf(value->str, DISPLAY_MAX_BUF, "%s", ((fan_head.state == STATE_OFF)?" OFF":" ON  "));
}

uint8_t fan_headTempr(dispString_t *value){
	value->x = 0;
	value->y = enterY(0, 0, &FontSuperBigDigit)+4;
	value->color = COLORED_SHOW;
	value->font = &FontSuperBigDigit;
	stringOut = fan_headState;
	return itoaFlash(fan_head, value->str);
}

uint8_t solderState(dispString_t *value){
	value->x = value->x;
	value->y = 0;
	value->color = COLORED_SHOW;
	value->font = &FontSmall;
	stringOut = fan_headTempr;
	return (uint8_t) snprintf(value->str, DISPLAY_MAX_BUF, "%s", ((solder.state == STATE_OFF)?" OFF":" ON  "));
}

uint8_t solderTempr(dispString_t *value){
	value->x = 0;
	value->y = 0;
	value->color = COLORED_SHOW;
	value->font = &FontSuperBigDigit;
	stringOut = solderState;
	return itoaFlash(solder, value->str);
}

void ShowString(void){
	static dispString_t current;
	static uint8_t charCount = 0, currPos;
	if (charCount == 0){
		if (stringOut){
			charCount = stringOut(&current);
			if (charCount >DISPLAY_MAX_BUF){
				charCount = DISPLAY_MAX_BUF;
				current.str[DISPLAY_MAX_BUF-1] = 0;
			}
			currPos = 0;
		}
	}
	if(charCount){
		current.x = drawCharAt(current.x, current.y, current.str[currPos++], current.font, current.color);
		//put_pixel(0,0,COLORED_SHOW);
		charCount--;
	}
	SetTimerTask(ShowString, 10);
}

void flashSwitch(void){
	flash ^= 0xff;
	SetTimerTask(flashSwitch, FLASH_PERIOD_MS);
}

void display_init(void){
	stringOut = solderTempr;
	wg12864_init(ShowString);	//Инициализация ЖКИ
	SetTimerTask(flashSwitch, FLASH_PERIOD_MS);
}