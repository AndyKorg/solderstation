/*
 * Спасение настроек в EEPROM
 * Используется весь массив eeprom для равномерного расходования ресурса flash-ячеек. Не знаю на сколько это вообще нужно, но прикольно.
 */


#ifndef EEPROM_H_
#define EEPROM_H_

#include <stdint.h>

typedef enum {
    EEP_BAD = 0,										//в eeprom нет коректных данных
    EEP_READY = 1,										//Память корректна
} epState_t;


typedef void (*epEndOp)(epState_t result);				//callback function по окончанию операции

epState_t EepromInit(size_t sizePacket);				//Инициализация указателей и состояния EEPROM. sizePacket - размер структуры сохраняемой в eeprom
epState_t EepromStartWrite(uint8_t *value, epEndOp cb_endOp);	//Начать запись value в EEPROM. Фунция окончания записи вызывается в прерывании!
epState_t EepromRead(uint8_t *value);					//Прочитать данные в value

#endif /* EEPROM_H_ */