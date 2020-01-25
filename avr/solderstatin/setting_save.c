#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "eeprom.h"
#include "setting_save.h"

#include "console.h"
#include "stdio.h"

bool eepromInited = false;

set_dev_t setting;

uint8_t *settingBuf = NULL;

//callback function по окончанию операции
void saveResult(epState_t result)
{
    if(result == EEP_READY) {
        console_print("s end\r");
    }
    free(settingBuf);
    settingBuf = NULL;
}

bool settingSave(void)
{
    bool ret = false;

    char tmp[10];
    snprintf(tmp, 10, "s=%d\r", setting.set.solder.value);
    console_print(tmp);
    if (settingBuf == NULL) {//операция еще идет
        if (!eepromInited) {
            EepromInit(sizeof(setting_t));
            eepromInited = true;
        }
        settingBuf = malloc(sizeof(setting_t));
        if (settingBuf) {
            uint8_t count = 0;
            uint8_t *rcv = (uint8_t*) (&setting.set);
            for(; count<sizeof(setting_t); count++) {
                *(settingBuf+count) = *(rcv+count);
            }
            if (EepromStartWrite(settingBuf, saveResult) == EEP_READY) {
                ret = true;
            } else {// освобождаем указатель т.к. операция не началась
                free(settingBuf);
                settingBuf = NULL;
            }
        }
    }
    return ret;
}

bool settingLoad(void)
{
    bool ret = false;
    if (settingBuf == NULL) {//операция еще идет?
        if (!eepromInited) {
            if (EepromInit(sizeof(setting_t)) == EEP_BAD) {
                console_print("no set\r");
                return false;
            }
        }
        eepromInited = true;
        settingBuf = malloc(sizeof(setting_t));
        if (settingBuf) {
            ret = EepromRead(settingBuf) == EEP_READY;
            if (ret) {
                uint8_t count = 0;
                uint8_t *rcv = (uint8_t*) (&setting.set);
                for(; count<sizeof(setting_t); count++) {
                    *(rcv+count) = *(settingBuf+count);
                }
                setting.ready = true;
                char tmp[10];
                snprintf(tmp, 10, "l=%d\r", setting.set.solder.value);
                console_print(tmp);
            }
            free(settingBuf);
            settingBuf = NULL;
        }
    }
    return ret;
}
