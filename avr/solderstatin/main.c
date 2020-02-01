#include <stdio.h>
#include <avr/io.h>
#include <stdlib.h>
#include <avr/wdt.h>
#include "EERTOS.h"
#include "power.h"
#include "display.h"
#include "keyboard.h"
#include "solder_fan.h"
#include "setting_save.h"
#include "buzzer.h"

#include "avrlibdefs.h"

#include "console.h"

#include "common.h"
#include "HAL.h"


#define soft_reset()        \
do                          \
{                           \
	wdt_enable(WDTO_15MS);  \
	for(;;)                 \
	{                       \
	}                       \
} while(0)

uint8_t reset(uint8_t cmd, uint16_t value)
{
    soft_reset();
    return 0;
}

uint8_t save_cmd(uint8_t cmd, uint16_t value)
{
    settingSave();
    return 0;
}

uint8_t load_cmd(uint8_t cmd, uint16_t value)
{
    console_print("l strt");
    if (settingLoad()) {
        console_print("l ok");
    }
    return 0;
}

int main(void)
{
    wdt_enable(WDTO_1S);
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
    buzzerInit();
    keyboard_init();

    console_print("start\r");
    console_cb(RESET_CMD, reset);
    console_cb(SAVE_CMD, save_cmd);
    console_cb(LOAD_CMD, load_cmd);

    while (1) {
        TaskManager();
        wdt_reset();
    }

    return 1;
}