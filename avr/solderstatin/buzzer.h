/*
 * Only for STLED316 mode
 */


#ifndef BUZZER_H_
#define BUZZER_H_

#include "HAL.h"
#include "common.h"

#ifdef KEY_MODULE_STLED
void buzzerInit(void);
void buzzerShow(sound_type_t sound);
#endif

#endif /* BUZZER_H_ */