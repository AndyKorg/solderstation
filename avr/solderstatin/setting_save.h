/*
 * хранение настроек в eeprom
 */


#ifndef SETTING_SAVE_H_
#define SETTING_SAVE_H_

#include <stdint.h>
#include <stdbool.h>
#include "common.h"

typedef struct {
    setting_t set;
    bool ready;
} set_dev_t;

extern set_dev_t setting;

bool settingSave(void);
bool settingLoad(void);

#endif /* SETTING_SAVE_H_ */