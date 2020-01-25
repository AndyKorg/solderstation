/*
* ������ ��������� � eeprom
* ��������! ��-�� ����������� ������ ��� ���������� ��������� ���������� ��������� ����� ������ ������������ ���������!
*/
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <avr/eeprom.h>
#include "eeprom.h"


volatile uint16_t epAdrWrite, epAdrStartWr; 								//������ ������ � eeprom - ������� � ���������
volatile uint16_t epPacketSize;												//������ ������
uint16_t epAdr;																//����� ������ ������. ���� ����� EEPROM_NULL �� ����� �� ���������
volatile bool epProcess = false;											//���� �������� � eeprom

volatile uint8_t epStepWr;													//������� ����� ���������� ������� ��� ���� ������� ������� �� ���������� ��������� ������
volatile uint8_t epCountWr;

epEndOp	ep_endOpFunc = NULL;
volatile uint8_t *epBufPacket = NULL;										//����� ������

#define epWriteInterruptOff()	do {EECR &= ~(1<<EERIE);} while (0)			//��������� ���������� �� EEPROM, � ������������� � ��������� ������
#define epWriteInterruptOn()	do {EECR |= (1<<EERIE);} while(0)			//��������� ���������� �� ����� ������ - ����� ������
#define EEPROM_SIGNATURE		0xaa										//��������� � EEPROM �������������
#define EEPROM_NO_SIGNATURE		0x55										//��������� � EEPROM �� �������������, ���������� ��������� ��������� �����

#define EEPROM_BASE				0											//������� ����� � �������� ���������� �������� � eeprom
#define EEPROM_NULL				(E2END+1)									//������� �������������� ������ eeprom

#define eepromWriteBusy()		((EECR & (1<<(EEWE))) != 0)					//������ ������ �������
#define eepromWriteIsFree()		((EECR & (1<<(EEWE))) == 0)					//������ �� ������ ��������� ������


/************************************************************************/
/* ������ ����� �� eeprom									            */
/************************************************************************/
uint16_t EeprmReadByte(volatile const uint16_t Adr)
{
    while(eepromWriteBusy());												//�������� ���������� ���������� �������� � EEPROM
    EEAR = Adr;
    EECR |= (1<<EERE);
    return EEDR;
}

/************************************************************************/
/* ������ ����� �� ������ epAdrWrite                                    */
/************************************************************************/
inline void WrieByteEeprom(uint16_t adr, uint8_t Value)
{
    while(eepromWriteBusy());												//�������� ���������� ���������� �������� � EEPROM, ��� �� ������ ������. ������ ������ �� ���� ������ ����������
    EEAR = adr;
    EECR |= (1<<EERE);
    if (EEDR != Value) {														//����� �� ���������, ���� ����������
        EEDR = Value;
        cli();
        EECR |= (1<<EEMWE);													//��������� ������
        EECR |= (1<<EEWE);													//�������
        sei();
    }
}

/************************************************************************/
/* ���������� ����� � eeprom ��� ���������� ������                      */
/************************************************************************/
inline uint16_t epAdrNextPacket(void)
{
    if ((epAdr == EEPROM_NULL) || ((epAdr+epPacketSize) > E2END)) {		//����� ������ ��� �� ��������� ��� ����� ������ �� ������� ������� ������ eeprom, ����� � ������ ������ eeprom
        return EEPROM_BASE;
    }
    return epAdr+epPacketSize;										//������� ����� �������� �� ������ ������
}

//�������� ���������, ���������� �� ������ ���������, ������� ������� ��������� ��������
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
/* ������ ���������� ����� ��������� ������� ��������� ����             */
/************************************************************************/
ISR(EE_RDY_vect)
{
    static uint8_t stepFinal;

    if (epAdrWrite == (epAdrStartWr+1)) {		//����� ������, ������ ���� �������
        stepFinal = 0;
    }
    if ((epAdrWrite - epAdrStartWr+1) == epPacketSize) {			//������� ���� �����
        switch (stepFinal) {
        case 0:
            WrieByteEeprom(epAdrStartWr, EEPROM_SIGNATURE);			//������ ������ ������ �������� - ����� � �������������
            stepFinal++;
            return;
        case 1:
            stepFinal++;
            if (epAdr == EEPROM_NULL) {
                epEndWriteOperation();								//����������� ������ ���, �������� ���������
                return;
            }
            WrieByteEeprom(epAdr, EEPROM_NO_SIGNATURE);				//���������� �����, �������� ��� ������������
            return;
        case 2:
            epEndWriteOperation();									//������ �������� ���������
            return;
        default:
            break;
        }
    }
    ++epAdrWrite;
    WrieByteEeprom(epAdrWrite, *(epBufPacket + (epAdrWrite - epAdrStartWr-1)));
}

/************************************************************************/
/* ������������� ������ eeprom - ����������� ������� ���������� ������  */
/************************************************************************/
epState_t EepromInit(size_t sizePacket)
{
    uint8_t	Sig;
    epState_t ret = EEP_BAD;

    epPacketSize = sizePacket+1;
    epProcess = true;
    epAdr = EEPROM_BASE;
    do {																		//����� ���������� ������ � ������ �������
        Sig = EeprmReadByte(epAdr);
        epAdr += epPacketSize;												//��������� ����� ������
    } while ((Sig == EEPROM_NO_SIGNATURE) && (epAdr < (E2END-epPacketSize)));
    if (Sig == EEPROM_SIGNATURE) {											//���� ������ ��������� �� ����� eeprom ��������� �� ���, � ��������� ������ NULL
        epAdr -= epPacketSize;												//���������� ����� ������ ����� � ������
        ret = EEP_READY;
    } else {
        epAdr = EEPROM_NULL;
    }
    epProcess = false;
    epWriteInterruptOff();															//���������� �� eeprom ������ ���������
    return ret;
}


/************************************************************************/
/* ���������� � ������ ������� ��������� ������ ������� ������			*/
/* ���������� ��� �������������											*/
/* �� ��������� ������� ������� � ������� �������� ��������� ������     */
/* ���������� ������ ��� ���������� ������� ������ ��� ��� ����         */
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
    ep_endOpFunc = cb_endOp;//������� ������ �� ��������� ��������
    uint16_t count = 0;//����� � �����
    for( ; count<epPacketSize-1; count++) {
        *(epBufPacket+count) = *(value+count);
    }
    epAdrStartWr = (epAdr == EEPROM_NULL)?EEPROM_BASE:epAdrNextPacket();
    epAdrWrite = epAdrStartWr+1;//1 ���� ��� ����� ����������
    WrieByteEeprom(epAdrWrite, *epBufPacket); // ����� ������
    epWriteInterruptOn();
    return EEP_READY;
}

/************************************************************************/
/* ��������� ������ � value*/
/************************************************************************/
epState_t EepromRead(uint8_t *value)
{
    if ((value == NULL) || (epPacketSize > E2END) || (epProcess) || (epAdr == EEPROM_NULL)) {
        return EEP_BAD;
    }
    uint16_t count = 1;//������ ���� - ��� ���� �����
    for(; count<epPacketSize; count++) {
        *(value+count-1) = EeprmReadByte(epAdr+count);
    }
    return EEP_READY;
}
