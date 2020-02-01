/*
 * display.h
 *
 */
#ifndef DISPLAY_H_
#define DISPLAY_H_
#include "common.h"

#define DISPLAY_REFRESH_MS				250		//обновление диспле€
#define FLASH_PERIOD_MS					500		//ѕериод мигани€
#define SHOW_NEED_PERIOD_S				3		//ѕериод в течение которого показываетс€ целева€ температура после переключени€ статус ON

void display_init(void);
void displaySetting(void);		//показать установки. –аботает только если все устройства в статусе off

#endif /* DISPLAY_H_ */