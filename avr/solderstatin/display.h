/*
 * display.h
 *
 */
#ifndef DISPLAY_H_
#define DISPLAY_H_
#include "common.h"

#define DISPLAY_REFRESH_MS				250		//обновление дисплея
#define FLASH_PERIOD_MS					500		//Период мигания
#define SHOW_NEED_PERIOD_S				3		//Период в течение которого показывается целевая температура после переключения статус ON

void display_init(void);

#endif /* DISPLAY_H_ */