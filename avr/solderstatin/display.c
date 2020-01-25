/*
* display.c
*
*/

#include <stdio.h>
#include <avr/io.h>
#include <stdlib.h>
#include <stdbool.h>
#include "avrlibtypes.h"
#include "common.h"
#include "EERTOS.h"
#include "HAL.h"
#include "wg12864b.h"
#include "display.h"

#include "console.h"

#define  DISPLAY_MAX_BUF 32

uint8_t flash = 0;
bool forceShowNeedSolder = false, forceShowNeedFan = false;		//�������� �������� ������� �����������

//��������� ������
typedef struct {
    uint8_t x;					//��������� ����������
    uint8_t y;
    char str[DISPLAY_MAX_BUF];	//���� ������, ����������� 0 ����������!
    sFONT *font;				//����� ������� ��������
    eColored color;				//����� ������
} dispString_t;

typedef uint8_t (*strOut)(dispString_t *value);

static strOut stringOut;//������� ������������ ��������� ������
static uint8_t fan_heat_tempr_end_x = 0; //���������� x ��������� ������ ������ ����������� ����

inline uint8_t print_state(dispString_t *value, const eState state)
{
    if (state == STATE_NO_DEVICE) {
        return (uint8_t) snprintf(value->str, DISPLAY_MAX_BUF, "%s", " ---  ");
    } else if(state == STATE_OFF) {
        return (uint8_t) snprintf(value->str, DISPLAY_MAX_BUF, "%s", " OFF");
    }
    return (uint8_t) snprintf(value->str, DISPLAY_MAX_BUF, "%s", " ON  ");
}


void HideNeedTemperSolder()
{
    forceShowNeedSolder = false;
}

void HideNeedTemperFan()
{
    forceShowNeedFan = false;
}

void SetForceFlagTempr(device_t *dev, eState *prev, bool *flag, TPTR hideFunc)
{
    if ((dev->state == STATE_ON) && ((*prev) != STATE_ON)) {
        *flag = true;
        SetTimerTask(hideFunc, SHOW_NEED_PERIOD_S*1000);
    }
    if (dev->state != STATE_ON) {  	//�������� ������� ������������ ���� ����� �� ������ ���������
        *flag = false;
    }
    *prev = dev->state;
}

inline uint8_t itoaFlash(device_t dev, char *buf, bool forceShowNeed)
{
    if ((dev.state == STATE_SET) && (flash)) {
        return (uint8_t) snprintf(buf, DISPLAY_MAX_BUF, "    ");
    }
    return (uint8_t) snprintf(buf, DISPLAY_MAX_BUF, "%04d", forceShowNeed?dev.need:((dev.state == STATE_SET)?dev.setSelect:dev.current));
}

uint8_t solderTempr(dispString_t *value);

uint8_t fan_value(dispString_t *value)
{
    static u16 prev = 0;
    stringOut = solderTempr;
    if ((fan_heat_tempr_end_x) && (prev != fan.current)) {
        value->x = fan_heat_tempr_end_x+3;
        value->y = DISPLAY_Y_MAX - Font8.Height-2;//������� �� ���� ������ ����� �� ������ ������
        value->color = COLORED_SHOW;
        value->font = &Font8;
        if (fan.state != STATE_ON) {
            prev = 0;
            return snprintf(value->str, DISPLAY_MAX_BUF, " ---   ");
        }
        prev = fan.current;
        uint8_t tmp = fan.current/(fan.limitADC/100);
        return snprintf(value->str, DISPLAY_MAX_BUF, " %03d%%  ", tmp>100?100:tmp);
    }
    return 0;//�� �������� ������ �.�. ���������� ����������
}

uint8_t fan_headState(dispString_t *value)
{
    value->x = value->x;
    fan_heat_tempr_end_x = value->x;	//��������� ��������� ������ ����������� ����
    value->y = value->y;				//������ ������ ������� �����
    value->color = COLORED_SHOW;
    value->font = &FontSmall;
    stringOut = fan_value;
    return print_state(value, fan_heat.state);
}

uint8_t fan_headTempr(dispString_t *value)
{
    static eState prev = STATE_OFF;
    value->x = 0;//������ �� ��������� ���� ������
    value->y = enterY(0, 0, &FontSuperBigDigit)+4;//��������� ������
    value->color = COLORED_SHOW;
    value->font = &FontSuperBigDigit;
    stringOut = fan_headState;
    SetForceFlagTempr(&fan_heat, &prev, &forceShowNeedFan, HideNeedTemperFan);
    return itoaFlash(fan_heat, value->str, forceShowNeedFan);
}

uint8_t solderState(dispString_t *value)
{
    value->x = value->x; //������ � ����� ���������� ������
    value->y = 0;
    value->color = COLORED_SHOW;
    value->font = &FontSmall;
    stringOut = fan_headTempr;
    return print_state(value, solder.state);
}

uint8_t solderTempr(dispString_t *value)
{
    static eState prev = STATE_OFF;
    value->x = 0;
    value->y = 0;
    value->color = COLORED_SHOW;
    value->font = &FontSuperBigDigit;
    stringOut = solderState;
    SetForceFlagTempr(&solder, &prev, &forceShowNeedSolder, HideNeedTemperSolder);
    return itoaFlash(solder, value->str, forceShowNeedSolder);
}

//��������� ��������� ������ � ����� ���������� ������� �
void ShowString(void)
{
    static dispString_t current;	//������� ��������� ������
    static uint8_t charCount = 0, currPos;
    if (charCount == 0) {
        if (stringOut) {
            charCount = stringOut(&current);
            if (charCount >DISPLAY_MAX_BUF) {
                charCount = DISPLAY_MAX_BUF;
                current.str[DISPLAY_MAX_BUF-1] = 0;
            }
            currPos = 0;
        }
    }
    if(charCount) {
        current.x = drawCharAt(current.x, current.y, current.str[currPos++], current.font, current.color, ShowString);//������� ��������� ������ � �� ��������� ������� ������� ShowString
        charCount--;
    } else {
        SetTask(ShowString);//�������� ������ ������� ����� ������� �����
    }
}

void flashSwitch(void)
{
    flash ^= 0xff;
    SetTimerTask(flashSwitch, FLASH_PERIOD_MS);
}

void display_init(void)
{
    stringOut = solderTempr;	//������ ����������� ���������
    wg12864_init(ShowString);	//������������� ���
    SetTimerTask(flashSwitch, FLASH_PERIOD_MS);//���� ������� ��������
}