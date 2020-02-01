/*
 * only STLED316 mode
 *
 */
#include <avr/io.h>
#include <stdbool.h>
#include "EERTOS.h"
#include "buzzer.h"
#include "common.h"

#define BuzzerPinOff()	do {BUZZER_PORT &= ~(1<<BUZZER_PIN);} while (0);
#define BuzzerPinOn()	do {BUZZER_PORT |= (1<<BUZZER_PIN);} while (0);

#define SHORT_PERIOD	100
#define LONG_PERIOD		(SHORT_PERIOD*10)

volatile bool buzzerIsOn = false, buzzerProtect = false;
volatile uint8_t countbeep = 0;
void buzzerOff(void);

//������ ��������� ������� ��������
void buzzerProtectEnd(void)
{
    buzzerProtect = false;
}


//��������� � ������ ������� ����������
void buzzerOn(void)
{
    if (countbeep) {
        countbeep--;
        buzzerIsOn = true;
        SetTimerTask(buzzerOff, SHORT_PERIOD);
    }
}
//���������� ������� ������ ��������� �����
void buzzerOff(void)
{
    buzzerIsOn = false;
    buzzerProtect = true;
    SetTimerTask(buzzerProtectEnd, 500);
    if (countbeep) {
        SetTimerTask(buzzerOn, SHORT_PERIOD);
    }
}
void buzzerBeep(void)
{
    static bool prev = false;
    if (buzzerIsOn) {
        if (prev) {
            BuzzerPinOn();
        } else {
            BuzzerPinOff();
        }
        prev ^= true;
    } else {
        BuzzerPinOff();
    }
    SetTimerTask(buzzerBeep, 1000/BUZZER_FREQUENCY_HZ);
}

void buzzerInit(void)
{
    PinOutputMode(BUZZER_PORT, BUZZER_PIN);
    BuzzerPinOff();
    buzzerIsOn = false;
    countbeep = 0;
    SetTask(buzzerBeep);
}

void buzzerShow(sound_type_t sound)
{
    if (buzzerIsOn || countbeep || buzzerProtect) return;//���� ����� ������ ����� ��������
    switch (sound) {
    case SOUND_SHORT:
        buzzerIsOn = true;
        countbeep = 0;
        SetTimerTask(buzzerOff, SHORT_PERIOD);
        break;
    case SOUND_LONG:
        buzzerIsOn = true;
        countbeep = 0;
        SetTimerTask(buzzerOff, LONG_PERIOD);
        break;
    case SOUND_FLASH_3:
        buzzerIsOn = true;
        countbeep = 3;
        SetTimerTask(buzzerOff, SHORT_PERIOD);
        break;
    default:
        break;
    }
}
