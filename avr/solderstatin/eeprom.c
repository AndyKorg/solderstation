/*
* запись структуры в eeprom
* ВНИМАНИЕ! Из-за возможности работы без регулятора громкости сохранение настройки часов должно производится последним!
*/
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <avr/eeprom.h>
#include "eeprom.h"


volatile uint16_t epAdrWrite, epAdrStartWr; 								//адреса записи в eeprom - текущий и стартовый
volatile uint16_t epPacketSize;												//Размер пакета
uint16_t epAdr;																//Адрес начала пакета. Если равно EEPROM_NULL то адрес не корректен
volatile bool epProcess = false;											//Идет операция в eeprom

volatile uint8_t epStepWr;													//Счетчик шагов показывает сколько раз была вызвана функция из прерывания окончания записи
volatile uint8_t epCountWr;

epEndOp	ep_endOpFunc = NULL;
volatile uint8_t *epBufPacket = NULL;										//Буфер пакета

#define epWriteInterruptOff()	do {EECR &= ~(1<<EERIE);} while (0)			//Запретить прерывания от EEPROM, а следовательно и закончить запись
#define epWriteInterruptOn()	do {EECR |= (1<<EERIE);} while(0)			//Разрешить прерывание от блока записи - старт записи
#define EEPROM_SIGNATURE		0xaa										//Настройки в EEPROM действительны
#define EEPROM_NO_SIGNATURE		0x55										//Настройки в EEPROM не действительна, необходимо проверить следующий пакет

#define EEPROM_BASE				0											//Базовый адрес с которого начинается хранение в eeprom
#define EEPROM_NULL				(E2END+1)									//Признак некорректности адреса eeprom

#define eepromWriteBusy()		((EECR & (1<<(EEWE))) != 0)					//Память занята записью
#define eepromWriteIsFree()		((EECR & (1<<(EEWE))) == 0)					//Память не занята процессом записи


/************************************************************************/
/* Чтение байта из eeprom									            */
/************************************************************************/
uint16_t EeprmReadByte(volatile const uint16_t Adr)
{
    while(eepromWriteBusy());												//Ожидание завершения предыдущей операции с EEPROM
    EEAR = Adr;
    EECR |= (1<<EERE);
    return EEDR;
}

/************************************************************************/
/* Запись байта по адресу epAdrWrite                                    */
/************************************************************************/
inline void WrieByteEeprom(uint16_t adr, uint8_t Value)
{
    while(eepromWriteBusy());												//Ожидание завершения предыдущей операции с EEPROM, тут на всякий случай. Всегда должно за один проход выполнятся
    EEAR = adr;
    EECR |= (1<<EERE);
    if (EEDR != Value) {														//Байты не совпадают, надо переписать
        EEDR = Value;
        cli();
        EECR |= (1<<EEMWE);													//Разрешить запись
        EECR |= (1<<EEWE);													//Пишется
        sei();
    }
}

/************************************************************************/
/* Возвращает адрес в eeprom для следующего пакета                      */
/************************************************************************/
inline uint16_t epAdrNextPacket(void)
{
    if ((epAdr == EEPROM_NULL) || ((epAdr+epPacketSize) > E2END)) {		//Адрес пакета еще не определен или пакет уходит за границу адресов памяти eeprom, адрес с начала памяти eeprom
        return EEPROM_BASE;
    }
    return epAdr+epPacketSize;										//Текущий адрес сдвигаем на размер пакета
}

//операция закончена, прерывания от записи запретить, вызвать функцию окончания операции
inline void epEndWriteOperation(void)
{
    epWriteInterruptOff();
    if (epBufPacket) {
        free((void *)epBufPacket);
        epBufPacket = NULL;
    }
    if (ep_endOpFunc) {
        ep_endOpFunc(EEP_READY);
    }
    epAdr = epAdrStartWr;
    epProcess = false;
}
/************************************************************************/
/* Запись очередного байта закончена берется следующий байт             */
/************************************************************************/
ISR(EE_RDY_vect)
{
    static uint8_t stepFinal;

    if (epAdrWrite == (epAdrStartWr+1)) {		//Старт записи, первый байт записан
        stepFinal = 0;
    }
    if ((epAdrWrite - epAdrStartWr+1) == epPacketSize) {			//записан весь пакет
        switch (stepFinal) {
        case 0:
            WrieByteEeprom(epAdrStartWr, EEPROM_SIGNATURE);			//Начало нового пакета помечаем - готов к использованию
            stepFinal++;
            return;
        case 1:
            stepFinal++;
            if (epAdr == EEPROM_NULL) {
                epEndWriteOperation();								//Предыдущего пакета нет, операция закончена
                return;
            }
            WrieByteEeprom(epAdr, EEPROM_NO_SIGNATURE);				//предыдущий пакет, помечаем как неактуальный
            return;
        case 2:
            epEndWriteOperation();									//Работа автомата закончена
            return;
        default:
            break;
        }
    }
    ++epAdrWrite;
    WrieByteEeprom(epAdrWrite, *(epBufPacket + (epAdrWrite - epAdrStartWr-1)));
}

/************************************************************************/
/* Инициализация памяти eeprom - проверяется наличие актуальной записи  */
/************************************************************************/
epState_t EepromInit(size_t sizePacket)
{
    uint8_t	Sig;
    epState_t ret = EEP_BAD;

    epPacketSize = sizePacket+1;
    epProcess = true;
    epAdr = EEPROM_BASE;
    do {																		//Поиск актуальных данных в списке пакетов
        Sig = EeprmReadByte(epAdr);
        epAdr += epPacketSize;												//Следующий адрес пакета
    } while ((Sig == EEPROM_NO_SIGNATURE) && (epAdr < (E2END-epPacketSize)));
    if (Sig == EEPROM_SIGNATURE) {											//Если данные актуальны то адрес eeprom указывает на них, в противном случае NULL
        epAdr -= epPacketSize;												//Отматываем назад лишние байты в адресе
        ret = EEP_READY;
    } else {
        epAdr = EEPROM_NULL;
    }
    epProcess = false;
    epWriteInterruptOff();															//Прерывания от eeprom записи запрещены
    return ret;
}


/************************************************************************/
/* записывает в память область указанной памяти размера пакета			*/
/* указанного при инициализации											*/
/* по окончании вызвает функцию в которую передает результат записи     */
/* Возвращает ошибку при превышении размера памяти или еще чего         */
/************************************************************************/
epState_t EepromStartWrite(uint8_t *value, epEndOp cb_endOp)
{
    if ((value == NULL) || (epPacketSize > E2END) || (cb_endOp == NULL) || (epProcess)) {
        return EEP_BAD;
    }
    if (epBufPacket) {
        free((void *)epBufPacket);
    }
    epBufPacket = malloc(epPacketSize);
    if (!epBufPacket) {
        return EEP_BAD;
    }
    epProcess = true;
    ep_endOpFunc = cb_endOp;//функция вызова по окончании операции
    uint16_t count = 0;//пакет в буфер
    for( ; count<epPacketSize-1; count++) {
        *(epBufPacket+count) = *(value+count);
    }
    epAdrStartWr = (epAdr == EEPROM_NULL)?EEPROM_BASE:epAdrNextPacket();
    epAdrWrite = epAdrStartWr+1;//1 байт для флага пропускаем
    WrieByteEeprom(epAdrWrite, *epBufPacket); // старт записи
    epWriteInterruptOn();
    return EEP_READY;
}

/************************************************************************/
/* Прочитать данные в value*/
/************************************************************************/
epState_t EepromRead(uint8_t *value)
{
    if ((value == NULL) || (epPacketSize > E2END) || (epProcess) || (epAdr == EEPROM_NULL)) {
        return EEP_BAD;
    }
    uint16_t count = 1;//первый байт - это байт флага
    for(; count<epPacketSize; count++) {
        *(value+count-1) = EeprmReadByte(epAdr+count);
    }
    return EEP_READY;
}
